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
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff223746),0.0f,0.0f,
                                           bg,1000.0f,750.0f,false));
    g.fillRect(getLocalBounds());
    g.setColour(border);
    g.drawRoundedRectangle(1.0f,1.0f,1218.0f,846.0f,14.0f,1.1f);
    juce::Path logo;
    logo.startNewSubPath(29.0f,43.0f);
    logo.cubicTo(44.0f,14.0f,55.0f,62.0f,76.0f,31.0f);
    logo.cubicTo(87.0f,19.0f,96.0f,33.0f,104.0f,30.0f);
    g.setColour(cyan);
    g.strokePath(logo,juce::PathStrokeType(5.0f,juce::PathStrokeType::curved));
    drawText(g,"TideTune",{120,16,227,50},32.0f,white,true);
    drawText(g,"Pro",{347,17,90,47},30.0f,cyan,true);
    g.setColour(border);g.drawVerticalLine(448,18.0f,65.0f);
    drawText(g,"VOCAL PITCH ENGINE",{467,23,220,35},12.0f,secondary);
    drawText(g,"V0.3 | LIVE",{1060,25,127,33},12.0f,cyan,true,
             juce::Justification::centredRight);

    drawPanel(g,{24.0f,84.0f,1172.0f,94.0f},11.0f);
    drawText(g,"INPUT RANGE",{44,94,182,23},11.0f,secondary,true);
    drawText(g,"KEY",{261,94,112,23},11.0f,secondary,true);
    drawText(g,"SCALE",{401,94,195,23},11.0f,secondary,true);
    drawText(g,"CORRECTION MODE",{641,94,189,23},11.0f,secondary,true);
    drawText(g,"SET YOUR SONG'S KEY AND SCALE",{873,100,267,20},10.0f,dim);

    drawPanel(g,{24.0f,193.0f,1172.0f,291.0f},13.0f);
    drawText(g,"LIVE VOCAL INPUT",{47,246,285,21},11.0f,secondary,true);
    drawText(g,displayedMidi<0?"--":noteName((int)std::round(displayedMidi)),
             {46,271,246,87},68.0f,white,true);
    drawText(g,displayedMidi<0?"WAITING FOR VOCAL":
             juce::String(displayedConfidence*100.0f,0)+"% PITCH CONFIDENCE",
             {52,350,245,23},12.0f,amber,true);
    drawText(g,"CORRECTED TARGET",{943,246,241,21},11.0f,secondary,true);
    drawText(g,noteName(displayedTarget),{945,271,240,87},68.0f,cyan,true);
    drawText(g,"MONOPHONIC • KEY-LOCKED",{949,350,240,23},11.0f,secondary,true);
    drawCenterMeter(g);

    drawPanel(g,{24.0f,494.0f,1172.0f,205.0f},13.0f);
    for(int i=1;i<4;++i) {
        g.setColour(border.withAlpha(0.65f));
        g.drawVerticalLine(24+i*293,507.0f,687.0f);
    }
    drawPanel(g,{24.0f,709.0f,1172.0f,119.0f},13.0f);
    drawText(g,"ALLOWED",{40,738,159,25},15.0f,white,true);
    drawText(g,"NOTES",{40,762,159,25},15.0f,cyan,true);
    drawText(g,"CLICK TO ENABLE",{40,794,160,18},10.0f,secondary);
    drawKeyboard(g);
    g.setColour(dim);
    g.drawHorizontalLine(836,24.0f,1194.0f);
}

void TideTuneAudioProcessorEditor::drawCenterMeter(juce::Graphics& g) {
    if(showPitchTrace_) { drawPitchTrace(g); return; }
    const float cx=607.0f,cy=336.0f,outer=111.0f;
    g.setColour(juce::Colour(0xff071119));
    g.fillEllipse(cx-outer-10,cy-outer-10,2*(outer+10),2*(outer+10));
    g.setColour(juce::Colour(0xff5b7a8c));
    g.drawEllipse(cx-outer-9,cy-outer-9,2*(outer+9),2*(outer+9),2.0f);
    for(int tick=0;tick<=100;++tick) {
        const float normalized=((float)tick-50.0f)/50.0f;
        const float a=normalized*2.12f,sn=std::sin(a),cs=std::cos(a);
        const float inner=outer-(tick%10==0?18.0f:11.0f);
        const bool lit=displayedMidi>=0.0f && std::abs(normalized*50.0f-displayedCents)<4.0f;
        g.setColour(lit?cyan:(tick%10==0?secondary:juce::Colour(0xff34596b)));
        g.drawLine(cx+sn*inner,cy-cs*inner,cx+sn*outer,cy-cs*outer,tick%10==0?2.1f:1.5f);
    }
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff182c3b),cx,cy-95,
                                           juce::Colour(0xff07121c),cx,cy+95,false));
    g.fillEllipse(cx-84,cy-84,168,168);
    g.setColour(juce::Colour(0xff294657));
    g.drawEllipse(cx-84,cy-84,168,168,1.3f);
    drawText(g,noteName(displayedTarget),{526,284,165,79},66.0f,white,true,
             juce::Justification::centred);
    const juce::String cents=displayedMidi>=0.0f?
          juce::String(displayedCents>=0?"+":"")+juce::String(displayedCents,1)+" c":"NO INPUT";
    drawText(g,cents,{542,355,131,38},24.0f,cyan,true,juce::Justification::centred);
    drawText(g,"CENTS",{555,394,106,24},11.0f,secondary,true,
             juce::Justification::centred);
    drawText(g,"-50",{466,331,51,24},11.0f,dim);
    drawText(g,"+50",{702,331,51,24},11.0f,dim);
    drawText(g,"0",{598,214,25,23},12.0f,white,true);
}

