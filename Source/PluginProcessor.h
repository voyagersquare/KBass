#pragma once
#include <JuceHeader.h>
#include "SynthEngine.h"

inline constexpr std::array<const char*,23> parameterIds {
    "wave1","wave2","octave","detune","glide","pitchLfo","mix",
    "ampAttack","ampDecay","ampSustain","ampRelease",
    "filterAttack","filterDecay","filterSustain","filterRelease",
    "envTarget","lfoWave","lfoSpeed","cutoff","resonance","filterEnv","filterLfo","volume"
};

class KBassProcessor final : public juce::AudioProcessor {
public:
    KBassProcessor();
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 6; }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboard;
    std::atomic<float> peak{0};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    kbass::Parameters readParameters() const noexcept;
    void dispatchMidi(const juce::MidiMessage&);
    std::array<std::atomic<float>*,23> raw{};
    kbass::SynthEngine engine;
    std::atomic<int> currentProgram{0};
    std::atomic<bool> resetRequested{false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KBassProcessor)
};
