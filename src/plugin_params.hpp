#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <string>
#include <iostream>

using namespace juce;

/**
 * Parameter type classification based on JUCE parameter characteristics
 */
enum class ParameterType {
    Boolean,        // On/Off, True/False
    Discrete,       // ComboBox, stepped values
    Continuous,     // Slider, continuous range
    Unknown
};

/**
 * Information about a single plugin parameter
 */
struct ParameterInfo {
    int index;
    String id;              // Parameter ID (if available)
    String name;            // Display name
    String label;           // Units (dB, Hz, %, etc.)
    ParameterType type;
    
    float defaultValue;     // Normalized 0.0-1.0
    float currentValue;     // Normalized 0.0-1.0
    
    // For continuous parameters
    float minValue;
    float maxValue;
    
    // For discrete parameters
    int numSteps;           // Number of discrete steps (0 = continuous)
    std::vector<String> valueStrings;  // Text for each discrete value
    
    // Metadata
    bool isAutomatable;
    bool isMetaParameter;
    bool isOrientationInverted;
    
    /**
     * Get human-readable value string for current value
     */
    String getCurrentValueText() const {
        if (!valueStrings.empty() && numSteps > 0) {
            int step = (int)(currentValue * (numSteps - 1) + 0.5f);
            step = jlimit(0, (int)valueStrings.size() - 1, step);
            return valueStrings[step];
        }
        return String(currentValue, 3);
    }
    
    /**
     * Get human-readable value string for any normalized value
     */
    String getValueText(float normalizedValue) const {
        if (!valueStrings.empty() && numSteps > 0) {
            int step = (int)(normalizedValue * (numSteps - 1) + 0.5f);
            step = jlimit(0, (int)valueStrings.size() - 1, step);
            return valueStrings[step];
        }
        return String(normalizedValue, 3);
    }
};

/**
 * Plugin parameter inspector and manipulator
 */
class PluginParameterManager {
public:
    /**
     * Query all parameters from a plugin
     */
    static std::vector<ParameterInfo> queryParameters(AudioPluginInstance& plugin) {
        std::vector<ParameterInfo> params;
        
        auto& rawParams = plugin.getParameters();
        const int numParams = rawParams.size();
        
        for (int i = 0; i < numParams; ++i) {
            ParameterInfo info;
            info.index = i;
            
            // Try to get parameter ID from AudioProcessorParameter
            auto* param = (i < rawParams.size()) ? rawParams[i] : nullptr;
            if (param != nullptr) {
                info.name = param->getName(100);
                info.label = param->getLabel();
                info.currentValue = param->getValue();
                info.id = getParameterIdentifier(*param, i);
                info.defaultValue = param->getDefaultValue();
                info.isAutomatable = param->isAutomatable();
                info.isMetaParameter = param->isMetaParameter();
                info.isOrientationInverted = param->isOrientationInverted();
                info.numSteps = param->getNumSteps();
                
                // Classify parameter type
                info.type = classifyParameterType(*param);
                
                // Get discrete value strings if applicable
                if (info.numSteps > 0 && info.numSteps < 1000) {
                    for (int step = 0; step < info.numSteps; ++step) {
                        float normalizedValue = (float)step / (float)(info.numSteps - 1);
                        String valueText = param->getText(normalizedValue, 100);
                        info.valueStrings.push_back(valueText);
                    }
                }
                
                // Get range for continuous parameters
                if (auto* rangedParam = dynamic_cast<AudioParameterFloat*>(param)) {
                    info.minValue = rangedParam->range.start;
                    info.maxValue = rangedParam->range.end;
                } else if (auto* intParam = dynamic_cast<AudioParameterInt*>(param)) {
                    info.minValue = (float)intParam->getRange().getStart();
                    info.maxValue = (float)intParam->getRange().getEnd();
                } else {
                    info.minValue = 0.0f;
                    info.maxValue = 1.0f;
                }
            } else {
                // Fallback for legacy parameters
                info.name = String("Parameter ") + String(std::to_string(i));
                info.label = {};
                info.currentValue = 0.0f;
                info.id = String(std::to_string(i));
                info.defaultValue = 0.5f;
                info.isAutomatable = true;
                info.isMetaParameter = false;
                info.isOrientationInverted = false;
                info.numSteps = 0;
                info.type = ParameterType::Continuous;
                info.minValue = 0.0f;
                info.maxValue = 1.0f;
            }
            
            params.push_back(info);
        }
        
        return params;
    }
    
    /**
     * Set a parameter by index (normalized 0.0-1.0)
     */
    static bool setParameter(AudioPluginInstance& plugin, int index, float normalizedValue) {
        auto& params = plugin.getParameters();
        const int numParams = params.size();
        if (index < 0 || index >= numParams) {
            std::cerr << "ERROR: Parameter index " << index << " out of range (0-" 
                      << jmax(0, numParams - 1) << ")\n";
            return false;
        }
        
        normalizedValue = jlimit(0.0f, 1.0f, normalizedValue);

        if (index < params.size()) {
            if (auto* param = params[index]) {
                param->setValueNotifyingHost(normalizedValue);
                return true;
            }
        }

        // Fallback for processors that don't expose AudioProcessorParameter objects
        std::cerr << "WARNING: Parameter index " << index
                  << " is not backed by an AudioProcessorParameter; using legacy setParameter() (host automation may not be notified).\n";

        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
        plugin.setParameter(index, normalizedValue);
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE
        return true;
    }
    
    /**
     * Set a parameter by name (normalized 0.0-1.0)
     */
    static bool setParameterByName(AudioPluginInstance& plugin, const String& name, float normalizedValue) {
        auto& params = plugin.getParameters();
        for (int i = 0; i < params.size(); ++i) {
            if (auto* param = params[i]) {
                if (param->getName(100).equalsIgnoreCase(name)) {
                    return setParameter(plugin, i, normalizedValue);
                }
            }
        }
        
        std::cerr << "ERROR: Parameter '" << name << "' not found\n";
        return false;
    }
    
