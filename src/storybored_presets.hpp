#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <map>
#include <vector>

using namespace juce;

/**
 * StoryBored JSON preset loader
 * Loads presets from StoryBored's custom JSON format and applies them to VST3 plugins
 */
class StoryBoredPresetLoader {
public:
    struct PresetMetadata {
        String name;
        String category;
        String author;
        String description;
        String pluginVersion;
        String parameterSchemaVersion;
        StringArray tags;
        String created;
        String modified;
    };
    
    struct PresetData {
        PresetMetadata metadata;
        std::map<String, float> parameters;
        bool isValid = false;
    };
    
    /**
     * Load a StoryBored JSON preset file
     * 
     * @param presetPath Path to the .json preset file
     * @return PresetData structure with metadata and parameters
     */
    static PresetData loadPreset(const String& presetPath) {
        PresetData result;
        
        File presetFile(presetPath);
        if (!presetFile.existsAsFile()) {
            std::cerr << "ERROR: Preset file not found: " << presetPath << "\n";
            return result;
        }
        
        // Read file content
        String jsonContent = presetFile.loadFileAsString();
        if (jsonContent.isEmpty()) {
            std::cerr << "ERROR: Failed to read preset file: " << presetPath << "\n";
            return result;
        }
        
        // Parse JSON using JUCE's JSON parser
        var jsonData;
        Result parseResult = JSON::parse(jsonContent, jsonData);
        if (parseResult.failed()) {
            std::cerr << "ERROR: Failed to parse JSON: " << parseResult.getErrorMessage() << "\n";
            return result;
        }
        
        // Extract preset object
        if (!jsonData.isObject() || !jsonData.hasProperty("preset")) {
            std::cerr << "ERROR: Invalid preset format - missing 'preset' root object\n";
            return result;
        }
        
        var preset = jsonData["preset"];
        
        // Extract metadata
        if (preset.hasProperty("metadata")) {
            var metadata = preset["metadata"];
            result.metadata.name = metadata.getProperty("name", "Unnamed").toString();
            result.metadata.category = metadata.getProperty("category", "").toString();
            result.metadata.author = metadata.getProperty("author", "").toString();
            result.metadata.description = metadata.getProperty("description", "").toString();
            result.metadata.pluginVersion = metadata.getProperty("pluginVersion", "").toString();
            result.metadata.parameterSchemaVersion = metadata.getProperty("parameterSchemaVersion", "").toString();
            result.metadata.created = metadata.getProperty("created", "").toString();
            result.metadata.modified = metadata.getProperty("modified", "").toString();
            
            // Extract tags array
            if (metadata.hasProperty("tags") && metadata["tags"].isArray()) {
                Array<var>* tagsArray = metadata["tags"].getArray();
                if (tagsArray) {
                    for (const auto& tag : *tagsArray) {
                        result.metadata.tags.add(tag.toString());
                    }
                }
            }
        }
        
        // Extract parameters
        if (preset.hasProperty("parameters")) {
            var parameters = preset["parameters"];
            if (parameters.isObject()) {
                DynamicObject* paramObj = parameters.getDynamicObject();
                if (paramObj) {
                    auto& properties = paramObj->getProperties();
                    for (int i = 0; i < properties.size(); ++i) {
                        String paramName = properties.getName(i).toString();
                        var paramValue = properties.getValueAt(i);
                        
                        // Convert to float (handles both int and float JSON values)
                        float value = static_cast<float>(paramValue);
                        result.parameters[paramName] = value;
                    }
                }
            }
        }
        
        result.isValid = !result.parameters.empty();
        return result;
    }
    
