#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdlib>
#include <iostream>

static void require(bool condition,const char* name) {
    if(!condition) { std::cerr<<"FAIL: "<<name<<'\n'; std::exit(1); }
}
static void set(KBassProcessor& p,const char* id,float value) {
    auto* parameter=p.parameters.getParameter(id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
static void checkSmoothCutoff(KBassProcessor& processor,juce::Slider& slider) {
    struct Listener final:juce::AudioProcessorParameter::Listener {
        int begins=0,ends=0;
        void parameterValueChanged(int,float) override {}
        void parameterGestureChanged(int,bool begin) override { if(begin) ++begins; else ++ends; }
    } listener;
    auto* parameter=processor.parameters.getParameter("cutoff"); parameter->addListener(&listener);
    juce::int64 eventTime=10000;
    const auto event=[&](juce::Point<float> position,juce::ModifierKeys modifiers) {
        const juce::Time time(++eventTime);
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),position,modifiers,
            1,0,0,0,0,&slider,&slider,time,{40,40},time,1,true);
    };
    const auto reset=[&](float hz) { set(processor,"cutoff",hz); listener.begins=listener.ends=0; };
    const int left=juce::ModifierKeys::leftButtonModifier,shift=juce::ModifierKeys::shiftModifier;
    for(const float hz:{30.0f,200.0f,700.0f,5000.0f,16000.0f}) {
        reset(hz); slider.mouseDown(event({40,40},left)); slider.mouseDrag(event({40,39},left)); slider.mouseUp(event({40,39},{}));
        const double ratio=(slider.getValue()-hz)/hz;
        require(ratio>.002 && ratio<.004,"one-pixel cutoff movement is small and uniform across the whole range");
        require(std::abs(slider.getValue()-processor.parameters.getRawParameterValue("cutoff")->load())<.002,"smooth cutoff drag reaches the audio parameter");
        require(listener.begins==1 && listener.ends==1,"cutoff drag preserves host automation gestures");
    }
    reset(700); slider.mouseDown(event({40,40},left)); slider.mouseDrag(event({40,39},left));
    const double coarse=slider.getValue();
    slider.mouseDrag(event({40,38},left|shift)); const double fine=slider.getValue();
    slider.mouseDrag(event({40,37},left)); const double normal=slider.getValue(); slider.mouseUp(event({40,37},{}));
    require(fine>coarse && fine-coarse<(coarse-700)*.11 && normal>fine,"Shift changes precision mid-drag without a value jump");
    require(listener.begins==1 && listener.ends==1,"precision changes keep a single automation gesture");
    juce::MouseWheelDetails wheel{0,.1f,false,false,false};
    reset(700); slider.mouseWheelMove(event({40,40},{}),wheel); const double normalWheel=slider.getValue()-700;
    require(normalWheel>0 && normalWheel<6,"cutoff wheel gives fine frequency changes");
    reset(700); slider.mouseWheelMove(event({40,40},shift),wheel);
    require(slider.getValue()>700 && slider.getValue()-700<normalWheel*.11,"Shift wheel gives tenfold finer adjustment");
    wheel.isReversed=true; reset(700); slider.mouseWheelMove(event({40,40},{}),wheel);
    require(slider.getValue()<700,"wheel direction is respected");
    reset(700); require(slider.keyPressed(juce::KeyPress(juce::KeyPress::upKey)),"cutoff accepts arrow adjustment");
    require(std::abs(slider.getValue()-701)<.01,"arrow step is one Hz");
    require(slider.keyPressed(juce::KeyPress(juce::KeyPress::downKey,shift,0)),"cutoff accepts fine arrow adjustment");
    require(std::abs(slider.getValue()-700.9)<.01,"Shift arrow step is 0.1 Hz");
    reset(18000); slider.mouseWheelMove(event({40,40},{}),juce::MouseWheelDetails{0,.1f,false,false,false});
    require(std::abs(slider.getValue()-18000)<.01,"cutoff maximum is enforced");
    reset(20); slider.keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(std::abs(slider.getValue()-20)<.01,"cutoff minimum is enforced");
    parameter->removeListener(&listener); reset(650);
}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    KBassProcessor p; p.prepareToPlay(48000,512);
    require(p.getParameters().size()==23,"all audio parameters and envelope selector are registered");
    require(p.acceptsMidi() && !p.producesMidi(),"instrument MIDI routing");
    juce::AudioBuffer<float> audio(2,512); juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1,36,1.0f),123); p.processBlock(audio,midi);
    require(audio.getMagnitude(0,123)==0,"sample-accurate MIDI onset");
    require(audio.getMagnitude(123,389)>.001,"instrument generates audio");
    require(midi.isEmpty(),"no unexpected MIDI output");
    for(int i=0;i<p.getNumPrograms();++i) {
        p.setCurrentProgram(i); midi.addEvent(juce::MidiMessage::noteOn(1,36,.9f),0); p.processBlock(audio,midi);
        require(audio.getMagnitude(0,512)>.0001,"each factory preset produces sound");
    }
    set(p,"wave1",2); set(p,"wave2",1); set(p,"ampAttack",.24f); set(p,"filterAttack",.67f); set(p,"envTarget",1); set(p,"cutoff",432);
    juce::MemoryBlock saved; p.getStateInformation(saved);
    KBassProcessor restored; restored.setStateInformation(saved.getData(),int(saved.getSize()));
    for(const char* id:parameterIds)
        require(std::abs(p.parameters.getRawParameterValue(id)->load()-restored.parameters.getRawParameterValue(id)->load())<1e-5,"DAW state round trip retains all parameters");
    const char invalid[]="bad state"; restored.setStateInformation(invalid,sizeof(invalid));
    require(restored.parameters.getRawParameterValue("cutoff")->load()==432,"invalid state leaves patch intact");
    auto broken=p.parameters.copyState(); broken.setProperty("schemaVersion",1,nullptr);
    broken.getChildWithProperty("id","cutoff").setProperty("value",-10,nullptr);
    juce::MemoryBlock invalidState; juce::AudioProcessor::copyXmlToBinary(*broken.createXml(),invalidState);
    restored.setStateInformation(invalidState.getData(),int(invalidState.getSize()));
    require(restored.parameters.getRawParameterValue("cutoff")->load()==432,"out-of-range state is rejected atomically");
    for(int i=0;i<3;++i) {
        std::unique_ptr<KBassEditor> editor(static_cast<KBassEditor*>(restored.createEditor()));
        require(editor && editor->getWidth()==1108 && editor->getHeight()==780,"reference-sized editor opens and reopens");
        if(i==0) {
            auto* cutoffSlider=editor->findControl("cutoff"); require(cutoffSlider!=nullptr,"cutoff control is present");
            checkSmoothCutoff(restored,*cutoffSlider);
        }
        editor->selectEnvelope(false);
        auto* attack=editor->findControl("ampAttack"); require(attack && std::abs(attack->getValue()-.24)<1e-5,"AMP knobs restore their own values");
        attack->setValue(.31,juce::sendNotificationSync);
        editor->selectEnvelope(true);
        attack=editor->findControl("filterAttack"); require(attack && std::abs(attack->getValue()-.67)<1e-5,"FILT knobs restore separate values");
        require(std::abs(restored.parameters.getRawParameterValue("ampAttack")->load()-.31f)<1e-5,"switching envelope retains AMP edits");
        set(restored,"ampAttack",.24f);
        std::vector<juce::Component*> controls;
        for(auto* child:editor->getChildren()) {
            if(!child->isVisible()) continue;
            if(dynamic_cast<juce::Slider*>(child) || dynamic_cast<juce::Button*>(child) || dynamic_cast<juce::ComboBox*>(child)) controls.push_back(child);
        }
        require(controls.size()==22,"nineteen synth controls, separate factory/user menus and Save are visible");
        for(size_t a=0;a<controls.size();++a) {
            require(editor->getLocalBounds().reduced(48,8).contains(controls[a]->getBounds()),"controls remain within metal faceplate");
            for(size_t b=a+1;b<controls.size();++b)
                require(!controls[a]->getBounds().intersects(controls[b]->getBounds()),"interactive hit areas do not overlap");
        }
        if(i==0 && argc>1) {
            KBassProcessor pictured;
            std::unique_ptr<juce::AudioProcessorEditor> picture(pictured.createEditor());
            auto image=picture->createComponentSnapshot(picture->getLocalBounds(),true,1);
            juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
            output.deleteFile();
            auto stream=output.createOutputStream(); require(stream!=nullptr,"screenshot output");
            require(juce::PNGImageFormat{}.writeImageToStream(image,*stream),"real editor screenshot");
        }
    }
    p.reset(); p.processBlock(audio,midi); require(audio.getMagnitude(0,512)==0,"host reset clears sound");
    auto layout=p.getBusesLayout(); layout.outputBuses.set(0,juce::AudioChannelSet::mono());
    require(p.setBusesLayout(layout),"mono output supported"); p.prepareToPlay(44100,256); audio.setSize(1,256);
    midi.addEvent(juce::MidiMessage::noteOn(1,48,1.0f),0); p.processBlock(audio,midi);
    require(audio.getMagnitude(0,256)>.0001,"mono output renders");
    std::cout<<"PASS: sample-accurate MIDI, factory sounds, complete state, envelope switching, UI lifecycle and audio buses\n";
}
