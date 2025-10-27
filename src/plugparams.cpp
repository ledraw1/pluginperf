#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <string>

#include "plugin_params.hpp"
#include "plugin_presets.hpp"

using namespace juce;

void printUsage() {
    std::cout << R"(
plugparams - Plugin Parameter Inspector and Manipulator

Usage:
  plugparams --plugin <path> [options]

Options:
  --plugin PATH           Path to VST3 plugin (required)
  --list                  List all parameters (default if no other action)
  --verbose               Show detailed parameter information
  --get INDEX|NAME        Get value of specific parameter
  --set INDEX|NAME=VALUE  Set parameter value (0.0-1.0 normalized)
  --load-preset PATH      Load VST3 preset file (.vstpreset)
  --save-preset PATH      Save current state to preset file
  --preset-info PATH      Show information about a preset file
  --json                  Output in JSON format
  
Examples:
  # List all parameters
  plugparams --plugin plugin.vst3
  
  # List with detailed info
  plugparams --plugin plugin.vst3 --verbose
  
  # Get specific parameter
  plugparams --plugin plugin.vst3 --get 0
  plugparams --plugin plugin.vst3 --get "Gain"
  
  # Set parameter value
  plugparams --plugin plugin.vst3 --set 0=0.5
  plugparams --plugin plugin.vst3 --set "Gain=0.75"
  
  # Set multiple parameters
  plugparams --plugin plugin.vst3 --set "Gain=0.5" --set "Mix=1.0"
  
  # JSON output
  plugparams --plugin plugin.vst3 --json
  
  # Load preset
  plugparams --plugin plugin.vst3 --load-preset preset.vstpreset
  
  # Save preset
  plugparams --plugin plugin.vst3 --save-preset my_preset.vstpreset
  
  # Preset info
  plugparams --preset-info preset.vstpreset

)" << std::endl;
}

struct Args {
    String pluginPath;
    String loadPresetPath;
    String savePresetPath;
    String presetInfoPath;
    bool listParams = false;
    bool verbose = false;
    bool jsonOutput = false;
    std::vector<String> getParams;
    std::vector<std::pair<String, float>> setParams;
};

bool parseArgs(int argc, char** argv, Args& args) {
    for (int i = 1; i < argc; ++i) {
        String arg(argv[i]);
        
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return false;
        } else if (arg == "--plugin" && i + 1 < argc) {
            args.pluginPath = argv[++i];
        } else if (arg == "--list") {
            args.listParams = true;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (arg == "--json") {
            args.jsonOutput = true;
        } else if (arg == "--load-preset" && i + 1 < argc) {
            args.loadPresetPath = argv[++i];
        } else if (arg == "--save-preset" && i + 1 < argc) {
            args.savePresetPath = argv[++i];
        } else if (arg == "--preset-info" && i + 1 < argc) {
            args.presetInfoPath = argv[++i];
        } else if (arg == "--get" && i + 1 < argc) {
            args.getParams.push_back(argv[++i]);
        } else if (arg == "--set" && i + 1 < argc) {
            String setArg(argv[++i]);
            int eqPos = setArg.indexOfChar('=');
            if (eqPos > 0) {
                String paramName = setArg.substring(0, eqPos);
                float value = setArg.substring(eqPos + 1).getFloatValue();
                args.setParams.push_back({paramName, value});
            } else {
                std::cerr << "ERROR: Invalid --set format. Use: --set NAME=VALUE\n";
                return false;
            }
        }
    }
    
    // Allow preset-info without plugin
    if (args.pluginPath.isEmpty() && args.presetInfoPath.isEmpty()) {
        std::cerr << "ERROR: --plugin is required\n";
        printUsage();
        return false;
    }
    
    // Default to list if no action specified
    if (args.getParams.empty() && args.setParams.empty() && 
        args.loadPresetPath.isEmpty() && args.savePresetPath.isEmpty()) {
        args.listParams = true;
    }
    
    return true;
}