    /**
     * Apply preset parameters to a plugin instance
     * 
     * @param plugin The plugin instance to apply parameters to
     * @param presetData The preset data to apply
     * @param verbose If true, print detailed parameter application info
     * @return Number of parameters successfully applied
     */
    static int applyPresetToPlugin(AudioPluginInstance& plugin, 
                                   const PresetData& presetData,
                                   bool verbose = false) {
        if (!presetData.isValid) {
            std::cerr << "ERROR: Invalid preset data\n";
            return 0;
        }
        
        int appliedCount = 0;
        int notFoundCount = 0;
        
        if (verbose) {
            std::cout << "\n=== Applying Preset: " << presetData.metadata.name << " ===\n";
            std::cout << "Category: " << presetData.metadata.category << "\n";
            std::cout << "Total parameters in preset: " << presetData.parameters.size() << "\n\n";
        }
        
        // Get all plugin parameters
        auto& pluginParams = plugin.getParameters();
        
        // Build a map of parameter IDs/names for quick lookup
        std::map<String, AudioProcessorParameter*> paramMap;
        for (auto* param : pluginParams) {
            if (param) {
                // Try to get parameter ID
                String paramId = getParameterIdentifier(*param);
                if (paramId.isNotEmpty()) {
                    paramMap[paramId] = param;
                }
                // Also map by name as fallback
                String paramName = param->getName(100);
                if (paramName.isNotEmpty()) {
                    paramMap[paramName] = param;
                }
            }
        }
        
        // Apply each parameter from preset
        for (const auto& [paramName, paramValue] : presetData.parameters) {
            // Try to find parameter using multiple strategies:
            // 1. Direct match with JSON parameter name
            // 2. Mapped display name (e.g., "clock_speed_0" -> "CLOCK SPEED L")
            // 3. Original parameter ID
            
            AudioProcessorParameter* param = nullptr;
            String matchedName = "";
            
            // Strategy 1: Direct match
            auto it = paramMap.find(paramName);
            if (it != paramMap.end()) {
                param = it->second;
                matchedName = paramName;
            }
            
            // Strategy 2: Try mapped display name
            if (!param) {
                String displayName = jsonParamIdToDisplayName(paramName);
                it = paramMap.find(displayName);
                if (it != paramMap.end()) {
                    param = it->second;
                    matchedName = displayName;
                }
            }
            
            if (param) {
                // Set the parameter value (already normalized 0-1)
                param->setValue(paramValue);
                
                if (verbose) {
                    std::cout << "✅ " << paramName.toStdString();
                    if (matchedName != paramName) {
                        std::cout << " → " << matchedName.toStdString();
                    }
                    std::cout << " = " << paramValue 
                             << " (" << param->getText(paramValue, 100).toStdString() << ")\n";
                }
                appliedCount++;
            } else {
                if (verbose) {
                    String displayName = jsonParamIdToDisplayName(paramName);
                    std::cout << "⚠️  " << paramName.toStdString();
                    if (displayName != paramName) {
                        std::cout << " (tried: " << displayName.toStdString() << ")";
                    }
                    std::cout << " = " << paramValue 
                             << " (parameter not found in plugin)\n";
                }
                notFoundCount++;
            }
        }
        
        if (verbose) {
            std::cout << "\n=== Summary ===\n";
            std::cout << "Applied: " << appliedCount << " parameters\n";
            if (notFoundCount > 0) {
                std::cout << "Not found: " << notFoundCount << " parameters\n";
            }
            std::cout << "\n";
        }
        
        return appliedCount;
    }
    
    /**
     * Print preset information without applying it
     * 
     * @param presetPath Path to the preset file
     */
    static void printPresetInfo(const String& presetPath) {
        PresetData preset = loadPreset(presetPath);
        
        if (!preset.isValid) {
            std::cerr << "ERROR: Failed to load preset\n";
            return;
        }
        
        std::cout << "\n=== Preset Information ===\n";
        std::cout << "File: " << File(presetPath).getFileName() << "\n";
        std::cout << "Name: " << preset.metadata.name << "\n";
        std::cout << "Category: " << preset.metadata.category << "\n";
        std::cout << "Author: " << preset.metadata.author << "\n";
        std::cout << "Description: " << preset.metadata.description << "\n";
        std::cout << "Plugin Version: " << preset.metadata.pluginVersion << "\n";
        std::cout << "Schema Version: " << preset.metadata.parameterSchemaVersion << "\n";
        
        if (!preset.metadata.tags.isEmpty()) {
            std::cout << "Tags: " << preset.metadata.tags.joinIntoString(", ") << "\n";
        }
        
        std::cout << "Created: " << preset.metadata.created << "\n";
        std::cout << "Modified: " << preset.metadata.modified << "\n";
        std::cout << "\nParameters: " << preset.parameters.size() << "\n";
        
        // Print first few parameters as sample
        int count = 0;
        for (const auto& [name, value] : preset.parameters) {
            std::cout << "  " << name << " = " << value << "\n";
            if (++count >= 10) {
                std::cout << "  ... (" << (preset.parameters.size() - 10) << " more)\n";
                break;
            }
        }
        std::cout << "\n";
    }

private:
    /**
     * Get parameter identifier (ID or name)
     * Helper function to extract parameter ID from AudioProcessorParameter
     */
    static String getParameterIdentifier(AudioProcessorParameter& param) {
        // Try to get parameter ID via getName with large buffer
        String paramName = param.getName(1000);
        
        // For JUCE parameters, the ID might be accessible via paramID
        // This is a best-effort approach
        if (auto* paramWithID = dynamic_cast<AudioProcessorParameterWithID*>(&param)) {
            return paramWithID->paramID;
        }
        
        return paramName;
    }
    
