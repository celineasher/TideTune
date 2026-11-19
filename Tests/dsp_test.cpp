#include "../Source/PitchEngine.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

static void testScale() {
    using namespace tidetune;
    assert(inScale(0, 0, 1));
    assert(!inScale(1, 0, 1));
    assert(inScale(7, 0, 1));
    assert(nearestAllowedMidi(61.2f, 0, 1) == 62);
    assert(nearestAllowedMidi(60.1f, 0, 1) == 60);
    assert(nearestAllowedMidi(69.0f, 9, 1) == 69);
    assert(nearestAllowedMidi(60.1f, 0, 0, 0xffe) == 61);  // C disabled
    assert(nearestAllowedMidi(60.1f, 0, 0, 0x000) == -1); // all disabled
}

static void testPitch(float freq, float expectedMidi, double sr = 48000.0,
                      int vocalRange = 3) {
    tidetune::PitchEngine engine;
    engine.prepare(sr);
    tidetune::Settings settings;
    settings.vocalRange = vocalRange;
    float block[256] {};
    float* data[] {block};
    for (int k = 0; k < 240; ++k) {
        for (int i = 0; i < 256; ++i) {
            const int t = k * 256 + i;
            block[i] = 0.25f * std::sin(2.0 * tidetune::pi * freq * t / sr);
        }
        engine.process(data, 1, 256, settings);
        for (float value : block) assert(std::isfinite(value));
    }
    const float detected = engine.detectedHz.load();
    const float midi = engine.detectedMidi.load();
    std::printf("%6.0f Hz sample-rate, %.1f Hz input => %.2f Hz (MIDI %.2f)\n",
                sr, freq, detected, midi);
    assert(std::abs(detected - freq) < freq * 0.03f);
    assert(std::abs(midi - expectedMidi) < 0.4f);
}

static double magnitudeAt(const std::vector<float>& x, int start, int end,
                          double freq, double sr) {
    double re = 0.0, im = 0.0;
    for (int i = start; i < end; ++i) {
        const double angle = 2.0 * tidetune::pi * freq * (i - start) / sr;
        re += x[i] * std::cos(angle);
        im -= x[i] * std::sin(angle);
    }
    return std::sqrt(re * re + im * im);
}

static void testCorrection(bool hard) {
    constexpr double sr = 48000.0;
    tidetune::PitchEngine engine;
    engine.prepare(sr);
    tidetune::Settings s;
    s.scale = 0;
    s.hard = hard;
    s.retuneMs = 0.0f;
    s.humanize = 0.0f;
    s.strength = 1.0f;
    std::vector<float> output;
    float block[256] {};
    float* data[] {block};
    for (int k = 0; k < 400; ++k) {
        for (int i = 0; i < 256; ++i) {
            const int t = k * 256 + i;
            block[i] = 0.4f * std::sin(2.0 * tidetune::pi * 450.0 * t / sr);
        }
        engine.process(data, 1, 256, s);
        output.insert(output.end(), block, block + 256);
    }
    const int begin = 48000;
    const int end = begin + 12000;
    const double targetPower = magnitudeAt(output, begin, end, 440.0, sr);
    const double originalPower = magnitudeAt(output, begin, end, 450.0, sr);
    std::printf("%s correction: 440Hz magnitude %.1f vs 450Hz %.1f\n",
                hard ? "Hard" : "Smooth", targetPower, originalPower);
    assert(engine.targetMidi.load() == 69);
    assert(targetPower > originalPower * 2.0);
}

static void testBypassAndSilence() {
    tidetune::PitchEngine engine;
    engine.prepare(48000.0);
    const int delay = engine.latencySamples();
    assert(delay > 0 && delay < 8192);
    tidetune::Settings s;
    s.bypass = true;
    float left[256], right[256];
    float* data[] {left, right};
    for (int block = 0; block < 300; ++block) {
        for (int j = 0; j < 256; ++j) {
            left[j] = (block * 256 + j) % 23 == 0 ? 0.25f : 0.0f;
            right[j] = (block * 256 + j) % 31 == 0 ? 0.2f : 0.0f;
        }
        engine.process(data, 2, 256, s);
        for (int j = 0; j < 256; ++j) {
            const int t = block * 256 + j - delay;
            const float expectedL = t >= 0 && t % 23 == 0 ? 0.25f : 0.0f;
            const float expectedR = t >= 0 && t % 31 == 0 ? 0.2f : 0.0f;
            assert(std::abs(left[j] - expectedL) < 0.001f);
            assert(std::abs(right[j] - expectedR) < 0.001f);
        }
    }
}

static void testVoicedNoise() {
    tidetune::PitchEngine engine;
    engine.prepare(48000.0);
    tidetune::Settings s;
    s.vocalRange = 1;
    float dataBuffer[256];
    float* data[] {dataBuffer};
    unsigned state = 94841;
    for (int block = 0; block < 160; ++block) {
        for (float& sample : dataBuffer) {
            state = 1664525u * state + 1013904223u;
            sample = 0.01f * (static_cast<float>((state >> 8) & 0xffff) / 32768.0f - 1.0f);
        }
        engine.process(data, 1, 256, s);
        for (float sample : dataBuffer) assert(std::isfinite(sample));
    }
    assert(engine.detectedMidi.load() < 0.0f);
}

int main() {
    testScale();
    testPitch(220.0f, 57.0f);
    testPitch(440.0f, 69.0f);
    testPitch(261.626f, 60.0f);
    testPitch(110.0f, 45.0f, 48000.0, 0);
    testPitch(660.0f, 76.0f, 48000.0, 2);
    testPitch(440.0f, 69.0f, 96000.0);
    testPitch(440.0f, 69.0f, 192000.0);
    testCorrection(true);
    testCorrection(false);
    testBypassAndSilence();
    testVoicedNoise();
    std::puts("All standalone DSP tests passed.");
}
