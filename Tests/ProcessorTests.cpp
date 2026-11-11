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
            if(dynamic_cast<juce::Slider*>(child) || dynamic_cast<juce::Button*>(child)) controls.push_back(child);
        }
        require(controls.size()==20,"all nineteen synth controls and preset plaque are visible");
        for(size_t a=0;a<controls.size();++a) {
            require(editor->getLocalBounds().reduced(48,20).contains(controls[a]->getBounds()),"controls remain within metal faceplate");
            for(size_t b=a+1;b<controls.size();++b)
                require(!controls[a]->getBounds().intersects(controls[b]->getBounds()),"interactive hit areas do not overlap");
        }
        if(i==0 && argc>1) {
            restored.setCurrentProgram(0); editor->selectEnvelope(false);
            auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);
            juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
            auto stream=output.createOutputStream(); require(stream!=nullptr,"screenshot output");
            require(juce::PNGImageFormat{}.writeImageToStream(image,*stream),"real editor screenshot");
            restored.setStateInformation(saved.getData(),int(saved.getSize()));
        }
    }
    p.reset(); p.processBlock(audio,midi); require(audio.getMagnitude(0,512)==0,"host reset clears sound");
    auto layout=p.getBusesLayout(); layout.outputBuses.set(0,juce::AudioChannelSet::mono());
    require(p.setBusesLayout(layout),"mono output supported"); p.prepareToPlay(44100,256); audio.setSize(1,256);
    midi.addEvent(juce::MidiMessage::noteOn(1,48,1.0f),0); p.processBlock(audio,midi);
    require(audio.getMagnitude(0,256)>.0001,"mono output renders");
    std::cout<<"PASS: sample-accurate MIDI, factory sounds, complete state, envelope switching, UI lifecycle and audio buses\n";
}
