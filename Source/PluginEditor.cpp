#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
const juce::Colour bg(0xff070e15);
const juce::Colour surface(0xff111d29);
const juce::Colour surface2(0xff1b2c3a);
const juce::Colour border(0xff355064);
const juce::Colour blue(0xff32b9ec);
const juce::Colour cyan(0xff3aebf1);
const juce::Colour white(0xffeaf6fc);
const juce::Colour secondary(0xffa1b5c6);
const juce::Colour dim(0xff667e8f);
const juce::Colour amber(0xffffbd80);
constexpr int keyX=207, keyY=725, keyW=69, keyH=92;

int whitePosition(int pc) {
    constexpr int map[12] {0,-1,1,-1,2,3,-1,4,-1,5,-1,6};
    return map[pc];
}
int blackAfterWhite(int pc) {
    switch(pc) {case 1:return 0;case 3:return 1;case 6:return 3;
                case 8:return 4;case 10:return 5;default:return -1;}
}
}

TideLookAndFeel::TideLookAndFeel() {
    setColour(juce::ComboBox::backgroundColourId,surface2);
    setColour(juce::ComboBox::textColourId,white);
    setColour(juce::ComboBox::outlineColourId,border);
    setColour(juce::PopupMenu::backgroundColourId,surface);
    setColour(juce::PopupMenu::textColourId,white);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,blue);
    setColour(juce::PopupMenu::highlightedTextColourId,bg);
    setColour(juce::Slider::textBoxTextColourId,white);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff0b1720));
    setColour(juce::Slider::textBoxOutlineColourId,border);
    setColour(juce::ToggleButton::textColourId,white);
}

void TideLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
                                      float pos,float start,float end,juce::Slider&) {
    const float radius=std::min(w*0.44f,h*0.46f);
    const float cx=x+w*0.5f,cy=y+h*0.49f,a=start+pos*(end-start),r=radius-4.0f;
    g.setColour(juce::Colour(0xff050d14));
    g.fillEllipse(cx-radius-3,cy-radius-3,2*radius+6,2*radius+6);
    juce::Path inactive,active;
    inactive.addCentredArc(cx,cy,r,r,0.0f,start,end,true);
    active.addCentredArc(cx,cy,r,r,0.0f,start,a,true);
    g.setColour(juce::Colour(0xff344b5a));
    g.strokePath(inactive,juce::PathStrokeType(8.0f,juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(0xff135b6d));
    g.strokePath(active,juce::PathStrokeType(10.0f,juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
    g.setColour(cyan);
    g.strokePath(active,juce::PathStrokeType(6.0f,juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
    const float face=radius-16.0f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff586b7a),cx-face,cy-face,
                                           juce::Colour(0xff101c28),cx+face,cy+face,true));
    g.fillEllipse(cx-face,cy-face,2*face,2*face);
    g.setColour(juce::Colour(0xff83909c));
    g.drawEllipse(cx-face,cy-face,2*face,2*face,1.5f);
    const float needle=face*0.78f;
    g.setColour(white);
    g.drawLine(cx+std::sin(a)*needle*0.46f,cy-std::cos(a)*needle*0.46f,
               cx+std::sin(a)*needle,cy-std::cos(a)*needle,2.8f);
}
void TideLookAndFeel::drawComboBox(juce::Graphics& g,int width,int height,bool,
                                   int,int,int,int,juce::ComboBox&) {
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff253a4a),0,0,
                                           juce::Colour(0xff101b27),0,(float)height,false));
    g.fillRoundedRectangle(0.5f,0.5f,(float)width-1,(float)height-1,7.0f);
    g.setColour(border);
    g.drawRoundedRectangle(0.5f,0.5f,(float)width-1,(float)height-1,7.0f,1.0f);
    juce::Path chevron;
    const float cx=width-16.0f,cy=height*0.5f;
    chevron.startNewSubPath(cx-5,cy-2);
    chevron.lineTo(cx,cy+2);
    chevron.lineTo(cx+5,cy-2);
    g.setColour(cyan);
    g.strokePath(chevron,juce::PathStrokeType(1.6f));
}

