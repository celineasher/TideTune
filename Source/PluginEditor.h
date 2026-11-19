#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

class TideLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    TideLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float position, float start, float end, juce::Slider&) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool, int, int,
                      int, int, juce::ComboBox&) override;
};

class TideTuneAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit TideTuneAudioProcessorEditor(TideTuneAudioProcessor&);
    ~TideTuneAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void setupDial(int index, const juce::String& text, const juce::String& id,
                   const juce::String& suffix);
    void drawCenterMeter(juce::Graphics&);
    void drawKeyboard(juce::Graphics&);
    void drawText(juce::Graphics&, const juce::String&, juce::Rectangle<int>,
                  float, juce::Colour, bool = false);
    void togglePitchClass(int);
    void applyPreset(int preset);
    bool noteEnabled(int) const;
    static juce::String noteName(int midi);

    TideTuneAudioProcessor& processor;
    TideLookAndFeel lookAndFeel;
    juce::ComboBox rootBox, scaleBox, rangeBox, characterBox;
    juce::ToggleButton bypassButton;
    std::array<juce::TextButton, 3> presetButtons;
    std::array<juce::Slider, 7> dials;
    std::array<juce::Label, 7> dialCaptions;
    std::array<std::unique_ptr<SliderAttachment>, 7> dialAttachments;
    std::unique_ptr<ComboAttachment> rootAttachment, scaleAttachment, rangeAttachment, characterAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    float displayedMidi = -1.0f;
    float displayedConfidence = 0.0f;
    float displayedCents = 0.0f;
    int displayedTarget = -1;
    float noteTrail[62] {};
    int noteTrailIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TideTuneAudioProcessorEditor)
};
