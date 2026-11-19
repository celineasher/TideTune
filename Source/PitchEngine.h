#pragma once

// TideTune 0.2: standalone, allocation-free monophonic vocal pitch correction.
// No JUCE dependencies. The algorithm is a low-latency dual-grain shifter,
// not a formant-preserving PSOLA/vocoder. Keep expectations realistic.

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace tidetune {
inline constexpr double pi = 3.14159265358979323846;

struct Settings {
    int root = 0;
    int scale = 1;             // Chromatic, Major, Minor, Harmonic Minor, Maj Penta, Min Penta
    int vocalRange = 1;        // 0 Low, 1 Mid, 2 High, 3 Wide
    int transpose = 0;         // semitones, -12..+12
    uint16_t noteMask = 0x0fff;// pitch classes (C bit 0, B bit 11)
    float retuneMs = 3.0f;
    float strength = 1.0f;
    float humanize = 0.0f;
    float flex = 0.0f;
    float vibrato = 0.0f;     // generated vibrato, 0..1
    float mix = 1.0f;
    bool hard = true;
    bool bypass = false;
};

inline bool inScale(int semitone, int root, int scale) noexcept {
    static constexpr bool pattern[6][12] = {
        {1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,1,0,1,1,0,1,0,1,0,1},
        {1,0,1,1,0,1,0,1,1,0,1,0},
        {1,0,1,1,0,1,0,1,1,0,0,1},
        {1,0,1,0,1,0,0,1,0,1,0,0},
        {1,0,0,1,0,1,0,1,0,0,1,0}
    };
    const int degree = (semitone - root + 24) % 12;
    return pattern[std::clamp(scale, 0, 5)][degree];
}

inline bool allowed(int midi, int root, int scale, uint16_t mask) noexcept {
    const int pitchClass = (midi % 12 + 12) % 12;
    return inScale(pitchClass, root, scale) && (mask & (1u << pitchClass)) != 0;
}

inline int nearestAllowedMidi(float midi, int root, int scale,
                              uint16_t mask = 0x0fff) noexcept {
    const int center = static_cast<int>(std::round(midi));
    int chosen = center;
    float bestDistance = 100.0f;
    for (int candidate = center - 12; candidate <= center + 12; ++candidate) {
        if (allowed(candidate, root, scale, mask)) {
            const float distance = std::abs(midi - static_cast<float>(candidate));
            if (distance < bestDistance) { bestDistance = distance; chosen = candidate; }
        }
    }
    // A completely disabled scale is intentionally a no-correction condition.
    return bestDistance < 99.0f ? chosen : -1;
}

class PitchEngine final {
public:
    static constexpr int delayBufferSize = 32768;
    static constexpr int delayMask = delayBufferSize - 1;
    static constexpr int analysisSize = 1024;
    static constexpr int maxAnalysisLag = 480;

    void prepare(double newSampleRate) noexcept {
        sampleRate_ = std::clamp(newSampleRate, 8000.0, 192000.0);
        decimationFactor_ = std::max(1, static_cast<int>(std::round(sampleRate_ / 24000.0)));
        grainLength_ = static_cast<float>(std::clamp(sampleRate_ * (1024.0 / 48000.0), 512.0, 4096.0));
        baseDelay_ = grainLength_ * 0.75f;
        dryDelaySamples_ = static_cast<int>(std::round(baseDelay_ + grainLength_ * 0.5f));
        reset();
    }

    int latencySamples() const noexcept { return dryDelaySamples_; }

    void reset() noexcept {
        for (auto& channel : delayLine_) channel.fill(0.0f);
        analysisRing_.fill(0.0f);
        analysisWork_.fill(0.0f);
        yin_.fill(1.0f);
        writePos_ = 0;
        analysisWritePos_ = 0;
        samplesSeen_ = 0;
        decimationCounter_ = 0;
        detectCountdown_ = 256;
        phase_ = 0.0f;
        ratio_ = desiredRatio_ = 1.0f;
        activity_ = 0.0f;
        lockedTarget_ = -1;
        noteAge_ = 0;
        vibratoPhase_ = 0.0f;
        detectedHz.store(0.0f, std::memory_order_relaxed);
        detectedMidi.store(-1.0f, std::memory_order_relaxed);
        targetMidi.store(-1, std::memory_order_relaxed);
        confidence.store(0.0f, std::memory_order_relaxed);
        correctionCents.store(0.0f, std::memory_order_relaxed);
    }

    // In-place processing, no allocations, with 1 or 2 channels.
    void process(float** channels, int channelCount, int numSamples, const Settings& s) noexcept {
        if (channelCount < 1 || numSamples < 1 || channels == nullptr || channels[0] == nullptr) return;
        const int count = std::min(channelCount, 2);
        const float mix = s.bypass ? 0.0f : std::clamp(s.mix, 0.0f, 1.0f);
        const float response = std::clamp(s.retuneMs, 0.0f, 400.0f);
        const float humanize = std::clamp(s.humanize, 0.0f, 1.0f);
        const float timeMs = (s.hard ? 0.65f : 4.0f) + response * (s.hard ? 1.0f : 1.6f)
                            * (1.0f + humanize * 1.8f);
        const float slew = 1.0f - std::exp(-1000.0f / (timeMs * static_cast<float>(sampleRate_)));
        const float activitySlew = 1.0f - std::exp(-1.0f / (0.008f * static_cast<float>(sampleRate_)));
        const float vibratoStep = 5.4f / static_cast<float>(sampleRate_);

        for (int i = 0; i < numSamples; ++i) {
            const float left = channels[0][i];
            const float right = count > 1 && channels[1] != nullptr ? channels[1][i] : left;
            delayLine_[0][writePos_] = left;
            delayLine_[1][writePos_] = right;
            const float mono = (left + right) * 0.5f;
            if (++decimationCounter_ >= decimationFactor_) {
                decimationCounter_ = 0;
                analysisRing_[analysisWritePos_] = mono;
                analysisWritePos_ = (analysisWritePos_ + 1) % analysisSize;
            }
            ++samplesSeen_;
            if (--detectCountdown_ <= 0) {
                detectCountdown_ = 256;
                estimatePitch(s);
            }
            float targetRatio = desiredRatio_;
            if (!s.bypass && s.vibrato > 0.001f && lockedTarget_ >= 0) {
                const float offsetSemitones = std::sin(2.0f * static_cast<float>(pi) * vibratoPhase_)
                                             * 0.35f * std::clamp(s.vibrato, 0.0f, 1.0f);
                targetRatio *= std::pow(2.0f, offsetSemitones / 12.0f);
            }
            vibratoPhase_ += vibratoStep;
            if (vibratoPhase_ >= 1.0f) vibratoPhase_ -= 1.0f;
            ratio_ += slew * (targetRatio - ratio_);
            ratio_ = std::clamp(ratio_, 0.45f, 2.2f);

            // Crossfade to aligned dry on clean/unvoiced passages, avoiding
            // permanent comb filtering when desired shift is zero.
            const float activeTarget = (std::abs(targetRatio - 1.0f) > 0.00035f && !s.bypass) ? 1.0f : 0.0f;
            activity_ += activitySlew * (activeTarget - activity_);

            const float delta = std::abs(ratio_ - 1.0f);
            phase_ += delta / grainLength_;
            if (phase_ >= 1.0f) phase_ -= std::floor(phase_);
            const bool upward = ratio_ >= 1.0f;
            const float secondPhase = phase_ < 0.5f ? phase_ + 0.5f : phase_ - 0.5f;
            const float delayA = baseDelay_ + (upward ? (1.0f - phase_) : phase_) * grainLength_;
            const float delayB = baseDelay_ + (upward ? (1.0f - secondPhase) : secondPhase) * grainLength_;
            const float windowA = std::sin(static_cast<float>(pi) * phase_);
            const float windowB = std::sin(static_cast<float>(pi) * secondPhase);
            const float gainA = windowA * windowA;
            const float gainB = windowB * windowB;
            for (int ch = 0; ch < count; ++ch) {
                const float dry = readDelay(ch, static_cast<float>(dryDelaySamples_));
                const float wet = gainA * readDelay(ch, delayA) + gainB * readDelay(ch, delayB);
                channels[ch][i] = dry + (wet - dry) * (mix * activity_);
            }
            writePos_ = (writePos_ + 1) & delayMask;
        }
    }

    std::atomic<float> detectedHz {0.0f};
    std::atomic<float> detectedMidi {-1.0f};
    std::atomic<int> targetMidi {-1};
    std::atomic<float> confidence {0.0f};
    std::atomic<float> correctionCents {0.0f};

private:
    float readDelay(int ch, float delay) const noexcept {
        const float position = static_cast<float>(writePos_) - delay;
        const int whole = static_cast<int>(std::floor(position));
        const float fraction = position - static_cast<float>(whole);
        const auto& b = delayLine_[ch];
        return b[whole & delayMask] * (1.0f - fraction)
               + b[(whole + 1) & delayMask] * fraction;
    }

    void estimatePitch(const Settings& s) noexcept {
        if (samplesSeen_ < static_cast<uint64_t>(analysisSize * decimationFactor_)) return;
        const int n = analysisSize;
        for (int i = 0; i < n; ++i)
            analysisWork_[i] = analysisRing_[(analysisWritePos_ + n - analysisSize + i) % n];

        float energy = 0.0f;
        for (int i = 256; i < 768; ++i) energy += analysisWork_[i] * analysisWork_[i];
        if (energy < (512.0f * 0.006f * 0.006f)) { setUnvoiced(); return; }

        // YIN difference and cumulative mean difference, with range-specific lags.
        const float rate = static_cast<float>(sampleRate_ / decimationFactor_);
        static constexpr float rangeMin[4] {55.0f, 80.0f, 140.0f, 55.0f};
        static constexpr float rangeMax[4] {350.0f, 750.0f, 1200.0f, 1200.0f};
        const int index = std::clamp(s.vocalRange, 0, 3);
        const int minLag = std::max(2, static_cast<int>(rate / rangeMax[index]));
        const int maxLag = std::min(maxAnalysisLag, static_cast<int>(rate / rangeMin[index]));
        constexpr int span = 512;
        if (maxLag >= analysisSize - span) { setUnvoiced(); return; }

        float cumulative = 0.0f;
        int bestLag = -1;
        float bestYin = 1.0f;
        for (int lag = 1; lag <= maxLag; ++lag) {
            float difference = 0.0f;
            for (int j = 0; j < span; ++j) {
                const float d = analysisWork_[j] - analysisWork_[j + lag];
                difference += d * d;
            }
            cumulative += difference;
            yin_[lag] = cumulative > 1.0e-12f
                       ? difference * static_cast<float>(lag) / cumulative : 1.0f;
            if (lag >= minLag && yin_[lag] < bestYin) { bestYin = yin_[lag]; bestLag = lag; }
        }
        int selected = -1;
        for (int lag = minLag; lag < maxLag; ++lag) {
            if (yin_[lag] < 0.16f) {
                while (lag + 1 <= maxLag && yin_[lag + 1] < yin_[lag]) ++lag;
                selected = lag;
                break;
            }
        }
        if (selected < 0 && bestYin < 0.26f) selected = bestLag;
        if (selected < 1) { setUnvoiced(); return; }
        float lag = static_cast<float>(selected);
        if (selected > 1 && selected < maxLag) {
            const float a = yin_[selected - 1], b = yin_[selected], c = yin_[selected + 1];
            const float denominator = a - 2.0f * b + c;
            if (std::abs(denominator) > 0.00001f)
                lag += std::clamp(0.5f * (a - c) / denominator, -0.5f, 0.5f);
        }
        const float hz = rate / lag;
        if (!std::isfinite(hz) || hz < rangeMin[index] * 0.96f || hz > rangeMax[index] * 1.04f) {
            setUnvoiced(); return;
        }
        const float midi = 69.0f + 12.0f * std::log2(hz / 440.0f);
        const int proposed = nearestAllowedMidi(midi, s.root, s.scale, s.noteMask);
        if (proposed < 0) { setUnvoiced(); return; }

        // Small hysteresis reduces target-note chatter at semitone boundaries.
        if (lockedTarget_ < 0 || !allowed(lockedTarget_, s.root, s.scale, s.noteMask)
            || std::abs(midi - static_cast<float>(lockedTarget_)) > 0.64f)
            lockedTarget_ = proposed;
        if (lockedTarget_ == proposed) ++noteAge_;
        else noteAge_ = 0;
        noteAge_ = std::min(noteAge_, 10000);

        const float correction = static_cast<float>(lockedTarget_) - midi;
        const float flex = std::clamp(s.flex, 0.0f, 1.0f);
        const float distance = std::abs(correction);
        const float keepFlex = flex * (1.0f - std::clamp(distance / 0.75f, 0.0f, 1.0f));
        const float human = std::clamp(s.humanize, 0.0f, 1.0f);
        const float holdLength = std::clamp(static_cast<float>(noteAge_) * (256.0f / static_cast<float>(sampleRate_)) / 0.5f, 0.0f, 1.0f);
        const float effectiveStrength = std::clamp(s.strength, 0.0f, 1.0f) * (1.0f - keepFlex)
                                       * (1.0f - (s.hard ? 0.18f : 0.4f) * human * holdLength);
        const float semitoneShift = correction * effectiveStrength + static_cast<float>(s.transpose);
        desiredRatio_ = std::clamp(std::pow(2.0f, semitoneShift / 12.0f), 0.5f, 2.0f);
        detectedHz.store(hz, std::memory_order_relaxed);
        detectedMidi.store(midi, std::memory_order_relaxed);
        targetMidi.store(lockedTarget_ + s.transpose, std::memory_order_relaxed);
        confidence.store(1.0f - yin_[selected], std::memory_order_relaxed);
        correctionCents.store(semitoneShift * 100.0f, std::memory_order_relaxed);
    }

    void setUnvoiced() noexcept {
        desiredRatio_ = 1.0f;
        lockedTarget_ = -1;
        noteAge_ = 0;
        detectedHz.store(0.0f, std::memory_order_relaxed);
        detectedMidi.store(-1.0f, std::memory_order_relaxed);
        targetMidi.store(-1, std::memory_order_relaxed);
        confidence.store(0.0f, std::memory_order_relaxed);
        correctionCents.store(0.0f, std::memory_order_relaxed);
    }

    std::array<std::array<float, delayBufferSize>, 2> delayLine_ {};
    std::array<float, analysisSize> analysisRing_ {};
    std::array<float, analysisSize> analysisWork_ {};
    std::array<float, maxAnalysisLag + 1> yin_ {};
    double sampleRate_ = 48000.0;
    int writePos_ = 0, analysisWritePos_ = 0;
    uint64_t samplesSeen_ = 0;
    int decimationCounter_ = 0, decimationFactor_ = 2, detectCountdown_ = 256;
    int dryDelaySamples_ = 1280;
    int lockedTarget_ = -1, noteAge_ = 0;
    float grainLength_ = 1024.0f, baseDelay_ = 768.0f;
    float phase_ = 0.0f, ratio_ = 1.0f, desiredRatio_ = 1.0f;
    float activity_ = 0.0f, vibratoPhase_ = 0.0f;
};
} // namespace tidetune
