#pragma once
#include "PluginProcessor.h"

class HardwareLook final : public juce::LookAndFeel_V4 {
public:
    HardwareLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
private:
    juce::Image chrome;
};

class KBassEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit KBassEditor(KBassProcessor&);
    ~KBassEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    juce::Slider* findControl(const juce::String& id);
    void selectEnvelope(bool filter);
private:
    using Attachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    struct Dial {
        juce::String id;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<Attachment> attachment;
    };
    void addDial(const char*,const char*,const char*,bool selector=false,bool large=false);
    void bindEnvelope();
    void timerCallback() override;
    void showPresets();
    void makeTexture();
    void drawPanel(juce::Graphics&,juce::Rectangle<float>,const juce::String&,float labelX);
    void drawScrew(juce::Graphics&,float,float);
    KBassProcessor& processor;
    HardwareLook hardware;
    std::vector<Dial> dials;
    std::array<juce::Slider,4> envelope;
    std::array<std::unique_ptr<Attachment>,4> envelopeAttachments;
    juce::ToggleButton envelopeTarget;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> targetAttachment;
    juce::TextButton presetButton;
    juce::TooltipWindow tooltips{this,600};
    juce::Image texture;
    int displayedEnvelope=-1;
    float meter=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KBassEditor)
};
