#include "PluginEditor.h"

namespace {
const juce::Colour ink{0xffc4c4b7},line{0xff92958a};
juce::Font lettering(float size,bool bold=true) {
    return juce::Font(juce::FontOptions(juce::Font::getDefaultSerifFontName(),size,bold ? juce::Font::bold : juce::Font::plain));
}
void text(juce::Graphics& g,const juce::String& value,float x,float y,float w,float h,float size,
          juce::Justification justification=juce::Justification::centred) {
    g.setFont(lettering(size));
    g.setColour(juce::Colours::black.withAlpha(.45f));
    g.drawText(value,juce::Rectangle<float>(x+1,y+1,w,h),justification);
    g.setColour(ink); g.drawText(value,juce::Rectangle<float>(x,y,w,h),justification);
}
void configure(juce::Slider& s,juce::Component* parent) {
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    s.setRotaryParameters(float(kbass::pi*1.25),float(kbass::pi*2.75),true);
    s.setMouseDragSensitivity(420);
    s.setPopupDisplayEnabled(true,true,parent);
    s.setWantsKeyboardFocus(false);
}
}

double SmoothDial::proportionOfLengthToValue(double proportion) {
    if(fineCutoff && getMinimum()>0)
        return getMinimum()*std::exp(std::log(getMaximum()/getMinimum())*juce::jlimit(0.0,1.0,proportion));
    return juce::Slider::proportionOfLengthToValue(proportion);
}
double SmoothDial::valueToProportionOfLength(double value) {
    if(fineCutoff && getMinimum()>0)
        return std::log(juce::jlimit(getMinimum(),getMaximum(),value)/getMinimum())/std::log(getMaximum()/getMinimum());
    return juce::Slider::valueToProportionOfLength(value);
}
void SmoothDial::mouseDown(const juce::MouseEvent& e) {
    actualPosition=virtualPosition=e.position;
    juce::Slider::mouseDown(e);
}
void SmoothDial::mouseDrag(const juce::MouseEvent& e) {
    const float scale=e.mods.isShiftDown() && !bool(getProperties()["selector"]) ? .1f : 1.0f;
    virtualPosition+=(e.position-actualPosition)*scale;
    actualPosition=e.position;
    // Use JUCE's drag lifecycle to keep popup values, mouse capture and host
    // automation gestures balanced, even when Shift is changed mid-drag.
    juce::Slider::mouseDrag(e.withNewPosition(virtualPosition));
}
void SmoothDial::mouseUp(const juce::MouseEvent& e) { juce::Slider::mouseUp(e.withNewPosition(virtualPosition)); }
void SmoothDial::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& wheel) {
    auto adjusted=wheel;
    const float scale=(fineCutoff ? .04f : .2f)*(e.mods.isShiftDown() ? .1f : 1.0f);
    adjusted.deltaX*=scale; adjusted.deltaY*=scale;
    juce::Slider::mouseWheelMove(e,adjusted);
}
bool SmoothDial::keyPressed(const juce::KeyPress& key) {
    const int code=key.getKeyCode();
    if(fineCutoff && isEnabled() && !key.getModifiers().isCtrlDown() && !key.getModifiers().isAltDown()
       && !key.getModifiers().isCommandDown() && (code==juce::KeyPress::upKey || code==juce::KeyPress::rightKey
       || code==juce::KeyPress::downKey || code==juce::KeyPress::leftKey)) {
        const double direction=(code==juce::KeyPress::upKey || code==juce::KeyPress::rightKey) ? 1.0 : -1.0;
        const juce::Slider::ScopedDragNotification gesture(*this);
        setValue(getValue()+direction*(key.getModifiers().isShiftDown() ? .1 : 1.0),juce::sendNotificationSync);
        return true;
    }
    return juce::Slider::keyPressed(key);
}

