#include "PluginProcessor.h"
#include <algorithm>
#include <cmath>

juce::File KBassProcessor::defaultUserPresetFolder()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Independent Audio").getChildFile("KBass").getChildFile("Presets");
}
bool KBassProcessor::isValidUserPresetName(const juce::String& name)
{
    if (name.isEmpty() || name.length()>64 || name!=name.trim() || name=="." || name==".."
        || name.containsAnyOf("/\\:*?\"<>|")) return false;
    for (const auto c : name) if (c<32 || c==127) return false;
    return true;
}
juce::String KBassProcessor::getCurrentUserPresetName() const
{
    const juce::ScopedLock lock(presetNameLock);
    return currentUserPresetName;
}
void KBassProcessor::setUserPresetName(const juce::String& name)
{
    const juce::ScopedLock lock(presetNameLock);
    currentUserPresetName=name;
}
juce::Result KBassProcessor::readUserPreset(const juce::File& file,juce::ValueTree& state,juce::String& name) const
{
    if (!file.existsAsFile() || file.getSize()>65536 || file.getSize()==0)
        return juce::Result::fail("The preset file is missing or invalid.");
    const auto xml=juce::XmlDocument::parse(file.loadFileAsString());
    if (!xml || !xml->hasTagName("KBassUserPreset") || xml->getIntAttribute("formatVersion")!=1)
        return juce::Result::fail("This is not a supported KBass user preset.");
    name=xml->getStringAttribute("name");
    if (!isValidUserPresetName(name) || xml->getNumChildElements()!=1)
        return juce::Result::fail("The preset name or contents are invalid.");
    state=juce::ValueTree::fromXml(*xml->getFirstChildElement());
    if (!state.hasType("KBassState") || static_cast<int>(state.getProperty("schemaVersion"))!=1
        || state.getNumChildren()!=static_cast<int>(parameterIds.size()))
        return juce::Result::fail("The preset state is incomplete or unsupported.");
    for (auto id : parameterIds)
    {
        const auto child=state.getChildWithProperty("id",id);
        const auto number=juce::JSON::fromString(child.getProperty("value").toString());
        if (!child.isValid()
            || !(number.isInt() || number.isInt64() || number.isDouble()))
            return juce::Result::fail("The preset contains an invalid parameter.");
        const double value=static_cast<double>(number);
        const auto* parameter=parameters.getParameter(id);
        const float storedValue=static_cast<float>(value);
        if (!std::isfinite(value) || !std::isfinite(storedValue) || storedValue<parameter->convertFrom0to1(0)
            || storedValue>parameter->convertFrom0to1(1)
            || ((juce::String(id)=="wave1" || juce::String(id)=="wave2" || juce::String(id)=="octave" || juce::String(id)=="envTarget" || juce::String(id)=="lfoWave") && value!=std::floor(value)))
            return juce::Result::fail("The preset contains a parameter outside its valid range.");
    }
    return juce::Result::ok();
}
std::vector<KBassProcessor::UserPreset> KBassProcessor::getUserPresets() const
{
    std::vector<UserPreset> result;
    // Scanning and file I/O are called by the editor, never by processBlock.
    for (const auto& file : userPresetFolder.findChildFiles(juce::File::findFiles,false,"*.kbasspreset"))
    {
        juce::ValueTree state; juce::String name;
        if (readUserPreset(file,state,name).wasOk()) result.push_back({name,file});
    }
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b)
    { return a.name.compareNatural(b.name,false)<0; });
    return result;
}
bool KBassProcessor::userPresetExists(const juce::String& name) const
{
    for (const auto& preset : getUserPresets()) if (preset.name.equalsIgnoreCase(name.trim())) return true;
    return false;
}
juce::Result KBassProcessor::saveUserPreset(const juce::String& requestedName,bool replaceExisting)
{
    const auto name=requestedName.trim();
    if (!isValidUserPresetName(name))
        return juce::Result::fail("Use a name of 1 to 64 characters without / \\ : * ? \" < > |.");
    juce::File destination;
    for (const auto& preset : getUserPresets())
        if (preset.name.equalsIgnoreCase(name)) { destination=preset.file; break; }
    if (destination!=juce::File{} && !replaceExisting)
        return juce::Result::fail("A user preset with this name already exists.");
    if (const auto result=userPresetFolder.createDirectory(); result.failed()) return result;
    // UUID filenames keep display names separate from filesystem paths.
    if (destination==juce::File{}) destination=userPresetFolder.getChildFile(juce::Uuid().toString()+".kbasspreset");
    auto state=parameters.copyState();
    state.setProperty("schemaVersion",1,nullptr);
    state.setProperty("program",currentProgram.load(),nullptr);
    state.setProperty("userPresetName",name,nullptr);
    juce::XmlElement document("KBassUserPreset");
    document.setAttribute("formatVersion",1);
    document.setAttribute("name",name);
    document.setAttribute("pluginVersion",JucePlugin_VersionString);
    document.addChildElement(state.createXml().release());
    juce::TemporaryFile temporary(destination);
    if (!document.writeTo(temporary.getFile()) || !temporary.overwriteTargetFileWithTemporary())
        return juce::Result::fail("The preset could not be written. Check that the preset folder is writable.");
    setUserPresetName(name);
    updateHostDisplay(juce::AudioProcessorListener::ChangeDetails{}.withNonParameterStateChanged(true));
    return juce::Result::ok();
}
juce::Result KBassProcessor::loadUserPreset(const juce::File& file)
{
    juce::ValueTree state; juce::String name;
    if (const auto result=readUserPreset(file,state,name); result.failed()) return result;
    state.setProperty("userPresetName",name,nullptr);
    juce::MemoryBlock data;
    copyXmlToBinary(*state.createXml(),data);
    setStateInformation(data.getData(),static_cast<int>(data.getSize()));
    updateHostDisplay(juce::AudioProcessorListener::ChangeDetails{}.withNonParameterStateChanged(true).withProgramChanged(true));
    return juce::Result::ok();
}