TideTuneAudioProcessorEditor::TideTuneAudioProcessorEditor(TideTuneAudioProcessor& p)
    :juce::AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&lookAndFeel);
    rootBox.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},1);
    scaleBox.addItemList({"Chromatic","Major","Minor","Harmonic Minor",
                          "Major Pentatonic","Minor Pentatonic"},1);
    rangeBox.addItemList({"Low / Bass","Mid / Tenor","High / Soprano","Wide"},1);
    characterBox.addItemList({"HARD / Snap","SMOOTH"},1);
    for(juce::ComboBox* c:{&rootBox,&scaleBox,&rangeBox,&characterBox}) addAndMakeVisible(*c);
    auto& ps=processor.parameters;
    rootAttachment=std::make_unique<ComboAttachment>(ps,"root",rootBox);
    scaleAttachment=std::make_unique<ComboAttachment>(ps,"scale",scaleBox);
    rangeAttachment=std::make_unique<ComboAttachment>(ps,"range",rangeBox);
    characterAttachment=std::make_unique<ComboAttachment>(ps,"character",characterBox);
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::tickColourId,cyan);
    addAndMakeVisible(bypassButton);
    bypassAttachment=std::make_unique<ButtonAttachment>(ps,"bypass",bypassButton);

    setupDial(0,"RETUNE SPEED","speed"," ms");
    setupDial(1,"STRENGTH","strength"," %");
    setupDial(2,"FLEX TUNE","flex"," %");
    setupDial(3,"HUMANIZE","humanize"," %");
    setupDial(4,"VIBRATO","vibrato"," %");
    setupDial(5,"TRANSPOSE","transpose"," st");
    setupDial(6,"WET MIX","mix"," %");
    const juce::String presetNames[]{"HARD RAP","MELODIC","NATURAL"};
    for(int i=0;i<3;++i) {
        auto& btn=presetButtons[static_cast<size_t>(i)];
        btn.setButtonText(presetNames[i]);
        btn.setColour(juce::TextButton::buttonColourId,surface2);
        btn.setColour(juce::TextButton::textColourOffId,white);
        btn.onClick=[this,i]{applyPreset(i);};
        addAndMakeVisible(btn);
    }
    autoModeButton.setButtonText("AUTO MODE");
    traceModeButton.setButtonText("PITCH TRACE");
    clearTraceButton.setButtonText("CLEAR TRACE");
    autoModeButton.onClick=[this]{showPitchTrace_=false;repaint();};
    traceModeButton.onClick=[this]{showPitchTrace_=true;repaint();};
    clearTraceButton.onClick=[this]{noteTrail.fill(-150.0f);noteTrailIndex=0;repaint();};
    for(juce::TextButton* btn:{&autoModeButton,&traceModeButton,&clearTraceButton}) {
        btn->setColour(juce::TextButton::buttonColourId,surface2);
        btn->setColour(juce::TextButton::textColourOffId,white);
        addAndMakeVisible(*btn);
    }
    noteTrail.fill(-150.0f);
    setSize(1220,848);
    startTimerHz(30);
}
TideTuneAudioProcessorEditor::~TideTuneAudioProcessorEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void TideTuneAudioProcessorEditor::setupDial(int index,const juce::String& caption,
                                              const juce::String& id,const juce::String& suffix) {
    auto& knob=dials[static_cast<size_t>(index)];
    auto& lbl=dialCaptions[static_cast<size_t>(index)];
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow,false,105,25);
    knob.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,
                             juce::MathConstants<float>::pi*2.75f,true);
    knob.setTextValueSuffix(suffix);
    knob.setDoubleClickReturnValue(true,index==0?3.0:index==1||index==6?100.0:0.0);
    lbl.setText(caption,juce::dontSendNotification);
    lbl.setFont(juce::Font(juce::FontOptions(13.0f,juce::Font::bold)));
    lbl.setColour(juce::Label::textColourId,white);
    lbl.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(knob);
    addAndMakeVisible(lbl);
    dialAttachments[static_cast<size_t>(index)]=
        std::make_unique<SliderAttachment>(processor.parameters,id,knob);
}
void TideTuneAudioProcessorEditor::drawText(juce::Graphics& g,const juce::String& s,
          juce::Rectangle<int> rect,float fontSize,juce::Colour colour,bool isBold,
          juce::Justification justification) {
    g.setColour(colour);
    g.setFont(juce::Font(juce::FontOptions(fontSize,isBold?juce::Font::bold:juce::Font::plain)));
    g.drawFittedText(s,rect,justification,1);
}
void TideTuneAudioProcessorEditor::drawPanel(juce::Graphics& g,juce::Rectangle<float> rect,float radius) {
    g.setGradientFill(juce::ColourGradient(surface2,rect.getTopLeft(),surface,
                                           rect.getBottomRight(),false));
    g.fillRoundedRectangle(rect,radius);
    g.setColour(border.withAlpha(0.67f));
    g.drawRoundedRectangle(rect,radius,1.0f);
}
juce::String TideTuneAudioProcessorEditor::noteName(int midi) {
    if(midi<0)return "--";
    const char* n[]{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    return juce::String(n[(midi%12+12)%12])+juce::String(midi/12-1);
}

void TideTuneAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c2b3e), 180.0f, 0.0f,
                                            bg, 700.0f, 500.0f, false));
    g.fillRect(getLocalBounds());
    g.setColour(blue);
    g.fillRoundedRectangle(25.0f, 23.0f, 5.0f, 36.0f, 2.0f);
    drawText(g, "TIDE", {41, 15, 104, 53}, 30.0f, white, true);
    drawText(g, "TUNE", {138, 15, 112, 53}, 30.0f, cyan, true);
    drawText(g, "PITCH ENGINE / 02", {274, 25, 250, 30}, 11.0f, secondary);
    drawText(g, "PRESETS", {511, 25, 85, 30}, 10.0f, secondary);

    g.setColour(surface);
    g.fillRoundedRectangle(24.0f, 76.0f, 1042.0f, 84.0f, 13.0f);
    g.setColour(border);
    g.drawRoundedRectangle(24.0f, 76.0f, 1042.0f, 84.0f, 13.0f, 1.0f);
    drawText(g, "VOCAL RANGE", {41, 84, 160, 17}, 10.0f, secondary, true);
    drawText(g, "KEY", {252, 84, 110, 17}, 10.0f, secondary, true);
    drawText(g, "SCALE", {379, 84, 156, 17}, 10.0f, secondary, true);
    drawText(g, "CORRECTION", {624, 84, 190, 17}, 10.0f, secondary, true);
    drawText(g, "KEYBOARD: CLICK NOTES TO ENABLE / DISABLE", {38, 552, 615, 17}, 10.0f, secondary);
    drawText(g, "BLUE = TARGET   •   ORANGE = INPUT", {782, 552, 280, 17}, 10.0f, secondary);

    g.setColour(surface);
    g.fillRoundedRectangle(24.0f, 174.0f, 1042.0f, 232.0f, 16.0f);
    g.setColour(border);
    g.drawRoundedRectangle(24.0f, 174.0f, 1042.0f, 232.0f, 16.0f, 1.0f);
    drawCenterMeter(g);

    g.setColour(surface);
    g.fillRoundedRectangle(24.0f, 420.0f, 1042.0f, 127.0f, 14.0f);
    g.setColour(border);
    g.drawRoundedRectangle(24.0f, 420.0f, 1042.0f, 127.0f, 14.0f, 1.0f);
    drawKeyboard(g);
    drawText(g, "TIDE AUDIO / ORIGINAL DSP", {27, 665, 360, 16}, 10.0f, dim);
    drawText(g, "V0.2.1 • CORRECTION MODE / HARD", {811, 665, 256, 16}, 10.0f, secondary);
}