HardwareLook::HardwareLook():chrome(juce::Image::ARGB,256,256,true) {
    setColour(juce::TooltipWindow::backgroundColourId,juce::Colour(0xff202220));
    setColour(juce::TooltipWindow::textColourId,ink);
    setColour(juce::TooltipWindow::outlineColourId,line);
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff282b28));
    setColour(juce::PopupMenu::textColourId,ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff545b50));
    setColour(juce::ComboBox::textColourId,ink);
    setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff252923));
    setColour(juce::AlertWindow::backgroundColourId,juce::Colour(0xff33382f));
    setColour(juce::AlertWindow::textColourId,ink);
    setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff20241e));
    setColour(juce::TextEditor::textColourId,ink);
    setColour(juce::TextEditor::outlineColourId,line);
    for(int y=0;y<256;++y) for(int x=0;x<256;++x) {
        const double dx=(x-127.5)/127.5,dy=(y-127.5)/127.5,r=std::hypot(dx,dy);
        if(r>1) continue;
        const double a=std::atan2(dy,dx);
        const double cone=139+63*std::cos(2*a+.5)+26*std::sin(3*a-.8)-16*dy;
        const double edge=r>.94 ? 30*(1-r)/.06-17 : 0;
        const int shade=juce::jlimit(36,238,int(cone+edge+3*std::sin(r*360)));
        chrome.setPixelAt(x,y,juce::Colour::fromRGB(juce::uint8(shade),juce::uint8(shade+1),juce::uint8(std::min(255,shade+2))));
    }
    juce::Graphics scuffs(chrome);
    juce::Path face; face.addEllipse(2,2,252,252); scuffs.reduceClipRegion(face);
    juce::Random wear(0x4348524f);
    for(int i=0;i<110;++i) {
        const float x=25+wear.nextFloat()*200,y=25+wear.nextFloat()*200;
        const float length=8+wear.nextFloat()*48;
        scuffs.setColour(juce::Colours::black.withAlpha(.07f+wear.nextFloat()*.13f));
        scuffs.drawLine(x,y,x+length,y-3+wear.nextFloat()*6,1.4f+wear.nextFloat());
        scuffs.setColour(juce::Colour(0xffe1dfce).withAlpha(.055f+wear.nextFloat()*.06f));
        scuffs.drawLine(x,y+1.4f,x+length,y+1.1f,.9f);
    }
    for(int i=0;i<5;++i) {
        const float x=40+wear.nextFloat()*175,y=40+wear.nextFloat()*175,radius=15+wear.nextFloat()*45;
        scuffs.setGradientFill(juce::ColourGradient(juce::Colour(0xff292419).withAlpha(.07f),x,y,
            juce::Colours::transparentBlack,x+radius,y,true));
        scuffs.fillEllipse(x-radius,y-radius,radius*2,radius*2);
    }
}
void HardwareLook::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float pos,float,float,juce::Slider& slider) {
    const bool selector=bool(slider.getProperties()["selector"]);
    const bool large=bool(slider.getProperties()["large"]);
    const auto bounds=juce::Rectangle<float>(float(x),float(y),float(width),float(height)).reduced(selector ? 6.0f : 9.0f);
    const float r=std::min(bounds.getWidth(),bounds.getHeight())*.5f;
    const auto c=bounds.getCentre();
    if(!selector) {
        const float tickRadius=r+4;
        const int steps=large ? 60 : 10;
        for(int n=0;n<=steps;++n) {
            const float a=float(kbass::pi*1.25+kbass::pi*1.5*double(n)/steps);
            const float length=large ? (n%5==0 ? 7.0f : 3.5f) : 5.0f;
            g.setColour(juce::Colour(0xff171a17));
            g.drawLine(c.x+std::sin(a)*tickRadius,c.y-std::cos(a)*tickRadius,
                c.x+std::sin(a)*(tickRadius+length),c.y-std::cos(a)*(tickRadius+length),large ? 1.8f : 2.0f);
        }
    }
    const auto body=juce::Rectangle<float>(c.x-r,c.y-r,r*2,r*2);
    juce::Path shadow; shadow.addEllipse(body);
    juce::DropShadow(juce::Colours::black.withAlpha(.85f),selector ? 6 : 9,{4,6}).drawForPath(g,shadow);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffa6a8a1),c.x-r*.5f,c.y-r,
        juce::Colour(0xff080a09),c.x+r*.4f,c.y+r,false));
    g.fillEllipse(body);
    g.setColour(juce::Colour(0xff141714)); g.fillEllipse(body.reduced(3));
    if(!selector) {
        for(int n=0;n<28;++n) {
            const float a=float(n*2*kbass::pi/28);
            g.setColour(n<14 ? juce::Colour(0xff333731) : juce::Colour(0xff060807));
            g.drawLine(c.x+std::sin(a)*(r-5),c.y+std::cos(a)*(r-5),
                c.x+std::sin(a)*(r-1),c.y+std::cos(a)*(r-1),2.6f);
        }
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff41463e),c.x-r*.4f,c.y-r,
            juce::Colours::black,c.x+r*.3f,c.y+r,false));
        g.fillEllipse(body.reduced(6));
        g.setColour(juce::Colours::black); g.fillEllipse(body.reduced(10));
        // Scuffed edges on the black coating, varied by control rather than
        // flickering or changing position when the knob is redrawn.
        juce::Random rimWear(juce::int64(slider.getName().hashCode()));
        for(int n=0;n<11;++n) {
            const float a=rimWear.nextFloat()*float(kbass::pi*2),rad=r-3-rimWear.nextFloat()*4;
            const float px=c.x+std::sin(a)*rad,py=c.y-std::cos(a)*rad;
            g.setColour(juce::Colour(0xff9b9d8b).withAlpha(.13f+rimWear.nextFloat()*.2f));
            g.drawLine(px,py,px+1+rimWear.nextFloat()*2.5f,py-rimWear.nextFloat()*1.8f,.75f);
        }
    }
    const float capRadius=selector ? r*.84f : r*(large ? .57f : .53f);
    g.setOpacity(1.0f);
    g.drawImage(chrome,juce::Rectangle<float>(c.x-capRadius,c.y-capRadius,capRadius*2,capRadius*2));
    g.setColour(juce::Colour(0xff0b0d0c)); g.drawEllipse(c.x-capRadius,c.y-capRadius,capRadius*2,capRadius*2,1.1f);
    const float angle=selector ? 5.42f-pos*1.42f : float(kbass::pi*1.25+pos*kbass::pi*1.5);
    const float outer=selector ? r+1 : r-4;
    const float inner=selector ? r*.76f : r*.76f;
    g.setColour(juce::Colours::black.withAlpha(.65f));
    g.drawLine(c.x+std::sin(angle)*inner+1,c.y-std::cos(angle)*inner+1,
        c.x+std::sin(angle)*outer+1,c.y-std::cos(angle)*outer+1,4);
    g.setColour(juce::Colour(0xffdfdfce));
    g.drawLine(c.x+std::sin(angle)*inner,c.y-std::cos(angle)*inner,
        c.x+std::sin(angle)*outer,c.y-std::cos(angle)*outer,selector ? 3.0f : 3.8f);
}
void HardwareLook::drawToggleButton(juce::Graphics& g,juce::ToggleButton& button,bool hover,bool) {
    const float x=button.getWidth()*.5f;
    g.setColour(juce::Colour(0xff141714)); g.fillRoundedRectangle(x-5,0,10,float(button.getHeight()),4);
    g.setColour(juce::Colour(0xff777c6f)); g.drawLine(x-6,2,x-6,float(button.getHeight()-2));
    const float y=button.getToggleState() ? float(button.getHeight()-29) : 1;
    juce::Rectangle<float> handle(x-13,y,26,28);
    g.setGradientFill(juce::ColourGradient(juce::Colour(hover ? 0xff777b70 : 0xff54594e),0,y,
        juce::Colour(0xff0b0d0b),0,y+28,false));
    g.fillRoundedRectangle(handle,3); g.setColour(juce::Colours::black); g.drawRoundedRectangle(handle,3,1.5f);
    for(int i=0;i<5;++i) {
        g.setColour(juce::Colour(0xff10130f)); g.drawLine(x-9,y+5+i*4,x+9,y+5+i*4,1.3f);
        g.setColour(juce::Colour(0xff707568).withAlpha(.45f)); g.drawLine(x-9,y+6+i*4,x+9,y+6+i*4);
    }
}
void HardwareLook::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool hover,bool down) {
    const auto r=b.getLocalBounds().toFloat().reduced(1);
    juce::Path p; p.addRoundedRectangle(r,3);
    juce::DropShadow(juce::Colours::black.withAlpha(.65f),3,{1,2}).drawForPath(g,p);
    g.setGradientFill(juce::ColourGradient(juce::Colour(down ? 0xff747c6c : 0xffb5b9a9),0,0,
        juce::Colour(hover ? 0xff939b88 : 0xff747c6b),0,r.getHeight(),false));
    g.fillRoundedRectangle(r,3);
    g.setColour(juce::Colour(0xffe3e2d0).withAlpha(.5f)); g.drawRoundedRectangle(r.reduced(1),3,1);
}
void HardwareLook::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) {
    g.setColour(juce::Colour(0xff20261e)); g.setFont(juce::Font(juce::FontOptions(13,juce::Font::bold)));
    g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(6,2),juce::Justification::centred,1);
}
void HardwareLook::drawComboBox(juce::Graphics& g,int width,int height,bool down,int,int,int,int,juce::ComboBox&) {
    const auto r=juce::Rectangle<float>(.5f,.5f,float(width)-1,float(height)-1);
    g.setGradientFill(juce::ColourGradient(juce::Colour(down ? 0xff1c211a : 0xff232820),0,0,juce::Colour(0xff3c4237),0,float(height),false));
    g.fillRoundedRectangle(r,3);
    g.setColour(line.withAlpha(.55f)); g.drawRoundedRectangle(r,3,1);
    const float x=float(width)-15,y=float(height)*.5f;
    juce::Path arrow; arrow.startNewSubPath(x-4,y-2); arrow.lineTo(x,y+2); arrow.lineTo(x+4,y-2);
    g.setColour(ink); g.strokePath(arrow,juce::PathStrokeType(1.4f));
}
juce::Font HardwareLook::getComboBoxFont(juce::ComboBox&) { return juce::Font(juce::FontOptions(13)); }

