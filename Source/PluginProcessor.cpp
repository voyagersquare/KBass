#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
struct Preset { const char* name; std::array<float,23> values; };
constexpr std::array<Preset,6> presets {{
    {"01  Classic", {0,2,1,4,.06f,0,.35f, .005f,.25f,.75f,.16f, .002f,.22f,.05f,.15f, 0,1,4,650,.22f,3,0,.7f}},
    {"02  Sine Sub",{2,2,1,0,.09f,0,.15f, .008f,.3f,.95f,.2f, .002f,.2f,.5f,.2f, 0,1,3,400,.05f,0,0,.85f}},
    {"03  Square Punch",{1,1,1,2,.025f,0,.3f, .002f,.18f,.3f,.09f, .001f,.12f,0,.09f, 1,1,4,450,.4f,4,0,.65f}},
    {"04  Saw Slide",{0,0,0,12,.22f,0,.5f, .005f,.3f,.8f,.18f, .002f,.3f,.2f,.18f, 0,1,4,900,.28f,2,0,.65f}},
    {"05  Filter Motion",{0,2,1,3,.05f,0,.3f, .008f,.2f,.8f,.2f, .002f,.25f,.2f,.15f, 1,1,2.5f,850,.5f,1,1.8f,.65f}},
    {"06  Sine Lead",{2,2,0,5,.1f,.08f,.5f, .012f,.2f,.85f,.3f, .004f,.3f,.6f,.25f, 0,1,5,5000,.1f,.5f,0,.75f}}
}};
}