void TideTuneAudioProcessorEditor::drawCenterMeter(juce::Graphics& g) {
    drawText(g, "DETECTED", {49, 196, 220, 24}, 11.0f, secondary, true);
    drawText(g, "TARGET", {824, 196, 200, 24}, 11.0f, secondary, true);
    drawText(g, displayedMidi >= 0 ? noteName(static_cast<int>(std::round(displayedMidi))) : "--",
             {48, 229, 240, 86}, 75.0f, white, true);
    drawText(g, noteName(displayedTarget), {821, 229, 219, 86}, 75.0f, cyan, true);
    drawText(g, displayedMidi < 0 ? "WAITING FOR VOCAL" :
             juce::String((displayedMidi - std::round(displayedMidi)) * 100.0f, 1) + " cents / input",
             {49, 335, 235, 24}, 12.0f, secondary);
    drawText(g, displayedTarget < 0 ? "NO PITCH LOCK" :
             juce::String(static_cast<int>(displayedConfidence * 100)) + "% TRACKING CONFIDENCE",
             {821, 335, 233, 24}, 12.0f, blue);

    const float cx = 546.0f;
    const float cy = 289.0f;
    const float outerRadius = 93.0f;
    for (int i = 0; i <= 80; ++i) {
        const float v = (static_cast<float>(i) / 80.0f) * 2.0f - 1.0f;
        const float a = v * 2.13f;
        const float r0 = outerRadius - (i % 10 == 0 ? 17.0f : 9.0f);
        const float s = std::sin(a), c = std::cos(a);
        const bool active = displayedMidi >= 0 && std::abs(v * 100.0f - displayedCents) < 8.0f;
        g.setColour(active ? cyan : (std::abs(v) < 0.04f ? white : juce::Colour(0xff3e536c)));
        g.drawLine(cx + s * r0, cy - c * r0,
                   cx + s * outerRadius, cy - c * outerRadius, i % 10 == 0 ? 2.0f : 1.4f);
    }
    g.setColour(juce::Colour(0xff121a25));
    g.fillEllipse(cx - 65.0f, cy - 65.0f, 130.0f, 130.0f);
    g.setColour(border);
    g.drawEllipse(cx - 65.0f, cy - 65.0f, 130.0f, 130.0f, 1.0f);
    g.setColour(blue);
    g.setFont(juce::Font(juce::FontOptions(47.0f, juce::Font::bold)));
    g.drawFittedText(displayedTarget >= 0 ? noteName(displayedTarget) : "--",
                     juce::Rectangle<int>(480, 247, 132, 73), juce::Justification::centred, 1);
    g.setColour(secondary);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::plain)));
    g.drawFittedText("CORRECTED NOTE", {486, 317, 120, 18}, juce::Justification::centred, 1);
    drawText(g, "-100", {416, 375, 55, 18}, 10.0f, dim);
    drawText(g, "0", {544, 378, 33, 18}, 10.0f, secondary);
    drawText(g, "+100", {627, 375, 53, 18}, 10.0f, dim);
    // Pitch history, showing tracked input trends in the lower left of panel.
    g.setColour(juce::Colour(0xff293747));
    g.drawLine(40.0f, 389.0f, 342.0f, 389.0f, 1.0f);
    juce::Path trace;
    bool started = false;
    for (int i = 0; i < 62; ++i) {
        const float value = noteTrail[(noteTrailIndex + i) % 62];
        if (value < 0.0f) { started = false; continue; }
        const float x = 40.0f + i * (302.0f / 61.0f);
        const float y = 383.0f - std::clamp(value, -100.0f, 100.0f) * 0.19f;
        if (!started) trace.startNewSubPath(x, y);
        else trace.lineTo(x, y);
        started = true;
    }
    g.setColour(amber);
    g.strokePath(trace, juce::PathStrokeType(1.7f));
}