KBassEditor::KBassEditor(KBassProcessor& p):AudioProcessorEditor(p),processor(p) {
    setLookAndFeel(&hardware); setOpaque(true); setWantsKeyboardFocus(false);
    addDial("wave1","OSC1 WAVE","Oscillator 1: SAW / SQUARE / SINE",true);
    addDial("wave2","OSC2 WAVE","Oscillator 2: SAW / SQUARE / SINE",true);
    addDial("octave","OSC2 OCTAVE","Oscillator 2: normal, one octave down, two octaves down",true);
    addDial("detune","DETUNE","Oscillator 2 fine tuning, -50 to +50 cents");
    addDial("glide","GLIDE","Legato pitch glide between overlapping MIDI notes");
    addDial("pitchLfo","OSC LFO","LFO pitch modulation of both oscillators, 0 to 2 semitones");
    addDial("mix","MIX","Oscillator blend: left = OSC1, right = OSC2");
    addDial("lfoWave","LFO WAVE","LFO: SAW / TRIANGLE / SQUARE",true);
    addDial("lfoSpeed","LFO SPEED","LFO speed, 0.03 to 30 Hz");
    addDial("cutoff","CUTOFF","24 dB/oct low-pass cutoff, 20 Hz to 18 kHz. Shift-drag for fine changes.",false,true);
    addDial("resonance","RESONANCE","Filter resonance");
    addDial("filterEnv","FILTER ENV","Filter envelope depth, 0 to 6 octaves");
    addDial("filterLfo","FILTER LFO","Filter LFO depth, 0 to 4 octaves");
    addDial("volume","VOLUME","Master output level after saturation; fully left mutes the instrument");
    addDial("saturation","SATURATION","Drive and blend: 0% bypass, 100% full character. Volume adjusts the final level.");
    addDial("saturationType","SATURATION TYPE","Saturation character: TUBE / TAPE / TRANSISTOR",true);
    for(size_t i=0;i<envelope.size();++i) {
        configure(envelope[i],this); envelope[i].setName(juce::StringArray{"ATTACK","DECAY","SUSTAIN","RELEASE"}[int(i)]);
        addAndMakeVisible(envelope[i]);
    }
    envelopeTarget.setName("AMP / FILT"); envelopeTarget.setWantsKeyboardFocus(false);
    envelopeTarget.setTooltip("Choose which independent envelope the four knobs edit. Both envelopes continue working.");
    addAndMakeVisible(envelopeTarget);
    targetAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"envTarget",envelopeTarget);
    envelopeTarget.onClick=[this] { bindEnvelope(); };
    factoryPresets.setName("FACTORY PRESETS"); factoryPresets.setComponentID("factoryPresets");
    userPresets.setName("USER PRESETS"); userPresets.setComponentID("userPresets");
    factoryPresets.setWantsKeyboardFocus(false); userPresets.setWantsKeyboardFocus(false);
    factoryPresets.setTooltip("Choose a factory preset"); userPresets.setTooltip("Choose one of your saved sounds");
    factoryPresets.setTextWhenNothingSelected("Select factory preset");
    userPresets.setTextWhenNothingSelected("No user preset selected");
    factoryPresets.onChange=[this] {
        const int id=factoryPresets.getSelectedId();
        if(id>=1 && id<=processor.getNumPrograms()) { processor.setCurrentProgram(id-1); syncPresetSelection(); }
    };
    userPresets.beforePopup=[this] { refreshPresetMenus(); };
    userPresets.onChange=[this] {
        const int index=userPresets.getSelectedId()-1;
        if(juce::isPositiveAndBelow(index,int(userPresetFiles.size()))) {
            const auto result=processor.loadUserPreset(userPresetFiles[size_t(index)].file);
            if(result.failed()) showPresetError(result.getErrorMessage());
            syncPresetSelection();
        }
    };
    savePreset.setName("SAVE PRESET"); savePreset.setComponentID("savePreset");
    savePreset.setWantsKeyboardFocus(false); savePreset.setTooltip("Save the current sound as a user preset");
    savePreset.onClick=[this] { beginSavePreset(); };
    addAndMakeVisible(factoryPresets); addAndMakeVisible(userPresets); addAndMakeVisible(savePreset);
    refreshPresetMenus();
    bindEnvelope(); makeTexture(); setSize(1108,780); startTimerHz(20);
}
KBassEditor::~KBassEditor() {
    stopTimer(); envelopeTarget.onClick=nullptr; savePreset.onClick=nullptr;
    factoryPresets.onChange=nullptr; userPresets.onChange=nullptr; userPresets.beforePopup=nullptr;
    if(presetDialog) { presetDialog->setLookAndFeel(nullptr); presetDialog->exitModalState(0); }
    setLookAndFeel(nullptr);
}
void KBassEditor::addDial(const char* id,const char* name,const char* help,bool selector,bool large) {
    Dial dial; dial.id=id; dial.slider=std::make_unique<SmoothDial>();
    auto& s=*dial.slider; configure(s,this); s.setName(name); s.setComponentID(id); s.setTooltip(help);
    s.getProperties().set("selector",selector); s.getProperties().set("large",large);
    addAndMakeVisible(s);
    dial.attachment=std::make_unique<Attachment>(processor.parameters,id,s);
    if(dial.id=="saturation") {
        s.textFromValueFunction=[](double value) { return juce::String(value*100,1)+" %"; };
        s.valueFromTextFunction=[](const juce::String& value) { return value.getDoubleValue()/100; };
    }
    s.setDoubleClickReturnValue(true,processor.parameters.getParameter(id)->convertFrom0to1(processor.parameters.getParameter(id)->getDefaultValue()));
    if(selector) s.setMouseDragSensitivity(100);
    if(dial.id=="cutoff") {
        static_cast<SmoothDial&>(s).enableFineCutoff();
        s.textFromValueFunction=[](double value) { return juce::String(value,value<100 ? 2 : 1)+" Hz"; };
        s.valueFromTextFunction=[](const juce::String& value) { return value.getDoubleValue()*(value.containsIgnoreCase("kHz") ? 1000 : 1); };
    }
    dials.push_back(std::move(dial));
}
juce::Slider* KBassEditor::findControl(const juce::String& id) {
    for(auto& d:dials) if(d.id==id) return d.slider.get();
    for(auto& s:envelope) if(s.getComponentID()==id) return &s;
    return nullptr;
}
void KBassEditor::selectEnvelope(bool filter) {
    auto* parameter=processor.parameters.getParameter("envTarget");
    parameter->beginChangeGesture(); parameter->setValueNotifyingHost(filter ? 1.0f : 0.0f); parameter->endChangeGesture();
    bindEnvelope();
}
void KBassEditor::bindEnvelope() {
    const int target=int(processor.parameters.getRawParameterValue("envTarget")->load());
    if(displayedEnvelope==target) return;
    for(auto& attachment:envelopeAttachments) attachment.reset();
    const char* suffixes[]={"Attack","Decay","Sustain","Release"};
    for(size_t i=0;i<envelope.size();++i) {
        const auto id=juce::String(target==0 ? "amp" : "filter")+suffixes[i];
        envelope[i].setComponentID(id);
        envelope[i].setTooltip(juce::String(target==0 ? "Amplitude " : "Filter ")+suffixes[i]+". AMP and FILT retain separate settings.");
        envelopeAttachments[i]=std::make_unique<Attachment>(processor.parameters,id,envelope[i]);
        auto* parameter=processor.parameters.getParameter(id);
        envelope[i].setDoubleClickReturnValue(true,parameter->convertFrom0to1(parameter->getDefaultValue()));
    }
    displayedEnvelope=target; repaint(67,294,562,182);
}
void KBassEditor::timerCallback() {
    bindEnvelope();
    syncPresetSelection();
    const float level=processor.peak.load(std::memory_order_relaxed);
    const float next=std::max(level,meter*.85f);
    if(std::abs(next-meter)>.001f) { meter=next; repaint(920,290,115,95); }
}
void KBassEditor::makeTexture() {
    texture=juce::Image(juce::Image::RGB,1108,780,false);
    juce::Random random(0x4b42617373);
    for(int y=0;y<780;++y) for(int x=0;x<1108;++x) {
        if(x<48 || x>=1060) {
            const double grain=std::sin(x*.63+std::sin(y*.014)*1.3)+.5*std::sin(x*1.9+y*.0012)+.23*std::sin(x*4.2);
            const double edge=std::min(x<48 ? std::min(x,47-x) : std::min(x-1060,1107-x),6)/6.0;
            const float value=float((.69+.20*grain)*(.45+.55*edge));
            texture.setPixelAt(x,y,juce::Colour::fromFloatRGBA(.43f*value,.225f*value,.13f*value,1));
        } else {
            const double nx=(x-554)/540.0,ny=(y-390)/430.0;
            const double vignette=15*(nx*nx+ny*ny);
            const double brushed=1.1*std::sin(x*.91)+.6*std::sin(x*2.7);
            const double patina=-4.0-11.0*std::sin(x*.013+std::sin(y*.009))*std::sin(y*.016-x*.004)
                -7.0*std::cos(x*.031-y*.012+std::sin(y*.019));
            const int shade=juce::jlimit(30,112,int(86-vignette+brushed+patina+random.nextInt(14)-7));
            texture.setPixelAt(x,y,juce::Colour::fromRGB(juce::uint8(shade),juce::uint8(shade+2),juce::uint8(shade)));
        }
    }
    juce::Graphics wear(texture);
    wear.reduceClipRegion(60,7,988,763);
    juce::Random marks(0x504154494e41);
    for(int i=0;i<29;++i) {
        const float x=80+marks.nextFloat()*920,y=24+marks.nextFloat()*700;
        const float radius=35+marks.nextFloat()*100;
        const auto stain=i%3==0 ? juce::Colour(0xff2a2117) : juce::Colours::black;
        wear.setGradientFill(juce::ColourGradient(stain.withAlpha(.13f+marks.nextFloat()*.13f),x,y,
            juce::Colours::transparentBlack,x+radius,y,true));
        wear.fillEllipse(x-radius,y-radius*.55f,radius*2,radius*1.1f);
    }
    for(int i=0;i<420;++i) {
        const float x=65+marks.nextFloat()*970,y=12+marks.nextFloat()*746;
        const float length=(i<36 ? 28.0f : 3.0f)+marks.nextFloat()*(i<36 ? 74.0f : 29.0f);
        const float rise=(marks.nextFloat()-.5f)*13;
        wear.setColour(juce::Colours::black.withAlpha(.20f+marks.nextFloat()*.22f));
        wear.drawLine(x,y,x+length,y+rise,.65f+marks.nextFloat()*.55f);
        wear.setColour(juce::Colour(0xffc9c9b5).withAlpha(.12f+marks.nextFloat()*.20f));
        wear.drawLine(x,y+.9f,x+length,y+rise+.9f,.55f);
    }
    for(int i=0;i<650;++i) {
        wear.setColour(juce::Colours::black.withAlpha(.08f+marks.nextFloat()*.18f));
        wear.fillEllipse(65+marks.nextFloat()*970,10+marks.nextFloat()*748,1+marks.nextFloat()*3.5f,.7f+marks.nextFloat()*2);
    }
    const auto chip=[&](float x,float y,float width,float height,float opacity) {
        juce::Path patch;
        patch.startNewSubPath(x-width*.5f,y-height*.2f);
        patch.lineTo(x-width*.28f,y-height*.54f);
        patch.lineTo(x+width*.13f,y-height*.35f);
        patch.lineTo(x+width*.46f,y-height*.48f);
        patch.lineTo(x+width*.53f,y+height*.13f);
        patch.lineTo(x+width*.22f,y+height*.49f);
        patch.lineTo(x-width*.1f,y+height*.3f);
        patch.lineTo(x-width*.49f,y+height*.47f);
        patch.closeSubPath();
        patch.applyTransform(juce::AffineTransform::rotation((marks.nextFloat()-.5f)*.5f,x,y));
        wear.setColour(juce::Colours::black.withAlpha(opacity*.7f));
        wear.strokePath(patch,juce::PathStrokeType(1.7f));
        wear.setColour(juce::Colour(0xffa6a692).withAlpha(opacity)); wear.fillPath(patch);
        wear.setColour(juce::Colour(0xffdfddc5).withAlpha(opacity*.45f));
        wear.drawLine(x-width*.28f,y-height*.54f,x+width*.13f,y-height*.35f,.6f);
    };
    // Most exposed metal lives on case edges, seams and the areas rubbed by
    // hands. The lettering is painted afterwards and remains legible.
    for(int i=0;i<290;++i) {
        const int edge=marks.nextInt(6);
        float x=0,y=0;
        if(edge<2) {
            x=edge==0 ? 63+marks.nextFloat()*28 : 1019+marks.nextFloat()*25;
            y=16+marks.nextFloat()*728;
        } else {
            x=75+marks.nextFloat()*950;
            const float band=edge==2 ? 16.0f : edge==3 ? 737.0f : edge==4 ? 241.0f : 284.0f;
            y=band+(marks.nextFloat()-.5f)*17;
        }
        chip(x,y,2+marks.nextFloat()*13,1+marks.nextFloat()*4,.24f+marks.nextFloat()*.38f);
    }
    const std::array<juce::Point<float>,5> rubbedAreas{{{208,606},{826,628},{986,628},{965,110},{400,608}}};
    for(const auto centre:rubbedAreas) {
        const float radius=centre.x<250 ? 83.0f : 57.0f;
        for(int i=0;i<34;++i) {
            const float angle=float(kbass::pi*1.15)+marks.nextFloat()*float(kbass::pi*1.7);
            const float distance=radius+marks.nextFloat()*12;
            chip(centre.x+std::sin(angle)*distance,centre.y-std::cos(angle)*distance,
                1.5f+marks.nextFloat()*5,1+marks.nextFloat()*2.2f,.16f+marks.nextFloat()*.22f);
        }
    }
    for(int group=0;group<19;++group) {
        const float cx=100+marks.nextFloat()*865,cy=38+marks.nextFloat()*673;
        for(int n=0;n<12;++n) {
            const float x=cx+(marks.nextFloat()-.5f)*43,y=cy+(marks.nextFloat()-.5f)*18;
            const float length=8+marks.nextFloat()*35;
            wear.setColour(juce::Colour(0xffc7c6b0).withAlpha(.045f+marks.nextFloat()*.11f));
            wear.drawLine(x,y,x+length,y-2-marks.nextFloat()*4,.6f);
        }
    }
    juce::Graphics woodWear(texture);
    for(int side=0;side<2;++side) for(int i=0;i<85;++i) {
        const float x=(side==0 ? 5.0f : 1064.0f)+marks.nextFloat()*36,y=12+marks.nextFloat()*744;
        woodWear.setColour(juce::Colour(0xffb0956b).withAlpha(.10f+marks.nextFloat()*.18f));
        woodWear.drawLine(x,y,x+(marks.nextFloat()-.5f)*2,y+2+marks.nextFloat()*24,.5f+marks.nextFloat()*.5f);
    }
}
void KBassEditor::drawPanel(juce::Graphics& g,juce::Rectangle<float> r,const juce::String& title,float labelX) {
    g.setColour(juce::Colour(0xffd7d9c6).withAlpha(.025f)); g.fillRoundedRectangle(r,17);
    g.setColour(line.withAlpha(.76f)); g.drawRoundedRectangle(r,17,2.6f);
    const float width=title=="ENVELOPE" ? 190.0f : title=="OSC1/2" ? 140.0f : title=="FILTER" ? 130.0f : 90.0f;
    // Restore the same background pixels behind each interrupted border label.
    g.drawImage(texture,int(labelX-7),int(r.getY()-13),int(width),27,int(labelX-7),int(r.getY()-13),int(width),27);
    text(g,title,labelX-3,r.getY()-17,width-8,33,25);
}
void KBassEditor::drawScrew(juce::Graphics& g,float x,float y) {
    g.setColour(juce::Colours::black.withAlpha(.8f)); g.fillEllipse(x-15,y-13,30,30);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffa8aaa0),x-7,y-10,juce::Colour(0xff222824),x+9,y+11,false));
    g.fillEllipse(x-13,y-13,26,26); g.setColour(juce::Colour(0xff171d19)); g.drawEllipse(x-10,y-10,20,20,2);
    g.drawLine(x-6,y+7,x+6,y-7,4); g.setColour(juce::Colour(0xff92998b)); g.drawLine(x-7,y+5,x+5,y-8,1);
}
void KBassEditor::paint(juce::Graphics& g) {
    g.drawImageAt(texture,0,0);
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.6f),48,0,juce::Colours::transparentBlack,69,0,false));
    g.fillRect(48,0,22,780);
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack,1038,0,juce::Colours::black.withAlpha(.65f),1060,0,false));
    g.fillRect(1038,0,23,780);
    for(int x:{2,45,1060,1105}) { g.setColour(juce::Colours::black.withAlpha(.6f)); g.drawVerticalLine(x,0,780); }
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack,0,246,juce::Colours::black.withAlpha(.8f),0,262,false));
    g.fillRect(0,246,1108,17);
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.85f),0,263,juce::Colours::transparentBlack,0,284,false));
    g.fillRect(0,263,1108,22);
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack,0,747,juce::Colours::black.withAlpha(.75f),0,779,false));
    g.fillRect(0,747,1108,33);
    drawPanel(g,{67,39,138,182},"OSC1",108);
    drawPanel(g,{212,39,416,182},"OSC2",385);
    drawPanel(g,{636,39,404,182},"OSC1/2",790);
    drawPanel(g,{67,295,561,182},"ENVELOPE",277);
    drawPanel(g,{636,295,266,182},"LFO",741);
    drawPanel(g,{67,503,662,216},"FILTER",289);
    const auto waveLabels=[&](float x,float y,bool lfo) {
        text(g,"SAW",x,y,43,21,16,juce::Justification::centredLeft);
        text(g,lfo ? "TRI" : "SQU",x,y+28,43,21,16,juce::Justification::centredLeft);
        text(g,lfo ? "SQU" : "SINE",x,y+56,43,21,16,juce::Justification::centredLeft);
    };
    waveLabels(73,74,false); waveLabels(218,74,false); waveLabels(642,330,true);
    text(g,"0",352,74,29,21,17); text(g,"-1",352,102,29,21,17); text(g,"-2",352,130,29,21,17);
    text(g,"WAVE",92,177,108,30,23); text(g,"WAVE",236,177,108,30,23);
    text(g,"OCTAV",361,177,118,30,23); text(g,"DETUNE",499,177,119,30,23);
    text(g,"GLIDE",651,177,118,30,23); text(g,"LFO",776,177,118,30,23); text(g,"MIX",906,177,118,30,23);
    const char* names[]={"ATTACK","DECAY","SUSTAIN","RELEASE"};
    for(int i=0;i<4;++i) text(g,names[i],84+120.0f*i,431,122,32,23);
    text(g,"AMP",573,306,47,22,16); text(g,"FILT",573,401,47,22,16);
    text(g,"WAVE",664,431,105,32,23); text(g,"SPEED",777,431,105,32,23);
    text(g,"CUTOFF",134,671,150,34,23); text(g,"RESO",347,671,105,34,23);
    text(g,"ENV",477,671,105,34,23); text(g,"LFO",607,671,105,34,23);
    text(g,"VOLUME",754,681,144,27,20);
    text(g,"SATURATION",908,681,140,27,20);
    text(g,"TUBE",781,492,80,22,14,juce::Justification::centredRight);
    text(g,"TAPE",781,519,80,22,14,juce::Justification::centredRight);
    text(g,"TRANSISTOR",770,546,91,22,13,juce::Justification::centredRight);
    text(g,"SAT TYPE",944,516,104,22,15);
    for(const auto center:std::array<juce::Point<float>,1>{{{208,606}}})
        for(int n=0;n<=4;++n) {
            const float a=float(kbass::pi*1.25+n*kbass::pi*1.5/4);
            text(g,juce::String(n*3),center.x+std::sin(a)*91-14,center.y-std::cos(a)*91-11-((n==0 || n==4) ? 12.0f : 0.0f),28,23,15);
        }
    for(int i=0;i<4;++i) {
        const auto r=juce::Rectangle<float>(922,295+i*24.0f,109,14);
        g.setColour(juce::Colour(0xff85897c).withAlpha(.55f)); g.fillRoundedRectangle(r.translated(0,1.5f),7);
        g.setColour(juce::Colour(0xff181c18)); g.fillRoundedRectangle(r,7);
        if(meter>.015f && i==3) {
            g.setColour(juce::Colour(0xffa2b77c).withAlpha(std::min(.7f,meter)));
            g.fillRoundedRectangle(r.reduced(5,4).withWidth(99*std::min(1.0f,meter*2)),3);
        }
    }
    g.setFont(juce::Font(juce::FontOptions("Georgia",48,juce::Font::bold|juce::Font::italic)));
    g.setColour(ink.withAlpha(.95f)); g.drawText("KBass",777,716,266,56,juce::Justification::centredRight);
    text(g,"FACTORY PRESETS",380,722,205,17,11,juce::Justification::centredLeft);
    text(g,"USER PRESETS",596,722,205,17,11,juce::Justification::centredLeft);
    for(float x:{24.0f,1083.0f}) { drawScrew(g,x,38); drawScrew(g,x,728); }
}
void KBassEditor::resized() {
    const auto place=[&](const char* id,int x,int y,int w,int h) { if(auto* s=findControl(id)) s->setBounds(x,y,w,h); };
    place("wave1",111,75,70,70); place("wave2",255,75,70,70); place("octave",385,75,70,70);
    place("detune",510,61,98,98); place("glide",661,61,98,98); place("pitchLfo",786,61,98,98); place("mix",916,61,98,98);
    for(int i=0;i<4;++i) envelope[size_t(i)].setBounds(96+120*i,316,98,98);
    envelopeTarget.setBounds(581,334,29,61);
    place("lfoWave",681,331,70,70); place("lfoSpeed",780,316,98,98);
    place("cutoff",129,527,158,158); place("resonance",351,559,98,98);
    place("filterEnv",481,559,98,98); place("filterLfo",611,559,98,98);
    place("volume",774,576,104,104); place("saturation",934,576,104,104);
    place("saturationType",871,491,70,70);
    factoryPresets.setBounds(380,741,205,27);
    userPresets.setBounds(596,741,205,27);
    savePreset.setBounds(812,741,68,27);
}