void outputJSON(const std::vector<ParameterInfo>& params) {
    std::cout << "{\n";
    std::cout << "  \"parameters\": [\n";
    
    for (size_t i = 0; i < params.size(); ++i) {
        const auto& p = params[i];
        std::cout << "    {\n";
        std::cout << "      \"index\": " << p.index << ",\n";
        std::cout << "      \"id\": \"" << p.id.replace("\"", "\\\"") << "\",\n";
        std::cout << "      \"name\": \"" << p.name.replace("\"", "\\\"") << "\",\n";
        std::cout << "      \"type\": \"" << PluginParameterManager::getTypeString(p.type) << "\",\n";
        std::cout << "      \"currentValue\": " << p.currentValue << ",\n";
        std::cout << "      \"currentValueText\": \"" << p.getCurrentValueText().replace("\"", "\\\"") << "\",\n";
        std::cout << "      \"defaultValue\": " << p.defaultValue << ",\n";
        std::cout << "      \"label\": \"" << p.label.replace("\"", "\\\"") << "\",\n";
        
        if (p.type == ParameterType::Continuous) {
            std::cout << "      \"minValue\": " << p.minValue << ",\n";
            std::cout << "      \"maxValue\": " << p.maxValue << ",\n";
        }
        
        if (p.type == ParameterType::Discrete || p.type == ParameterType::Boolean) {
            std::cout << "      \"numSteps\": " << p.numSteps << ",\n";
            if (!p.valueStrings.empty()) {
                std::cout << "      \"values\": [";
                for (size_t j = 0; j < p.valueStrings.size(); ++j) {
                    if (j > 0) std::cout << ", ";
                    std::cout << "\"" << p.valueStrings[j].replace("\"", "\\\"") << "\"";
                }
                std::cout << "],\n";
            }
        }
        
        std::cout << "      \"automatable\": " << (p.isAutomatable ? "true" : "false") << ",\n";
        std::cout << "      \"metaParameter\": " << (p.isMetaParameter ? "true" : "false") << "\n";
        std::cout << "    }";
        if (i < params.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    
    std::cout << "  ]\n";
    std::cout << "}\n";
}

int main(int argc, char** argv) {
    Args args;
    if (!parseArgs(argc, argv, args))
        return argc <= 1 ? 0 : 1;
    
    // Handle preset-info without loading plugin
    if (!args.presetInfoPath.isEmpty()) {
        PluginPresetManager::printPresetInfo(args.presetInfoPath);
        return 0;
    }
    
    // Initialize JUCE
    MessageManager::getInstance();
    
    // Load plugin
    AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();
    
    AudioPluginFormat* vst3Format = nullptr;
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto* format = formatManager.getFormat(i);
        if (format->getName() == "VST3") {
            vst3Format = format;
            break;
        }
    }
    
    if (!vst3Format) {
        std::cerr << "ERROR: VST3 format not available\n";
        return 1;
    }
    
    // Scan the plugin file to get proper description
    OwnedArray<PluginDescription> foundTypes;
    vst3Format->findAllTypesForFile(foundTypes, args.pluginPath);
    
    if (foundTypes.isEmpty()) {
        std::cerr << "ERROR: No plugins found in file\n";
        std::cerr << "Path: " << args.pluginPath << "\n";
        return 1;
    }
    
    // Use the first plugin found
    PluginDescription desc = *foundTypes[0];
    
    String errorMessage;
    auto instance = vst3Format->createInstanceFromDescription(
        desc, 48000.0, 512, errorMessage
    );
    
    if (!instance) {
        std::cerr << "ERROR: Failed to load plugin\n";
        std::cerr << "Path: " << args.pluginPath << "\n";
        std::cerr << "Reason: " << errorMessage << "\n";
        return 1;
    }
    
    auto* plugin = instance.get();
    
    // Prepare plugin for processing
    plugin->prepareToPlay(48000.0, 512);
    
    // Handle --load-preset first (before any parameter operations)
    if (!args.loadPresetPath.isEmpty()) {
        StringPairArray metadata;
        if (PluginPresetManager::loadPresetWithMetadata(*plugin, args.loadPresetPath, &metadata)) {
            std::cout << "✓ Loaded preset: " << args.loadPresetPath << "\n";
            if (metadata.getValue("name", "").isNotEmpty()) {
                std::cout << "  Name: " << metadata.getValue("name", "") << "\n";
            }
            if (metadata.getValue("author", "").isNotEmpty()) {
                std::cout << "  Author: " << metadata.getValue("author", "") << "\n";
            }
        } else {
            std::cerr << "✗ Failed to load preset\n";
        }
    }
    
    // Query parameters
    auto params = PluginParameterManager::queryParameters(*plugin);
    
    // Handle --set operations
    for (const auto& [paramName, value] : args.setParams) {
        bool success = false;
        
        // Try as index first
        if (paramName.containsOnly("0123456789")) {
            int index = paramName.getIntValue();
            success = PluginParameterManager::setParameter(*plugin, index, value);
        } else {
            // Try by name
            success = PluginParameterManager::setParameterByName(*plugin, paramName, value);
            if (!success) {
                // Try by ID
                success = PluginParameterManager::setParameterById(*plugin, paramName, value);
            }
        }
        
        if (success) {
            std::cout << "✓ Set '" << paramName << "' = " << value << "\n";
        }
    }
    
    // Re-query if we set any parameters
    if (!args.setParams.empty()) {
        params = PluginParameterManager::queryParameters(*plugin);
    }
    
    // Handle --get operations
    for (const auto& paramName : args.getParams) {
        bool found = false;
        
        // Try as index first
        if (paramName.containsOnly("0123456789")) {
            int index = paramName.getIntValue();
            if (index >= 0 && index < (int)params.size()) {
                const auto& p = params[index];
                std::cout << p.name << " = " << p.currentValue 
                         << " (" << p.getCurrentValueText() << ")\n";
                found = true;
            }
        }
        
        // Try by name
        if (!found) {
            for (const auto& p : params) {
                if (p.name.equalsIgnoreCase(paramName) || p.id.equalsIgnoreCase(paramName)) {
                    std::cout << p.name << " = " << p.currentValue 
                             << " (" << p.getCurrentValueText() << ")\n";
                    found = true;
                    break;
                }
            }
        }
        
        if (!found) {
            std::cerr << "ERROR: Parameter '" << paramName << "' not found\n";
        }
    }
    
    // Handle --save-preset
    if (!args.savePresetPath.isEmpty()) {
        if (PluginPresetManager::createPresetWithMetadata(*plugin, args.savePresetPath, 
                                                          plugin->getName(), "PlugPerf")) {
            std::cout << "✓ Saved preset: " << args.savePresetPath << "\n";
        } else {
            std::cerr << "✗ Failed to save preset\n";
        }
    }
    
    // Handle --list or --json
    if (args.listParams) {
        if (args.jsonOutput) {
            outputJSON(params);
        } else {
            std::cout << "\nPlugin: " << plugin->getName() << "\n";
            PluginParameterManager::printParameters(params, args.verbose);
        }
    }
    
    // Cleanup
    plugin->releaseResources();
    instance.reset();
    MessageManager::deleteInstance();
    
    return 0;
}