bool TideTuneAudioProcessorEditor::noteEnabled(int note) const {
    return processor.parameters.getRawParameterValue("note" + juce::String(note))->load() > 0.5f;
}

void TideTuneAudioProcessorEditor::drawKeyboard(juce::Graphics& g) {
    const int root = static_cast<int>(processor.parameters.getRawParameterValue("root")->load());
    const int scale = static_cast<int>(processor.parameters.getRawParameterValue("scale")->load());
    const int inputClass = displayedMidi >= 0 ?
        (static_cast<int>(std::round(displayedMidi)) % 12 + 12) % 12 : -1;
    const int targetClass = displayedTarget >= 0 ? (displayedTarget % 12 + 12) % 12 : -1;

    // 2 octaves of piano keys. Clicking either octave toggles that pitch class globally.
    for (int octave = 0; octave < 2; ++octave) {
        const int offset = octave * 7;
        for (int note = 0; note < 12; ++note) {
            const int whiteIndex = whitePosition(note);
            if (whiteIndex < 0) continue;
            const int x = keyX + (offset + whiteIndex) * keyW;
            const bool usable = noteEnabled(note) && tidetune::inScale(note, root, scale);
            const bool target = targetClass == note;
            const bool input = inputClass == note;
            g.setColour(target ? blue : usable ? juce::Colour(0xffdfe6ef) : juce::Colour(0xff627083));
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(keyY),
                                   static_cast<float>(keyW - 2), static_cast<float>(keyH), 4.0f);
            g.setColour(juce::Colour(0xff0a111b));
            g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(keyY),
                                   static_cast<float>(keyW - 2), static_cast<float>(keyH), 4.0f, 1.0f);
            if (input) { g.setColour(amber); g.fillEllipse(static_cast<float>(x + keyW / 2 - 4),
                                                          static_cast<float>(keyY + 52), 8.0f, 8.0f); }
            if (whiteIndex == 0) drawText(g, octave == 0 ? "C3" : "C4",
                                          {x + 9, keyY + 65, 34, 13}, 10.0f, juce::Colour(0xff142031));
        }
    }
    for (int octave = 0; octave < 2; ++octave) {
        const int offset = octave * 7;
        for (int note = 0; note < 12; ++note) {
            const int after = blackAfterWhite(note);
            if (after < 0) continue;
            const int x = keyX + (offset + after + 1) * keyW - 24;
            const bool usable = noteEnabled(note) && tidetune::inScale(note, root, scale);
            const bool target = targetClass == note;
            const bool input = inputClass == note;
            g.setColour(target ? blue : usable ? juce::Colour(0xff29384c) : juce::Colour(0xff10161d));
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(keyY), 46.0f, 50.0f, 3.0f);
            g.setColour(juce::Colour(0xff070b12));
            g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(keyY), 46.0f, 50.0f, 3.0f, 1.0f);
            if (input) { g.setColour(amber); g.fillEllipse(static_cast<float>(x + 19),
                                                          static_cast<float>(keyY + 35), 8.0f, 8.0f); }
        }
    }
}