void TideTuneAudioProcessorEditor::drawPitchTrace(juce::Graphics& g) {
    const float x=390.0f,y=235.0f,w=433.0f,h=189.0f;
    g.setColour(juce::Colour(0xff091621));
    g.fillRoundedRectangle(x,y,w,h,9.0f);
    for(int n=0;n<5;++n) {
        const float py=y+(n+1)*h/6.0f;
        g.setColour(border.withAlpha(0.35f));
        g.drawHorizontalLine((int)py,x+8,x+w-8);
    }
    juce::Path trace;
    bool started=false;
    for(int n=0;n<(int)noteTrail.size();++n) {
        const float cents=noteTrail[(noteTrailIndex+n)%(int)noteTrail.size()];
        if(cents<-100.0f){started=false;continue;}
        const float px=x+10+n*((w-20)/(noteTrail.size()-1));
        const float py=y+0.5f*h-std::clamp(cents,-50.0f,50.0f)*(h*0.0082f);
        if(!started)trace.startNewSubPath(px,py);else trace.lineTo(px,py);
        started=true;
    }
    g.setColour(cyan);
    g.strokePath(trace,juce::PathStrokeType(2.1f,juce::PathStrokeType::curved));
    drawText(g,"LIVE CENTS TRACE  |  INPUT",{416,433,398,22},11.0f,secondary,true);
}

bool TideTuneAudioProcessorEditor::noteEnabled(int note) const {
    return processor.parameters.getRawParameterValue("note" + juce::String(note))->load() > 0.5f;
}

void TideTuneAudioProcessorEditor::drawKeyboard(juce::Graphics& g) {
    const int root=(int)processor.parameters.getRawParameterValue("root")->load();
    const int scale=(int)processor.parameters.getRawParameterValue("scale")->load();
    const int active=displayedTarget>=0?(displayedTarget%12+12)%12:-1;
    const char* whiteNoteNames[]{"C","D","E","F","G","A","B"};
    for(int octave=0;octave<2;++octave) {
        for(int pc=0;pc<12;++pc) {
            const int whiteIdx=whitePosition(pc);
            if(whiteIdx<0)continue;
            const int x=keyX+(octave*7+whiteIdx)*keyW;
            const bool enabled=noteEnabled(pc)&&tidetune::inScale(pc,root,scale);
            const bool target=pc==active;
            g.setGradientFill(juce::ColourGradient(
                target?cyan:(enabled?juce::Colour(0xff85dbe7):juce::Colour(0xffeef2f6)),
                (float)x,(float)keyY,
                target?juce::Colour(0xff21a9c9):juce::Colour(0xffd0dce5),
                (float)x,(float)(keyY+keyH),false));
            g.fillRoundedRectangle((float)x,(float)keyY,keyW-2.0f,(float)keyH,4.0f);
            g.setColour(target?cyan:juce::Colour(0xff344253));
            g.drawRoundedRectangle((float)x,(float)keyY,keyW-2.0f,(float)keyH,
                                   4.0f,target?2.5f:1.0f);
            drawText(g,juce::String(whiteNoteNames[whiteIdx])+juce::String(octave+3),
                     {x+4,keyY+68,keyW-10,18},12.0f,juce::Colour(0xff101a24),true,
                     juce::Justification::centred);
        }
    }
    for(int octave=0;octave<2;++octave) {
        for(int pc:{1,3,6,8,10}) {
            const int after=blackAfterWhite(pc);
            const int x=keyX+(octave*7+after+1)*keyW-21;
            const bool enabled=noteEnabled(pc)&&tidetune::inScale(pc,root,scale);
            const bool target=pc==active;
            g.setGradientFill(juce::ColourGradient(target?cyan:
                (enabled?juce::Colour(0xff263f52):juce::Colour(0xff0b111b)),
                (float)x,(float)keyY,
                target?blue:juce::Colour(0xff02070d),
                (float)x,(float)(keyY+56),false));
            g.fillRoundedRectangle((float)x,(float)keyY,42.0f,57.0f,4.0f);
            g.setColour(target?cyan:juce::Colour(0xff536675));
            g.drawRoundedRectangle((float)x,(float)keyY,42.0f,57.0f,4.0f,1.0f);
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
