#pragma once
#include "PluginProcessor.h"

class HardwareLook final : public juce::LookAndFeel_V4 {
public:
    HardwareLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
private:
    juce::Image chrome;
};

class SmoothDial final : public juce::Slider {
public:
    void enableFineCutoff() { fineCutoff=true; setMouseDragSensitivity(2200); }
    double proportionOfLengthToValue(double) override;
    double valueToProportionOfLength(double) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    bool fineCutoff=false;
    juce::Point<float> actualPosition,virtualPosition;
};

class PresetComboBox final : public juce::ComboBox {
public:
    std::function<void()> beforePopup;
    void preparePopup();
    void showPopup() override { preparePopup(); juce::ComboBox::showPopup(); }
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
    void refreshPresetMenus();
    void syncPresetSelection();
    void beginSavePreset();
    void saveNamedPreset(const juce::String&,bool replaceExisting);
    void showPresetError(const juce::String&);
    juce::AlertWindow* makePresetDialog(const juce::String&,const juce::String&);
    void makeTexture();
    void drawPanel(juce::Graphics&,juce::Rectangle<float>,const juce::String&,float labelX);
    void drawScrew(juce::Graphics&,float,float);
    KBassProcessor& processor;
    HardwareLook hardware;
    std::vector<Dial> dials;
    std::array<SmoothDial,4> envelope;
    std::array<std::unique_ptr<Attachment>,4> envelopeAttachments;
    juce::ToggleButton envelopeTarget;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> targetAttachment;
    PresetComboBox factoryPresets;
    PresetComboBox userPresets;
    juce::TextButton savePreset{"SAVE"};
    std::vector<KBassProcessor::UserPreset> userPresetFiles;
    juce::Component::SafePointer<juce::AlertWindow> presetDialog;
    juce::TooltipWindow tooltips{this,600};
    juce::Image texture;
    int displayedEnvelope=-1;
    float meter=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KBassEditor)
};