    /**
     * Set a parameter by ID (normalized 0.0-1.0)
     */
    static bool setParameterById(AudioPluginInstance& plugin, const String& id, float normalizedValue) {
        auto& params = plugin.getParameters();
        const int numParams = params.size();
        for (int i = 0; i < numParams; ++i) {
            if (auto* param = params[i]) {
                if (getParameterIdentifier(*param, i).equalsIgnoreCase(id)) {
                    return setParameter(plugin, i, normalizedValue);
                }
            }
        }
        
        std::cerr << "ERROR: Parameter ID '" << id << "' not found\n";
        return false;
    }
    
    /**
     * Print all parameters to console in a formatted table
     */
    static void printParameters(const std::vector<ParameterInfo>& params, bool verbose = false) {
        std::cout << "\n";
        std::cout << "Total Parameters: " << params.size() << "\n";
        std::cout << String::repeatedString("=", 100) << "\n";
        
        if (verbose) {
            // Detailed view
            for (const auto& p : params) {
                std::cout << "\nParameter #" << p.index << "\n";
                std::cout << String::repeatedString("-", 100) << "\n";
                std::cout << "  Name:         " << p.name << "\n";
                std::cout << "  ID:           " << p.id << "\n";
                std::cout << "  Type:         " << getTypeString(p.type) << "\n";
                std::cout << "  Current:      " << p.currentValue << " (" << p.getCurrentValueText() << ")\n";
                std::cout << "  Default:      " << p.defaultValue << "\n";
                
                if (p.type == ParameterType::Continuous) {
                    std::cout << "  Range:        " << p.minValue << " - " << p.maxValue;
                    if (!p.label.isEmpty())
                        std::cout << " " << p.label;
                    std::cout << "\n";
                } else if (p.type == ParameterType::Discrete && !p.valueStrings.empty()) {
                    std::cout << "  Steps:        " << p.numSteps << "\n";
                    std::cout << "  Values:       ";
                    for (int i = 0; i < jmin(5, (int)p.valueStrings.size()); ++i) {
                        if (i > 0) std::cout << ", ";
                        std::cout << p.valueStrings[i];
                    }
                    if (p.valueStrings.size() > 5)
                        std::cout << ", ... (" << p.valueStrings.size() << " total)";
                    std::cout << "\n";
                } else if (p.type == ParameterType::Boolean) {
                    std::cout << "  Values:       Off / On\n";
                }
                
                std::cout << "  Automatable:  " << (p.isAutomatable ? "Yes" : "No") << "\n";
                if (p.isMetaParameter)
                    std::cout << "  Meta:         Yes\n";
            }
        } else {
            // Compact table view
            std::cout << String::formatted("%-4s %-30s %-12s %-10s %-20s %s\n",
                                          "#", "Name", "Type", "Current", "Range/Values", "Label");
            std::cout << String::repeatedString("-", 100) << "\n";
            
            for (const auto& p : params) {
                String rangeInfo;
                if (p.type == ParameterType::Continuous) {
                    rangeInfo = String::formatted("%.2f - %.2f", p.minValue, p.maxValue);
                } else if (p.type == ParameterType::Boolean) {
                    rangeInfo = "Off / On";
                } else if (p.type == ParameterType::Discrete && !p.valueStrings.empty()) {
                    rangeInfo = String(std::to_string(p.numSteps)) + " steps";
                }
                
                std::cout << String::formatted("%-4d %-30s %-12s %-10.3f %-20s %s\n",
                                              p.index,
                                              p.name.substring(0, 30).toRawUTF8(),
                                              getTypeString(p.type).toRawUTF8(),
                                              p.currentValue,
                                              rangeInfo.substring(0, 20).toRawUTF8(),
                                              p.label.substring(0, 10).toRawUTF8());
            }
        }
        
        std::cout << String::repeatedString("=", 100) << "\n\n";
    }
    
    /**
     * Get string representation of parameter type
     */
    static String getTypeString(ParameterType type) {
        switch (type) {
            case ParameterType::Boolean:    return "Boolean";
            case ParameterType::Discrete:   return "Discrete";
            case ParameterType::Continuous: return "Continuous";
            case ParameterType::Unknown:    return "Unknown";
        }
        return "Unknown";
    }
    
private:
    /**
     * Classify parameter type based on characteristics
     */
    static ParameterType classifyParameterType(AudioProcessorParameter& param) {
        // Check for boolean
        if (dynamic_cast<AudioParameterBool*>(&param))
            return ParameterType::Boolean;
        
        // Check for choice/discrete
        if (dynamic_cast<AudioParameterChoice*>(&param))
            return ParameterType::Discrete;
        
        // Check number of steps
        int steps = param.getNumSteps();
        if (steps == 2)
            return ParameterType::Boolean;
        else if (steps > 2 && steps < 1000)
            return ParameterType::Discrete;
        else if (steps == 0 || steps >= 1000)
            return ParameterType::Continuous;
        
        return ParameterType::Unknown;
    }

    static String getParameterIdentifier(AudioProcessorParameter& param, int fallbackIndex) {
        if (auto* withID = dynamic_cast<AudioProcessorParameterWithID*>(&param)) {
            if (withID->paramID.isNotEmpty())
                return withID->paramID;
        }

        if (auto* hostedParam = dynamic_cast<HostedAudioProcessorParameter*>(&param)) {
            const auto paramID = hostedParam->getParameterID();
            if (paramID.isNotEmpty())
                return paramID;
        }

        return String(std::to_string(fallbackIndex));
    }
};
