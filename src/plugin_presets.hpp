#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

using namespace juce;

/**
 * Plugin preset loader and saver for VST3 .vstpreset files
 */
class PluginPresetManager {
public:
    /**
     * Load a VST3 preset file into a plugin instance
     * 
     * @param plugin The plugin instance to load the preset into
     * @param presetPath Path to the .vstpreset file
     * @return true if successful, false otherwise
     */
    static bool loadPreset(AudioPluginInstance& plugin, const String& presetPath) {
        File presetFile(presetPath);
        
        if (!presetFile.existsAsFile()) {
            std::cerr << "ERROR: Preset file not found: " << presetPath << "\n";
            return false;
        }
        
        // Read the preset file
        MemoryBlock presetData;
        if (!presetFile.loadFileAsData(presetData)) {
            std::cerr << "ERROR: Failed to read preset file: " << presetPath << "\n";
            return false;
        }
        
        // Try to set the state from the preset data
        plugin.setStateInformation(presetData.getData(), (int)presetData.getSize());
        
        return true;
    }
    
    /**
     * Save the current plugin state to a VST3 preset file
     * 
     * @param plugin The plugin instance to save
     * @param presetPath Path where the .vstpreset file will be saved
     * @return true if successful, false otherwise
     */
    static bool savePreset(AudioPluginInstance& plugin, const String& presetPath) {
        File presetFile(presetPath);
        
        // Get the plugin state
        MemoryBlock stateData;
        plugin.getStateInformation(stateData);
        
        if (stateData.isEmpty()) {
            std::cerr << "ERROR: Plugin state is empty\n";
            return false;
        }
        
        // Write to file
        if (!presetFile.replaceWithData(stateData.getData(), stateData.getSize())) {
            std::cerr << "ERROR: Failed to write preset file: " << presetPath << "\n";
            return false;
        }
        
        return true;
    }
    
    /**
     * Get the current plugin state as a base64-encoded string
     * Useful for storing state in text files or databases
     * 
     * @param plugin The plugin instance
     * @return Base64-encoded state string
     */
    static String getStateAsBase64(AudioPluginInstance& plugin) {
        MemoryBlock stateData;
        plugin.getStateInformation(stateData);
        return stateData.toBase64Encoding();
    }
    
    /**
     * Set plugin state from a base64-encoded string
     * 
     * @param plugin The plugin instance
     * @param base64State Base64-encoded state string
     * @return true if successful, false otherwise
     */
    static bool setStateFromBase64(AudioPluginInstance& plugin, const String& base64State) {
        MemoryBlock stateData;
        if (!stateData.fromBase64Encoding(base64State)) {
            std::cerr << "ERROR: Failed to decode base64 state\n";
            return false;
        }
        
        plugin.setStateInformation(stateData.getData(), (int)stateData.getSize());
        return true;
    }
    
    /**
     * Compare two plugin states to see if they're identical
     * 
     * @param plugin1 First plugin instance
     * @param plugin2 Second plugin instance
     * @return true if states are identical, false otherwise
     */
    static bool compareStates(AudioPluginInstance& plugin1, AudioPluginInstance& plugin2) {
        MemoryBlock state1, state2;
        plugin1.getStateInformation(state1);
        plugin2.getStateInformation(state2);
        
        return state1 == state2;
    }
    
    /**
     * Print information about a preset file
     * 
     * @param presetPath Path to the .vstpreset file
     */
    static void printPresetInfo(const String& presetPath) {
        File presetFile(presetPath);
        
        if (!presetFile.existsAsFile()) {
            std::cerr << "ERROR: Preset file not found: " << presetPath << "\n";
            return;
        }
        
        std::cout << "Preset File: " << presetFile.getFileName() << "\n";
        std::cout << "Path: " << presetFile.getFullPathName() << "\n";
        std::cout << "Size: " << presetFile.getSize() << " bytes\n";
        std::cout << "Modified: " << presetFile.getLastModificationTime().toString(true, true) << "\n";
        
        // Try to read and show basic info
        MemoryBlock presetData;
        if (presetFile.loadFileAsData(presetData)) {
            std::cout << "Data size: " << presetData.getSize() << " bytes\n";
            
            // Check if it looks like XML (some VST3 presets are XML-based)
            String dataAsString = presetData.toString();
            if (dataAsString.contains("<?xml") || dataAsString.contains("<VST3")) {
                std::cout << "Format: XML-based VST3 preset\n";
            } else {
                std::cout << "Format: Binary VST3 preset\n";
            }
        }
    }
    
    /**
     * Create a preset from current plugin state with metadata
     * 
     * @param plugin The plugin instance
     * @param presetPath Path where the preset will be saved
     * @param presetName Name for the preset
     * @param author Author name
     * @return true if successful, false otherwise
     */
    static bool createPresetWithMetadata(AudioPluginInstance& plugin, 
                                         const String& presetPath,
                                         const String& presetName,
                                         const String& author = "") {
        // Get current state
        MemoryBlock stateData;
        plugin.getStateInformation(stateData);
        
        if (stateData.isEmpty()) {
            std::cerr << "ERROR: Plugin state is empty\n";
            return false;
        }
        
        // Create XML wrapper with metadata
        std::unique_ptr<XmlElement> preset(new XmlElement("VST3Preset"));
        preset->setAttribute("name", presetName);
        preset->setAttribute("pluginName", plugin.getName());
        
        if (author.isNotEmpty())
            preset->setAttribute("author", author);
        
        preset->setAttribute("created", Time::getCurrentTime().toString(true, true));
        
        // Add state data as base64
        auto* stateElement = preset->createNewChildElement("PluginState");
        stateElement->setAttribute("data", stateData.toBase64Encoding());
        
        // Write to file
        File presetFile(presetPath);
        if (!preset->writeTo(presetFile)) {
            std::cerr << "ERROR: Failed to write preset file: " << presetPath << "\n";
            return false;
        }
        
        return true;
    }
    
    /**
     * Load a preset with metadata
     * 
     * @param plugin The plugin instance
     * @param presetPath Path to the preset file
     * @param outMetadata Optional pointer to receive metadata
     * @return true if successful, false otherwise
     */
    static bool loadPresetWithMetadata(AudioPluginInstance& plugin, 
                                       const String& presetPath,
                                       StringPairArray* outMetadata = nullptr) {
        File presetFile(presetPath);
        
        if (!presetFile.existsAsFile()) {
            std::cerr << "ERROR: Preset file not found: " << presetPath << "\n";
            return false;
        }
        
        // Try to parse as XML first
        std::unique_ptr<XmlElement> preset = XmlDocument::parse(presetFile);
        
        if (preset && preset->hasTagName("VST3Preset")) {
            // Extract metadata
            if (outMetadata) {
                outMetadata->set("name", preset->getStringAttribute("name"));
                outMetadata->set("pluginName", preset->getStringAttribute("pluginName"));
                outMetadata->set("author", preset->getStringAttribute("author"));
                outMetadata->set("created", preset->getStringAttribute("created"));
            }
            
            // Get state data
            auto* stateElement = preset->getChildByName("PluginState");
            if (stateElement) {
                String base64Data = stateElement->getStringAttribute("data");
                return setStateFromBase64(plugin, base64Data);
            }
        }
        
        // Fallback to raw binary preset
        return loadPreset(plugin, presetPath);
    }
};
