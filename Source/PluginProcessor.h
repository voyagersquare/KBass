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
    struct UserPreset { juce::String name; juce::File file; };
    explicit KBassProcessor(juce::File presetFolder = {});
    static juce::File defaultUserPresetFolder();
    juce::File getUserPresetFolder() const { return userPresetFolder; }
    std::vector<UserPreset> getUserPresets() const;
    bool userPresetExists(const juce::String&) const;
    juce::Result saveUserPreset(const juce::String&,bool replaceExisting=false);
    juce::Result loadUserPreset(const juce::File&);
    juce::String getCurrentUserPresetName() const;
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
    static bool isValidUserPresetName(const juce::String&);
    juce::Result readUserPreset(const juce::File&,juce::ValueTree&,juce::String&) const;
    void setUserPresetName(const juce::String&);
    std::array<std::atomic<float>*,23> raw{};
    kbass::SynthEngine engine;
    std::atomic<int> currentProgram{0};
    std::atomic<bool> resetRequested{false};
    const juce::File userPresetFolder;
    mutable juce::CriticalSection presetNameLock;
    juce::String currentUserPresetName;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KBassProcessor)
};