juce::AudioProcessorValueTreeState::ParameterLayout KBassProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto choice=[&](const char* id,const char* name,juce::StringArray values,int initial,bool automate=true) {
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,values,initial,
            juce::AudioParameterChoiceAttributes{}.withAutomatable(automate)));
    };
    const auto add=[&](const char* id,const char* name,float low,float high,float value,float centre,const char* unit) {
        juce::NormalisableRange<float> range(low,high);
        if(centre>low && centre<high) range.setSkewForCentre(centre);
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,range,value,
            juce::AudioParameterFloatAttributes{}.withLabel(unit)));
    };
    choice("wave1","Oscillator 1 waveform",{"Saw","Square","Sine"},0);
    choice("wave2","Oscillator 2 waveform",{"Saw","Square","Sine"},2);
    choice("octave","Oscillator 2 octave",{"0","-1","-2"},1);
    add("detune","Oscillator 2 detune",-50,50,4,0,"ct");
    add("glide","Glide",0,1.5f,.06f,.15f,"s");
    add("pitchLfo","Oscillators LFO amount",0,2,0,.2f,"st");
    add("mix","Oscillator mix",0,1,.35f,.5f,"");
    add("ampAttack","Amp attack",.001f,4,.005f,.1f,"s");
    add("ampDecay","Amp decay",.005f,4,.25f,.3f,"s");
    add("ampSustain","Amp sustain",0,1,.75f,.5f,"");
    add("ampRelease","Amp release",.005f,8,.16f,.4f,"s");
    add("filterAttack","Filter attack",.001f,4,.002f,.1f,"s");
    add("filterDecay","Filter decay",.005f,4,.22f,.3f,"s");
    add("filterSustain","Filter sustain",0,1,.05f,.5f,"");
    add("filterRelease","Filter release",.005f,8,.15f,.4f,"s");
    choice("envTarget","Envelope editor",{"AMP","FILT"},0,false);
    choice("lfoWave","LFO waveform",{"Saw","Triangle","Square"},1);
    add("lfoSpeed","LFO speed",.03f,30,4,3,"Hz");
    add("cutoff","Filter cutoff",20,18000,650,700,"Hz");
    add("resonance","Filter resonance",0,1,.22f,.5f,"");
    add("filterEnv","Filter envelope amount",0,6,3,3,"oct");
    add("filterLfo","Filter LFO amount",0,4,0,1,"oct");
    add("volume","Output volume",0,1,.7f,.5f,"");
    return layout;
}
KBassProcessor::KBassProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      parameters(*this,nullptr,"KBassState",createLayout()) {
    for(size_t i=0;i<raw.size();++i) raw[i]=parameters.getRawParameterValue(parameterIds[i]);
}
kbass::Parameters KBassProcessor::readParameters() const noexcept {
    std::array<float,23> v{};
    for(size_t i=0;i<v.size();++i) v[i]=raw[i]->load(std::memory_order_relaxed);
    kbass::Parameters p;
    p.wave1=int(v[0]); p.wave2=int(v[1]); p.octave=int(v[2]); p.detune=v[3];
    p.glide=v[4]; p.pitchLfo=v[5]; p.mix=v[6];
    p.amp={v[7],v[8],v[9],v[10]}; p.filter={v[11],v[12],v[13],v[14]};
    p.lfoWave=int(v[16]); p.lfoSpeed=v[17]; p.cutoff=v[18]; p.resonance=v[19];
    p.filterEnv=v[20]; p.filterLfo=v[21]; p.volume=v[22]; return p;
}
void KBassProcessor::prepareToPlay(double rate,int) {
    engine.setParameters(readParameters()); engine.prepare(rate); keyboard.reset(); peak=0;
}
void KBassProcessor::releaseResources() { resetRequested=true; peak=0; }
void KBassProcessor::reset() { resetRequested=true; }
bool KBassProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    return layout.getMainInputChannelSet().isDisabled() &&
        (layout.getMainOutputChannelSet()==juce::AudioChannelSet::mono() || layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());
}
void KBassProcessor::dispatchMidi(const juce::MidiMessage& m) {
    const int c=m.getChannel();
    if(m.isNoteOn()) engine.noteOn(c,m.getNoteNumber(),m.getFloatVelocity());
    else if(m.isNoteOff()) engine.noteOff(c,m.getNoteNumber());
    else if(m.isPitchWheel()) engine.pitchWheel(c,m.getPitchWheelValue());
    else if(m.isSustainPedalOn()) engine.sustainPedal(c,true);
    else if(m.isSustainPedalOff()) engine.sustainPedal(c,false);
    else if(m.isAllSoundOff()) engine.allNotesOff(c,true);
    else if(m.isAllNotesOff()) engine.allNotesOff(c,false);
    else if(m.isController() && m.getControllerNumber()==121) { engine.sustainPedal(c,false); engine.pitchWheel(c,8192); }
}
void KBassProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    engine.setParameters(readParameters());
    if(resetRequested.exchange(false)) { engine.reset(); keyboard.reset(); }
    const int length=audio.getNumSamples();
    if(audio.getNumChannels()==0 || length==0) { midi.clear(); return; }
    keyboard.processNextMidiBuffer(midi,0,length,true);
    auto* left=audio.getWritePointer(0);
    auto* right=audio.getNumChannels()>1 ? audio.getWritePointer(1) : nullptr;
    int position=0;
    for(const auto metadata:midi) {
        const int offset=juce::jlimit(position,length,metadata.samplePosition);
        engine.render(left+position,right ? right+position : nullptr,offset-position);
        dispatchMidi(metadata.getMessage()); position=offset;
    }
    engine.render(left+position,right ? right+position : nullptr,length-position);
    peak.store(audio.getMagnitude(0,length),std::memory_order_relaxed); midi.clear();
}
const juce::String KBassProcessor::getProgramName(int index) {
    return juce::isPositiveAndBelow(index,int(presets.size())) ? presets[size_t(index)].name : juce::String{};
}
void KBassProcessor::setCurrentProgram(int index) {
    if(!juce::isPositiveAndBelow(index,int(presets.size()))) return;
    for(size_t i=0;i<parameterIds.size();++i) {
        auto* p=parameters.getParameter(parameterIds[i]);
        p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(presets[size_t(index)].values[i])); p->endChangeGesture();
    }
    currentProgram=index; resetRequested=true;
}
void KBassProcessor::getStateInformation(juce::MemoryBlock& data) {
    auto state=parameters.copyState(); state.setProperty("schemaVersion",1,nullptr);
    state.setProperty("program",currentProgram.load(),nullptr);
    if(const auto xml=state.createXml()) copyXmlToBinary(*xml,data);
}
void KBassProcessor::setStateInformation(const void* data,int size) {
    if(const auto xml=getXmlFromBinary(data,size)) {
        if(!xml->hasTagName("KBassState")) return;
        auto state=juce::ValueTree::fromXml(*xml);
        if(!state.isValid() || int(state.getProperty("schemaVersion",0))!=1) return;
        // Validate the entire tree before allowing any value into the audio engine.
        for(const auto* id:parameterIds) {
            const auto child=state.getChildWithProperty("id",id);
            if(!child.isValid()) return;
            const float v=float(child.getProperty("value"));
            auto* p=parameters.getParameter(id);
            const auto range=p->getNormalisableRange();
            if(!std::isfinite(v) || v<range.start || v>range.end) return;
        }
        parameters.replaceState(state);
        currentProgram=juce::jlimit(0,5,int(state.getProperty("program",0))); resetRequested=true;
    }
}
juce::AudioProcessorEditor* KBassProcessor::createEditor() { return new KBassEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KBassProcessor(); }
