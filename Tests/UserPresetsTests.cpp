#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdlib>
#include <iostream>

namespace
{
void require(bool ok,const char* message)
{
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct PresetFolder
{
    juce::File file=juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("KBass-presets-test-"+juce::Uuid().toString());
    ~PresetFolder() { file.deleteRecursively(); }
};
void setValue(KBassProcessor& p,const char* id,float value)
{
    auto* parameter=p.parameters.getParameter(id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
float value(KBassProcessor& p,const char* id) { return p.parameters.getRawParameterValue(id)->load(); }
}
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    PresetFolder folder;
    KBassProcessor p(folder.file);
    require(p.getNumPrograms()==6,"all six factory presets remain available");
    require(p.getUserPresets().empty() && !folder.file.exists(),"opening an empty library does not create files");
    p.setCurrentProgram(2);
    setValue(p,"cutoff",432); setValue(p,"wave1",2); setValue(p,"wave2",1); setValue(p,"ampAttack",.21f); setValue(p,"filterAttack",.63f);
    setValue(p,"saturation",.64f); setValue(p,"saturationType",2);
    std::array<float,parameterIds.size()> saved{};
    for (size_t i=0;i<saved.size();++i) saved[i]=value(p,parameterIds[i]);
    require(p.saveUserPreset("Night Bass").wasOk(),"save user preset");
    require(p.getCurrentUserPresetName()=="Night Bass","saved preset becomes the selected user preset");
    auto list=p.getUserPresets();
    require(list.size()==1 && list[0].name=="Night Bass","saved preset appears in the user library");
    require(list[0].file.getParentDirectory()==folder.file,"preset stays inside its dedicated directory");
    const auto originalBytes=list[0].file.loadFileAsString();
    setValue(p,"cutoff",1234);
    require(p.saveUserPreset("night bass").failed(),"case-insensitive duplicate requires explicit replacement");
    require(list[0].file.loadFileAsString()==originalBytes,"declining replacement preserves the file");
    setValue(p,"saturation",0); setValue(p,"saturationType",0);
    require(p.loadUserPreset(list[0].file).wasOk(),"reload saved sound");
    for (size_t i=0;i<saved.size();++i)
        require(std::abs(value(p,parameterIds[i])-saved[i])<.001f,"all parameters return to the saved values");

    KBassProcessor restarted(folder.file);
    require(restarted.getUserPresets().size()==1,"user library persists across new plugin instances");
    require(restarted.loadUserPreset(list[0].file).wasOk() && std::abs(value(restarted,"cutoff")-432)<.01f,"new instance restores the saved sound");
    require(std::abs(value(restarted,"saturation")-.64f)<1e-5 && value(restarted,"saturationType")==2,"new instance recalls saturation amount and type");
    juce::MemoryBlock project;
    restarted.getStateInformation(project);
    PresetFolder otherFolder;
    KBassProcessor restoredProject(otherFolder.file);
    restoredProject.setStateInformation(project.getData(),static_cast<int>(project.getSize()));
    require(restoredProject.getCurrentUserPresetName()=="Night Bass","DAW project restores the user preset name without its file");
    require(std::abs(value(restoredProject,"cutoff")-432)<.01f,"DAW project retains the sound independently of the preset library");
    restoredProject.setCurrentProgram(0);
    require(restoredProject.getCurrentUserPresetName().isEmpty(),"factory preset selection clears the user preset label");

    setValue(p,"cutoff",987);
    require(p.saveUserPreset("Night Bass",true).wasOk(),"explicit replacement saves the new sound");
    require(p.getUserPresets().size()==1,"replacement does not create duplicates");
    require(restarted.loadUserPreset(list[0].file).wasOk() && std::abs(value(restarted,"cutoff")-987)<.01f,"replacement can be loaded by another instance");
    require(p.saveUserPreset("   ").failed() && p.saveUserPreset("../outside").failed()
        && p.saveUserPreset(juce::String::repeatedString("x",65)).failed(),"invalid preset names are rejected");

    const auto corrupt=folder.file.getChildFile("corrupt.kbasspreset");
    require(corrupt.replaceWithText("not a preset"),"create invalid-file fixture");
    require(p.getUserPresets().size()==1,"corrupt files are excluded from the dropdown library");
    const auto before=value(p,"cutoff");
    require(p.loadUserPreset(corrupt).failed() && std::abs(value(p,"cutoff")-before)<.01f,"invalid files do not change the current sound");
    auto malformed=juce::XmlDocument::parse(list[0].file.loadFileAsString());
    malformed->getFirstChildElement()->getChildByAttribute("id","cutoff")->setAttribute("value","nan");
    require(malformed->writeTo(corrupt),"write invalid-parameter fixture");
    require(p.loadUserPreset(corrupt).failed() && p.getUserPresets().size()==1,"non-finite parameter values are rejected");

    PresetFolder legacyFolder;
    require(legacyFolder.file.createDirectory().wasOk(),"create legacy library fixture");
    auto oldPreset=juce::XmlDocument::parse(originalBytes);
    oldPreset->setAttribute("pluginVersion","0.2.1");
    auto* oldState=oldPreset->getFirstChildElement(); oldState->setAttribute("schemaVersion",1);
    oldState->removeChildElement(oldState->getChildByAttribute("id","saturation"),true);
    oldState->removeChildElement(oldState->getChildByAttribute("id","saturationType"),true);
    const auto oldFile=legacyFolder.file.getChildFile("old.kbasspreset");
    require(oldPreset->writeTo(oldFile),"write 0.2.x user preset fixture");
    KBassProcessor oldReader(legacyFolder.file);
    setValue(oldReader,"saturation",1); setValue(oldReader,"saturationType",2);
    require(oldReader.getUserPresets().size()==1 && oldReader.loadUserPreset(oldFile).wasOk(),"legacy user presets remain listed and loadable");
    require(value(oldReader,"saturation")==0 && value(oldReader,"saturationType")==0 && value(oldReader,"cutoff")==432,"legacy user preset migration bypasses saturation and retains the patch");

    // Factory and user selectors are independent controls beside Save.
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    juce::ComboBox* factory=nullptr;
    juce::ComboBox* menu=nullptr;
    juce::TextButton* saveButton=nullptr;
    for(auto* child:editor->getChildren()) {
        if(child->getName()=="FACTORY PRESETS") factory=dynamic_cast<juce::ComboBox*>(child);
        if(child->getName()=="USER PRESETS") menu=dynamic_cast<juce::ComboBox*>(child);
        if(child->getName()=="SAVE PRESET") saveButton=dynamic_cast<juce::TextButton*>(child);
    }
    require(factory && menu && saveButton,"separate factory/user menus and Save exist");
    require(factory->getNumItems()==6 && menu->getNumItems()==1,"factory and valid user sounds occupy separate menus");
    require(menu->getText()=="Night Bass" && menu->getSelectedId()==1,"saved user sound is selected");
    require(factory->getRight()<menu->getX() && menu->getRight()<saveButton->getX(),"factory, user and Save are adjacent without overlap");
    require(factory->getY()==menu->getY() && menu->getY()>720,"preset menus occupy the requested bottom strip");
    factory->setSelectedId(1,juce::sendNotificationSync);
    require(p.getCurrentUserPresetName().isEmpty() && std::abs(value(p,"cutoff")-650)<.01f,"factory menu recalls its sound");
    setValue(p,"cutoff",1100);
    static_cast<PresetComboBox*>(factory)->preparePopup();
    for(juce::PopupMenu::MenuItemIterator item(*factory->getRootMenu());item.next();)
        if(item.getItem().itemID==factory->getSelectedId()) item.getItem().action();
    require(std::abs(value(p,"cutoff")-650)<.01f,"reselecting the current factory preset resets edited knobs");
    menu->setSelectedId(1,juce::sendNotificationSync);
    require(p.getCurrentUserPresetName()=="Night Bass" && std::abs(value(p,"cutoff")-987)<.01f,"user menu recalls the saved sound");
    setValue(p,"cutoff",2200);
    static_cast<PresetComboBox*>(menu)->preparePopup();
    for(juce::PopupMenu::MenuItemIterator item(*menu->getRootMenu());item.next();)
        if(item.getItem().itemID==menu->getSelectedId()) item.getItem().action();
    require(std::abs(value(p,"cutoff")-987)<.01f,"reselecting the current user preset restores the saved patch");
   #if JUCE_WINDOWS
    // Exercise the real dialog on the supported plugin platform; portable
    // storage/menu tests above also run without a Linux window manager.
    saveButton->onClick();
    auto* dialog=dynamic_cast<juce::AlertWindow*>(juce::ModalComponentManager::getInstance()->getModalComponent(0));
    require(dialog && dialog->getTextEditor("name"),"Save Preset opens the name-entry dialog");
    dialog->getTextEditor("name")->setText("From UI",false);
    dialog->exitModalState(1);
    juce::Timer::callAfterDelay(100,[] { juce::MessageManager::getInstance()->stopDispatchLoop(); });
    juce::MessageManager::getInstance()->runDispatchLoop();
    require(p.userPresetExists("From UI") && p.getCurrentUserPresetName()=="From UI","Save dialog callback persists the new preset");
    require(menu->getSelectedId()>=1 && menu->getText()=="From UI","new user preset is selected immediately");
   #endif
    std::cout << "PASS: user preset persistence, replacement, validation, project recall and preset menu\n";
}