    /**
     * Convert JSON parameter ID to VST3 display name
     * Based on EKKOHAUS parameter naming conventions
     * 
     * @param paramId JSON parameter ID (e.g., "clock_speed_0")
     * @return VST3 display name (e.g., "CLOCK SPEED L")
     */
    static String jsonParamIdToDisplayName(const String& paramId) {
        String baseName = paramId;
        String channelSuffix = "";
        
        // Handle channel suffix mapping: _0 -> L, _1 -> R
        if (paramId.endsWith("_0")) {
            baseName = paramId.dropLastCharacters(2);
            channelSuffix = " L";
        } else if (paramId.endsWith("_1")) {
            baseName = paramId.dropLastCharacters(2);
            channelSuffix = " R";
        }
        
        // Convert snake_case to display format (UPPER CASE)
        String displayBase = baseName.replace("_", " ").toUpperCase();
        
        // Special cases for Title Case parameters
        if (baseName.equalsIgnoreCase("dotted") || baseName.equalsIgnoreCase("triplet")) {
            displayBase = baseName.replace("_", " ");
            displayBase = displayBase.substring(0, 1).toUpperCase() + displayBase.substring(1).toLowerCase();
        }
        
        // Special name mappings from EKKOHAUS master_parameters.json
        static const std::map<String, String> nameMappings = {
            // Reverb/Diffusion parameters
            {"DIFFUSION", "REVERB AMOUNT"},
            {"DIFFUSION MIX FACTOR", "DECAY"},
            {"DIFFUSION MAX GAIN", "ROOMSIZE"},
            {"DIFFUSION LENGTH SCALE", "PREDELAY"},
            {"DIFFUSION HF DAMPING", "LOFI"},
            
            // Multi-tap parameters
            {"TAP 1 ENABLED", "Tap 1 (16th)"},
            {"TAP 2 ENABLED", "Tap 2 (8th)"},
            {"TAP 3 ENABLED", "Tap 3 (Dot 8th)"},
            {"TAP 4 ENABLED", "Tap 4 (1/4)"},
            
            // Global parameters
            {"STEREO LINK", "Stereo Link"},
            {"MULTI TAP MODE", "Multi-Tap Mode"},
            {"BUCKET COUNT", "Bucket Count"},
            {"CLOCK DIVIDER", "Clock Divider"},
            {"TEMPO SYNC ENABLED", "Tempo Sync"},
            {"PING PONG ENABLED", "Ping Pong Mode"},
            {"FEEDBACK MODE", "Swell Mode"},
            {"MIX", "Wet/Dry Mix"},
            {"REVERB POSITION", "Reverb Position"},
            {"DELAY SHIFT", "DELAYSHIFT"},
            {"STEREO WIDTH", "WIDTH"},
            
            // LFO parameters
            {"LFO SPEED", "LFO Speed"},
            {"LFO DEPTH", "LFO Depth"},
            {"LFO PHASE", "LFO Phase"},
            {"LFO SEED", "LFO Seed"},
            {"LFO SHAPE", "LFO Shape"},
            {"LFO SPEED RANGE", "LFO Speed Range"},
            
            // Oversampling parameters
            {"OS FACTOR", "OS Factor"},
            {"OS MODE", "OS Mode"},
            
            // Marshall Mode parameters
            {"MARSHALL MODE ENABLED", "Marshall Mode"},
            {"MARSHALL PRESET", "Marshall Preset"},
            {"CCD DIGITAL CORE", "CCD Mode"},
            
            // Additional parameters
            {"FREEZE MODE", "Freeze"},
            {"CAPTURE BUFFER", "Capture"},
            {"MONO INPUT", "Mono Input"},
            {"DRY DELAY COMPENSATION", "Through-Zero Delay"},
            {"PHASE INVERT B", "Phase Invert B"},
            
            // Note value parameters
            {"NOTE VALUE", "Note Value"},
        };
        
        auto it = nameMappings.find(displayBase);
        if (it != nameMappings.end()) {
            displayBase = it->second;
        }
        
        return displayBase + channelSuffix;
    }
};
