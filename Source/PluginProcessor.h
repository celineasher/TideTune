#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PitchEngine.h"
#include "SidechainEngine.h"
#include <array>
#include <atomic>

class TideTuneAudioProcessor final : public juce::AudioProcessor {
public:
    TideTuneAudioProcessor();
    ~TideTuneAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TideTune"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;
    tidetune::PitchEngine engine;
    tidetune::SidechainEngine sidechain;
    std::atomic<float> sidechainReductionDb {0.0f};
    std::atomic<bool> externalSidechainBusActive {false};

private:
    // Cached parameter pointers: no String construction or tree lookups on the audio thread.
    std::array<std::atomic<float>*, 36> parameterRefs_ {};
    float outputGainSmoothed_ = 1.0f;
    double sampleRate_ = 48000.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TideTuneAudioProcessor)
};