void TideTuneAudioProcessorEditor::applyPreset(int preset) {
    auto setParam = [this](const char* id, float plain) {
        if (auto* param = processor.parameters.getParameter(id)) {
            // Converts from real units to the APVTS normalized parameter value.
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(plain));
            param->endChangeGesture();
        }
    };
    if (preset == 0) {          // hard robotized rap vocal
        setParam("character", 0.0f);
        setParam("speed", 0.0f);
        setParam("strength", 100.0f);
        setParam("flex", 0.0f);
        setParam("humanize", 0.0f);
        setParam("vibrato", 0.0f);
        setParam("mix", 100.0f);
    } else if (preset == 1) {   // melodic rap lead, still audible tuning
        setParam("character", 0.0f);
        setParam("speed", 7.0f);
        setParam("strength", 100.0f);
        setParam("flex", 12.0f);
        setParam("humanize", 10.0f);
        setParam("vibrato", 0.0f);
        setParam("mix", 100.0f);
    } else {                    // transparent vocal correction
        setParam("character", 1.0f);
        setParam("speed", 42.0f);
        setParam("strength", 85.0f);
        setParam("flex", 55.0f);
        setParam("humanize", 55.0f);
        setParam("vibrato", 0.0f);
        setParam("mix", 100.0f);
    }
    repaint();
}

void TideTuneAudioProcessorEditor::togglePitchClass(int pc) {
    if (pc < 0 || pc >= 12) return;
    auto* parameter = processor.parameters.getParameter("note" + juce::String(pc));
    if (parameter == nullptr) return;
    const bool on = noteEnabled(pc);
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(on ? 0.0f : 1.0f);
    parameter->endChangeGesture();
    repaint();
}

void TideTuneAudioProcessorEditor::mouseDown(const juce::MouseEvent& e) {
    const auto p = e.getPosition();
    if (p.y >= keyY && p.y < keyY + keyH && p.x >= keyX && p.x < keyX + 14 * keyW) {
        const int octaveFromX = std::clamp((p.x - keyX) / (7 * keyW), 0, 1);
        const int baseX = keyX + octaveFromX * 7 * keyW;
        if (p.y < keyY + 50) {
            for (int semitone : {1, 3, 6, 8, 10}) {
                const int pos = blackAfterWhite(semitone);
                const int x = baseX + (pos + 1) * keyW - 24;
                if (p.x >= x && p.x < x + 46) { togglePitchClass(semitone); return; }
            }
        }
        const int whiteIndex = std::clamp((p.x - baseX) / keyW, 0, 6);
        const int whiteToSemitone[7] {0, 2, 4, 5, 7, 9, 11};
        togglePitchClass(whiteToSemitone[whiteIndex]);
    }
}

void TideTuneAudioProcessorEditor::resized() {
    rangeBox.setBounds(40, 108, 195, 38);
    rootBox.setBounds(252, 108, 111, 38);
    scaleBox.setBounds(379, 108, 231, 38);
    characterBox.setBounds(624, 108, 209, 38);
    bypassButton.setBounds(925, 108, 113, 38);
    for (int i = 0; i < 3; ++i)
        presetButtons[static_cast<size_t>(i)].setBounds(589 + i * 153, 22, 144, 39);
    for (int i = 0; i < 7; ++i) {
        const int x = 35 + i * 147;
        dials[static_cast<size_t>(i)].setBounds(x, 426, 140, 95);
        dialCaptions[static_cast<size_t>(i)].setBounds(x, 519, 140, 22);
    }
}

void TideTuneAudioProcessorEditor::timerCallback() {
    displayedMidi = processor.engine.detectedMidi.load(std::memory_order_relaxed);
    displayedTarget = processor.engine.targetMidi.load(std::memory_order_relaxed);
    displayedConfidence = processor.engine.confidence.load(std::memory_order_relaxed);
    displayedCents = displayedMidi >= 0.0f ?
        std::clamp((displayedMidi - std::round(displayedMidi)) * 100.0f, -100.0f, 100.0f) : 0.0f;
    noteTrail[noteTrailIndex] = displayedMidi >= 0 ? displayedCents : -150.0f;
    noteTrailIndex = (noteTrailIndex + 1) % 62;
    repaint();
}
