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
    s.setMouseDragSensitivity(280);
    s.setPopupDisplayEnabled(true,true,parent);
    s.setWantsKeyboardFocus(false);
}
}

HardwareLook::HardwareLook():chrome(juce::Image::ARGB,256,256,true) {
    setColour(juce::TooltipWindow::backgroundColourId,juce::Colour(0xff202220));
    setColour(juce::TooltipWindow::textColourId,ink);
    setColour(juce::TooltipWindow::outlineColourId,line);
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff282b28));
    setColour(juce::PopupMenu::textColourId,ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff545b50));
    for(int y=0;y<256;++y) for(int x=0;x<256;++x) {
        const double dx=(x-127.5)/127.5,dy=(y-127.5)/127.5,r=std::hypot(dx,dy);
        if(r>1) continue;
        const double a=std::atan2(dy,dx);
        const double cone=139+63*std::cos(2*a+.5)+26*std::sin(3*a-.8)-16*dy;
        const double edge=r>.94 ? 30*(1-r)/.06-17 : 0;
        const int shade=juce::jlimit(36,238,int(cone+edge+3*std::sin(r*360)));
        chrome.setPixelAt(x,y,juce::Colour::fromRGB(juce::uint8(shade),juce::uint8(shade+1),juce::uint8(std::min(255,shade+2))));
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
    }
    const float capRadius=selector ? r*.84f : r*(large ? .57f : .53f);
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
    juce::DropShadow(juce::Colours::black.withAlpha(.8f),4,{2,3}).drawForPath(g,p);
    g.setGradientFill(juce::ColourGradient(juce::Colour(down ? 0xff73766c : 0xffc4c5b9),0,0,
        juce::Colour(hover ? 0xff8c9283 : 0xff797d70),r.getWidth(),r.getHeight(),false));
    g.fillRoundedRectangle(r,3);
    g.setColour(juce::Colour(0xffe3e2d0).withAlpha(.5f)); g.drawRoundedRectangle(r.reduced(1),3,1);
    for(const auto point:std::array<juce::Point<float>,4>{{{6,6},{r.getWidth()-4,6},{6,r.getHeight()-4},{r.getWidth()-4,r.getHeight()-4}}}) {
        g.setColour(juce::Colour(0xff2c332e)); g.fillEllipse(point.x-2,point.y-2,4,4);
        g.setColour(juce::Colour(0xffbcc3b4)); g.drawLine(point.x-1,point.y-1,point.x+1,point.y+1);
    }
}
void HardwareLook::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) {
    g.setColour(juce::Colour(0xff282c27)); g.setFont(juce::Font(juce::FontOptions("Georgia",29,juce::Font::bold|juce::Font::italic)));
    g.drawText("KB",5,4,b.getWidth()-10,38,juce::Justification::centred);
    g.setFont(juce::Font(juce::FontOptions(11,juce::Font::bold)));
    g.drawFittedText(b.getButtonText(),9,43,b.getWidth()-18,18,juce::Justification::centred,1);
}

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
    addDial("volume","VOLUME","Master output level; fully left mutes the instrument",false,true);
    for(size_t i=0;i<envelope.size();++i) {
        configure(envelope[i],this); envelope[i].setName(juce::StringArray{"ATTACK","DECAY","SUSTAIN","RELEASE"}[int(i)]);
        addAndMakeVisible(envelope[i]);
    }
    envelopeTarget.setName("AMP / FILT"); envelopeTarget.setWantsKeyboardFocus(false);
    envelopeTarget.setTooltip("Choose which independent envelope the four knobs edit. Both envelopes continue working.");
    addAndMakeVisible(envelopeTarget);
    targetAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"envTarget",envelopeTarget);
    envelopeTarget.onClick=[this] { bindEnvelope(); };
    presetButton.setName("Factory presets"); presetButton.setWantsKeyboardFocus(false);
    presetButton.setButtonText(processor.getProgramName(processor.getCurrentProgram()).substring(4).toUpperCase());
    presetButton.setTooltip("Choose a factory preset"); presetButton.onClick=[this] { showPresets(); };
    addAndMakeVisible(presetButton);
    bindEnvelope(); makeTexture(); setSize(1108,780); startTimerHz(20);
}
KBassEditor::~KBassEditor() {
    stopTimer(); envelopeTarget.onClick=nullptr; presetButton.onClick=nullptr;
    setLookAndFeel(nullptr);
}
void KBassEditor::addDial(const char* id,const char* name,const char* help,bool selector,bool large) {
    Dial dial; dial.id=id; dial.slider=std::make_unique<juce::Slider>();
    auto& s=*dial.slider; configure(s,this); s.setName(name); s.setComponentID(id); s.setTooltip(help);
    s.getProperties().set("selector",selector); s.getProperties().set("large",large);
    addAndMakeVisible(s);
    dial.attachment=std::make_unique<Attachment>(processor.parameters,id,s);
    s.setDoubleClickReturnValue(true,processor.parameters.getParameter(id)->convertFrom0to1(processor.parameters.getParameter(id)->getDefaultValue()));
    if(selector) s.setMouseDragSensitivity(100);
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
    const auto name=processor.getProgramName(processor.getCurrentProgram()).substring(4).toUpperCase();
    if(presetButton.getButtonText()!=name) presetButton.setButtonText(name);
    const float level=processor.peak.load(std::memory_order_relaxed);
    const float next=std::max(level,meter*.85f);
    if(std::abs(next-meter)>.001f) { meter=next; repaint(920,290,115,95); }
}
void KBassEditor::showPresets() {
    juce::PopupMenu menu;
    for(int i=0;i<processor.getNumPrograms();++i)
        menu.addItem(i+1,processor.getProgramName(i),true,processor.getCurrentProgram()==i);
    const juce::Component::SafePointer<KBassEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(&presetButton),[safe](int choice) {
        if(safe && choice>0) safe->processor.setCurrentProgram(choice-1);
    });
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
            const double brushed=1.8*std::sin(x*.91)+.8*std::sin(x*2.7);
            const int shade=juce::jlimit(30,112,int(91-vignette+brushed+random.nextInt(12)-6));
            texture.setPixelAt(x,y,juce::Colour::fromRGB(juce::uint8(shade),juce::uint8(shade+2),juce::uint8(shade)));
        }
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
    text(g,"VOLUME",824,671,154,34,23);
    for(const auto center:std::array<juce::Point<float>,2>{{{208,606},{900,606}}})
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
    g.setFont(juce::Font(juce::FontOptions(10,juce::Font::plain)));
    g.setColour(ink.withAlpha(.55f)); g.drawText("SINE EDITION  /  VOYAGER SQUARE",76,738,350,17,juce::Justification::centredLeft);
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
    place("filterEnv",481,559,98,98); place("filterLfo",611,559,98,98); place("volume",821,527,158,158);
    presetButton.setBounds(913,400,129,76);
}
