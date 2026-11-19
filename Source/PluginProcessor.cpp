#include "PluginProcessor.h"
#include "PluginEditor.h"

TideTuneAudioProcessor::TideTuneAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "TideTuneSettings", createParameterLayout()) {
    const juce::StringArray ids {"root", "scale", "speed", "strength", "humanize", "mix",
                                 "bypass", "range", "character", "flex", "vibrato", "transpose"};
    for (int i = 0; i < ids.size(); ++i)
        parameterRefs_[static_cast<size_t>(i)] = parameters.getRawParameterValue(ids[i]);
    for (int i = 0; i < 12; ++i)
        parameterRefs_[static_cast<size_t>(12 + i)] =
            parameters.getRawParameterValue("note" + juce::String(i));
}

juce::AudioProcessorValueTreeState::ParameterLayout TideTuneAudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {"root", 1}, "Key",
        juce::StringArray {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {"scale", 1}, "Scale",
        juce::StringArray {"Chromatic", "Major", "Minor", "Harmonic Minor", "Major Pentatonic", "Minor Pentatonic"}, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"speed", 1}, "Retune Speed",
        juce::NormalisableRange<float>(0.0f, 400.0f, 0.1f, 0.42f), 3.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"strength", 1}, "Correction Strength",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"humanize", 1}, "Humanize",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"mix", 1}, "Wet Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID {"bypass", 1}, "Bypass", false));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {"range", 1}, "Vocal Range",
        juce::StringArray {"Low", "Mid / Tenor", "High", "Wide"}, 1));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {"character", 1}, "Correction Style",
        juce::StringArray {"HARD / Snap", "SMOOTH"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"flex", 1}, "Flex Tune",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"vibrato", 1}, "Added Vibrato",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID {"transpose", 1}, "Transpose", -12, 12, 0));
    const juce::StringArray noteNames {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (int i = 0; i < 12; ++i)
        p.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID {"note" + juce::String(i), 1},
            "Allow " + noteNames[i], true));

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& parameter : p) layout.add(std::move(parameter));
    return layout;
}

void TideTuneAudioProcessor::prepareToPlay(double sampleRate, int) {
    engine.prepare(sampleRate);
    setLatencySamples(engine.latencySamples());
}

void TideTuneAudioProcessor::releaseResources() {}

bool TideTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();
    return (input == juce::AudioChannelSet::stereo() || input == juce::AudioChannelSet::mono())
        && input == output;
}

void TideTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 1 || buffer.getNumSamples() < 1) return;
    auto val = [this](size_t i) {
        return parameterRefs_[i]->load(std::memory_order_relaxed);
    };
    tidetune::Settings s;
    s.root = static_cast<int>(val(0));
    s.scale = static_cast<int>(val(1));
    s.retuneMs = val(2);
    s.strength = val(3) / 100.0f;
    s.humanize = val(4) / 100.0f;
    s.mix = val(5) / 100.0f;
    s.bypass = val(6) > 0.5f;
    s.vocalRange = static_cast<int>(val(7));
    s.hard = val(8) < 0.5f;
    s.flex = val(9) / 100.0f;
    s.vibrato = val(10) / 100.0f;
    s.transpose = static_cast<int>(val(11));
    s.noteMask = 0;
    for (int i = 0; i < 12; ++i)
        if (val(static_cast<size_t>(12 + i)) > 0.5f)
            s.noteMask |= static_cast<uint16_t>(1u << i);

    float* channels[2] = { buffer.getWritePointer(0),
        buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr };
    engine.process(channels, buffer.getNumChannels(), buffer.getNumSamples(), s);
}

juce::AudioProcessorEditor* TideTuneAudioProcessor::createEditor() {
    return new TideTuneAudioProcessorEditor(*this);
}

void TideTuneAudioProcessor::getStateInformation(juce::MemoryBlock& data) {
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, data);
}

void TideTuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(parameters.state.getType().toString()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new TideTuneAudioProcessor();
}
