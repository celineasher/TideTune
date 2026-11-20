#include "PluginProcessor.h"
#include "PluginEditor.h"

TideTuneAudioProcessor::TideTuneAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::mono(), false)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "TideTuneSettings", createParameterLayout()) {
    const juce::StringArray ids {"root", "scale", "speed", "strength", "humanize", "mix",
                                 "bypass", "range", "character", "flex", "vibrato", "transpose"};
    for (int i = 0; i < ids.size(); ++i)
        parameterRefs_[static_cast<size_t>(i)] = parameters.getRawParameterValue(ids[i]);
    for (int i = 0; i < 12; ++i)
        parameterRefs_[static_cast<size_t>(12 + i)] =
            parameters.getRawParameterValue("note" + juce::String(i));
    const juce::StringArray scIds {"sc_enable","sc_source","sc_target","sc_threshold",
        "sc_ratio","sc_attack","sc_release","sc_knee","sc_amount","sc_hpf","sc_lpf","output_gain"};
    for (int i = 0; i < scIds.size(); ++i)
        parameterRefs_[static_cast<size_t>(24 + i)] = parameters.getRawParameterValue(scIds[i]);
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

    // V0.4: append unique parameter IDs, retaining all V0.3 IDs for session automation.
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID {"sc_enable",1}, "Sidechain Enabled",false));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {"sc_source",1}, "Sidechain Source",
        juce::StringArray {"Internal Vocal","External Bus"},0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {"sc_target",1}, "Sidechain Target",
        juce::StringArray {"Output Gain","Wet Mix","Correction Strength"},0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sc_threshold",1}, "SC Threshold",
        juce::NormalisableRange<float>(-60.0f,0.0f,0.1f),-24.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sc_ratio",1}, "SC Ratio",
        juce::NormalisableRange<float>(1.0f,20.0f,0.1f),4.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sc_attack",1}, "SC Attack",
        juce::NormalisableRange<float>(0.1f,100.0f,0.1f,0.5f),5.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sc_release",1}, "SC Release",
        juce::NormalisableRange<float>(10.0f,1000.0f,1.0f,0.45f),150.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sc_knee",1}, "SC Knee",
        juce::NormalisableRange<float>(0.0f,18.0f,0.1f),6.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sc_amount",1}, "SC Depth",
        juce::NormalisableRange<float>(0.0f,100.0f,0.1f),50.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sc_hpf",1}, "SC High-Pass",
        juce::NormalisableRange<float>(0.0f,500.0f,1.0f),0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sc_lpf",1}, "SC Low-Pass",
        juce::NormalisableRange<float>(0.0f,16000.0f,1.0f),0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"output_gain",1}, "Output Gain",
        juce::NormalisableRange<float>(-18.0f,12.0f,0.1f),0.0f));

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& parameter : p) layout.add(std::move(parameter));
    return layout;
}

void TideTuneAudioProcessor::prepareToPlay(double sampleRate,int) {
    sampleRate_ = sampleRate;
    engine.prepare(sampleRate);
    sidechain.prepare(sampleRate);
    outputGainSmoothed_ = 1.0f;
    setLatencySamples(engine.latencySamples());
}

void TideTuneAudioProcessor::releaseResources() {}

bool TideTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();
    if (!((input == juce::AudioChannelSet::stereo() || input == juce::AudioChannelSet::mono()) && input == output))
        return false;
    if (layouts.inputBuses.size() != 2 || layouts.outputBuses.size() != 1) return false;
    const auto& aux = layouts.inputBuses[1];
    return aux.isDisabled() || aux == juce::AudioChannelSet::mono()
           || aux == juce::AudioChannelSet::stereo();
}

void TideTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    auto main = getBusBuffer(buffer,true,0);
    if (main.getNumChannels() < 1 || main.getNumSamples() < 1) return;
    const bool auxEnabled = getBusCount(true)>1 && getBus(true,1)->isEnabled();
    externalSidechainBusActive.store(auxEnabled,std::memory_order_relaxed);
    auto aux = auxEnabled ? getBusBuffer(buffer,true,1) : juce::AudioBuffer<float>();
    auto val=[this](size_t i) noexcept {
        return parameterRefs_[i]->load(std::memory_order_relaxed);
    };
    tidetune::Settings s;
    s.root=static_cast<int>(val(0));
    s.scale=static_cast<int>(val(1));
    s.retuneMs=val(2);
    s.strength=val(3)/100.0f;
    s.humanize=val(4)/100.0f;
    s.mix=val(5)/100.0f;
    s.bypass=val(6)>0.5f;
    s.vocalRange=static_cast<int>(val(7));
    s.hard=val(8)<0.5f;
    s.flex=val(9)/100.0f;
    s.vibrato=val(10)/100.0f;
    s.transpose=static_cast<int>(val(11));
    s.noteMask=0;
    for(int i=0;i<12;++i) if(val(static_cast<size_t>(12+i))>0.5f)
        s.noteMask|=static_cast<uint16_t>(1u<<i);

    tidetune::SidechainSettings sc;
    sc.enabled=val(24)>0.5f; sc.external=val(25)>0.5f;
    sc.target=static_cast<int>(val(26));
    sc.thresholdDb=val(27); sc.ratio=val(28); sc.attackMs=val(29);
    sc.releaseMs=val(30); sc.kneeDb=val(31); sc.amount=val(32)/100.0f;
    sc.hpfHz=val(33); sc.lpfHz=val(34);
    const float outputTarget=juce::Decibels::decibelsToGain(val(35));
    const float outSlew=1.0f-std::exp(-1.0f/(0.005f*static_cast<float>(sampleRate_)));

    const int mainCount=juce::jmin(2,main.getNumChannels());
    const int auxCount=juce::jmin(2,aux.getNumChannels());
    for(int offset=0;offset<main.getNumSamples();offset+=tidetune::SidechainEngine::maxChunk) {
        const int n=juce::jmin(tidetune::SidechainEngine::maxChunk,main.getNumSamples()-offset);
        float* out[2]={main.getWritePointer(0,offset),
            mainCount>1?main.getWritePointer(1,offset):nullptr};
        const float* input[2]={out[0],out[1]};
        const float* ext[2]={auxEnabled&&auxCount>0?aux.getReadPointer(0,offset):nullptr,
            auxEnabled&&auxCount>1?aux.getReadPointer(1,offset):nullptr};
        const float factor=sidechain.analyze(input,mainCount,auxEnabled?ext:nullptr,
            auxEnabled?auxCount:0,n,sc);
        tidetune::Settings mod=s;
        if(sc.enabled&&sc.target==1) mod.mix*=factor;
        if(sc.enabled&&sc.target==2) mod.strength*=factor;
        engine.process(out,mainCount,n,mod);
        if(sc.enabled&&sc.target==0) sidechain.applyGain(out,mainCount,n);
        for(int i=0;i<n;++i) {
            outputGainSmoothed_+=outSlew*(outputTarget-outputGainSmoothed_);
            for(int ch=0;ch<mainCount;++ch)out[ch][i]*=outputGainSmoothed_;
        }
    }
    sidechainReductionDb.store(sidechain.reductionDb(),std::memory_order_relaxed);
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
