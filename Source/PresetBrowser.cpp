#include "PluginEditor.h"

void PresetComboBox::preparePopup() {
    if(beforePopup) beforePopup();
    const int selected=getSelectedId();
    const juce::Component::SafePointer<PresetComboBox> safe(this);
    for(juce::PopupMenu::MenuItemIterator item(*getRootMenu());item.next();) {
        auto& entry=item.getItem();
        entry.action=nullptr;
        // JUCE suppresses onChange for the already selected item. An explicit
        // re-selection should still restore the saved sound after knob edits.
        if(selected>0 && entry.itemID==selected)
            entry.action=[safe] { if(safe && safe->onChange) safe->onChange(); };
    }
}

void KBassEditor::refreshPresetMenus() {
    userPresetFiles=processor.getUserPresets();
    factoryPresets.clear(juce::dontSendNotification);
    for(int i=0;i<processor.getNumPrograms();++i) factoryPresets.addItem(processor.getProgramName(i),i+1);
    userPresets.clear(juce::dontSendNotification);
    if(userPresetFiles.empty()) {
        userPresets.addItem("No user presets yet",1); userPresets.setItemEnabled(1,false);
    } else {
        for(size_t i=0;i<userPresetFiles.size();++i) userPresets.addItem(userPresetFiles[i].name,int(i)+1);
    }
    syncPresetSelection();
}
void KBassEditor::syncPresetSelection() {
    const auto name=processor.getCurrentUserPresetName();
    if(name.isEmpty()) {
        factoryPresets.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
        userPresets.setSelectedId(0,juce::dontSendNotification);
        return;
    }
    factoryPresets.setSelectedId(0,juce::dontSendNotification);
    for(size_t i=0;i<userPresetFiles.size();++i)
        if(userPresetFiles[i].name.equalsIgnoreCase(name)) {
            userPresets.setSelectedId(int(i)+1,juce::dontSendNotification); return;
        }
    // A DAW project embeds the complete patch even if its preset file is absent.
    userPresets.setText(name,juce::dontSendNotification);
}

juce::AlertWindow* KBassEditor::makePresetDialog(const juce::String& title,const juce::String& message)
{
    auto* window=new juce::AlertWindow(title,message,juce::MessageBoxIconType::NoIcon,this);
    window->setLookAndFeel(&hardware);
    presetDialog=window;
    return window;
}
void KBassEditor::beginSavePreset()
{
    if (presetDialog!=nullptr) { presetDialog->toFront(true); return; }
    auto* window=makePresetDialog("Save user preset","Name the current sound. It will appear under User presets.");
    const auto selected=processor.getCurrentUserPresetName();
    window->addTextEditor("name",selected.isEmpty() ? "New bass" : selected,"Preset name");
    window->getTextEditor("name")->setInputRestrictions(64);
    window->addButton("Save",1,juce::KeyPress{juce::KeyPress::returnKey});
    window->addButton("Cancel",0,juce::KeyPress{juce::KeyPress::escapeKey});
    const juce::Component::SafePointer<KBassEditor> self(this);
    const juce::Component::SafePointer<juce::AlertWindow> dialog(window);
    window->enterModalState(true,juce::ModalCallbackFunction::create([self,dialog](int result)
    {
        if (self==nullptr || dialog==nullptr) return;
        dialog->setLookAndFeel(nullptr);
        self->presetDialog=nullptr;
        if (result==1) self->saveNamedPreset(dialog->getTextEditorContents("name").trim(),false);
    }),true);
}
void KBassEditor::saveNamedPreset(const juce::String& name,bool replaceExisting)
{
    if (!replaceExisting && processor.userPresetExists(name))
    {
        auto* window=makePresetDialog("Replace user preset?","A user preset named \""+name+"\" already exists.");
        window->addButton("Replace",1,juce::KeyPress{juce::KeyPress::returnKey});
        window->addButton("Cancel",0,juce::KeyPress{juce::KeyPress::escapeKey});
        const juce::Component::SafePointer<KBassEditor> self(this);
        const juce::Component::SafePointer<juce::AlertWindow> dialog(window);
        window->enterModalState(true,juce::ModalCallbackFunction::create([self,dialog,name](int result)
        {
            if (self==nullptr) return;
            if (dialog!=nullptr) dialog->setLookAndFeel(nullptr);
            self->presetDialog=nullptr;
            if (result==1) self->saveNamedPreset(name,true);
        }),true);
        return;
    }
    const auto result=processor.saveUserPreset(name,replaceExisting);
    if (result.failed()) showPresetError(result.getErrorMessage());
    else refreshPresetMenus();
}
void KBassEditor::showPresetError(const juce::String& message)
{
    auto* window=makePresetDialog("KBass presets",message);
    window->addButton("OK",0,juce::KeyPress{juce::KeyPress::returnKey},juce::KeyPress{juce::KeyPress::escapeKey});
    window->enterModalState(true,nullptr,true);
}
