#pragma once
#include <juce_core/juce_core.h>
#include <iostream>
#include <string>

using namespace juce;

/**
 * System information and metadata collection
 */
struct SystemInfo {
    String cpuModel;
    String cpuVendor;
    int numPhysicalCores;
    int numLogicalCores;
    int64 totalRAM;           // in bytes
    String osName;
    String osVersion;
    String computerName;
    String userName;
    int cpuSpeedMHz;
    bool hasSSE2;
    bool hasSSE3;
    bool hasSSE41;
    bool hasAVX;
    bool hasAVX2;
    bool hasAVX512F;
    bool hasNeon;
    
private:
    /**
     * Get accurate CPU model on macOS using system_profiler
     * Includes model identifier for disambiguation
     */
    static String getMacOSCPUModel() {
        #if JUCE_MAC
            ChildProcess process;
            if (process.start("system_profiler SPHardwareDataType")) {
                String output = process.readAllProcessOutput();
                
                String chipName;
                String modelId;
                
                // Look for "Chip:" line (Apple Silicon)
                int chipPos = output.indexOf("Chip:");
                if (chipPos >= 0) {
                    String line = output.substring(chipPos);
                    int endPos = line.indexOfChar('\n');
                    if (endPos > 0) {
                        chipName = line.substring(5, endPos).trim();
                    }
                }
                
                // Look for "Model Identifier:" for more specificity
                int modelPos = output.indexOf("Model Identifier:");
                if (modelPos >= 0) {
                    String line = output.substring(modelPos);
                    int endPos = line.indexOfChar('\n');
                    if (endPos > 0) {
                        modelId = line.substring(17, endPos).trim();
                    }
                }
                
                // Fallback to "Processor Name:" (Intel)
                if (chipName.isEmpty()) {
                    int procPos = output.indexOf("Processor Name:");
                    if (procPos >= 0) {
                        String line = output.substring(procPos);
                        int endPos = line.indexOfChar('\n');
                        if (endPos > 0) {
                            chipName = line.substring(15, endPos).trim();
                        }
                    }
                }
                
                // Return chip name with model ID if available
                if (chipName.isNotEmpty()) {
                    // Simplify Apple Silicon naming
                    if (chipName.startsWith("Apple M")) {
                        if (modelId.isNotEmpty()) {
                            return "Apple Silicon (" + modelId + ")";
                        }
                        return "Apple Silicon";
                    }
                    
                    // Intel or other processors - keep original name
                    if (modelId.isNotEmpty()) {
                        return chipName + " (" + modelId + ")";
                    }
                    return chipName;
                }
            }
        #endif
        
        // Fallback to JUCE
        return SystemStats::getCpuModel();
    }
    
public:
    /**
     * Collect all system information
     */
    static SystemInfo collect() {
        SystemInfo info;
        
        // CPU information - use native tools on macOS for accuracy
        #if JUCE_MAC
            info.cpuModel = getMacOSCPUModel();
            info.cpuVendor = info.cpuModel.contains("Apple") ? "Apple" : "Intel";
        #else
            info.cpuModel = SystemStats::getCpuModel();
            info.cpuVendor = SystemStats::getCpuVendor();
        #endif
        
        info.numPhysicalCores = SystemStats::getNumPhysicalCpus();
        info.numLogicalCores = SystemStats::getNumCpus();
        info.cpuSpeedMHz = SystemStats::getCpuSpeedInMegahertz();
        
        // CPU features
        info.hasSSE2 = SystemStats::hasSSE2();
        info.hasSSE3 = SystemStats::hasSSE3();
        info.hasSSE41 = SystemStats::hasSSE41();
        info.hasAVX = SystemStats::hasAVX();
        info.hasAVX2 = SystemStats::hasAVX2();
        info.hasAVX512F = SystemStats::hasAVX512F();
        info.hasNeon = SystemStats::hasNeon();
        
        // Memory
        info.totalRAM = SystemStats::getMemorySizeInMegabytes() * 1024LL * 1024LL;
        
        // OS information
        info.osName = SystemStats::getOperatingSystemName();
        // Parse version from OS name (e.g., "macOS 14.1.2")
        info.osVersion = info.osName.fromLastOccurrenceOf(" ", false, false);
        if (info.osVersion.isEmpty()) {
            info.osVersion = "Unknown";
        }
        info.computerName = SystemStats::getComputerName();
        info.userName = SystemStats::getLogonName();
        
        return info;
    }
    
    /**
     * Print system information to console
     */
    void print() const {
        std::cout << "\n";
        std::cout << String::repeatedString("=", 80) << "\n";
        std::cout << "SYSTEM INFORMATION\n";
        std::cout << String::repeatedString("=", 80) << "\n\n";
        
        std::cout << "Operating System:\n";
        std::cout << "  Name:           " << osName << "\n";
        std::cout << "  Version:        " << osVersion << "\n";
        std::cout << "  Computer:       " << computerName << "\n";
        std::cout << "  User:           " << userName << "\n\n";
        
        std::cout << "CPU:\n";
        std::cout << "  Model:          " << cpuModel << "\n";
        std::cout << "  Vendor:         " << cpuVendor << "\n";
        std::cout << "  Speed:          " << cpuSpeedMHz << " MHz\n";
        std::cout << "  Physical Cores: " << numPhysicalCores << "\n";
        std::cout << "  Logical Cores:  " << numLogicalCores << "\n\n";
        
        std::cout << "CPU Features:\n";
        std::cout << "  SSE2:           " << (hasSSE2 ? "Yes" : "No") << "\n";
        std::cout << "  SSE3:           " << (hasSSE3 ? "Yes" : "No") << "\n";
        std::cout << "  SSE4.1:         " << (hasSSE41 ? "Yes" : "No") << "\n";
        std::cout << "  AVX:            " << (hasAVX ? "Yes" : "No") << "\n";
        std::cout << "  AVX2:           " << (hasAVX2 ? "Yes" : "No") << "\n";
        std::cout << "  AVX-512F:       " << (hasAVX512F ? "Yes" : "No") << "\n";
        std::cout << "  NEON:           " << (hasNeon ? "Yes" : "No") << "\n\n";
        
        std::cout << "Memory:\n";
        std::cout << "  Total RAM:      " << formatBytes(totalRAM) << "\n";
        
        std::cout << String::repeatedString("=", 80) << "\n\n";
    }
    
    /**
     * Export as JSON string
     */
    String toJSON() const {
        String json = "{\n";
        json += "  \"system\": {\n";
        json += "    \"os\": {\n";
        json += "      \"name\": \"" + escapeJSON(osName) + "\",\n";
        json += "      \"version\": \"" + escapeJSON(osVersion) + "\",\n";
        json += "      \"computer\": \"" + escapeJSON(computerName) + "\",\n";
        json += "      \"user\": \"" + escapeJSON(userName) + "\"\n";
        json += "    },\n";
        json += "    \"cpu\": {\n";
        json += "      \"model\": \"" + escapeJSON(cpuModel) + "\",\n";
        json += "      \"vendor\": \"" + escapeJSON(cpuVendor) + "\",\n";
        json += "      \"speedMHz\": " + String(cpuSpeedMHz) + ",\n";
        json += "      \"physicalCores\": " + String(numPhysicalCores) + ",\n";
        json += "      \"logicalCores\": " + String(numLogicalCores) + ",\n";
        json += "      \"features\": {\n";
        json += "        \"sse2\": " + String(hasSSE2 ? "true" : "false") + ",\n";
        json += "        \"sse3\": " + String(hasSSE3 ? "true" : "false") + ",\n";
        json += "        \"sse41\": " + String(hasSSE41 ? "true" : "false") + ",\n";
        json += "        \"avx\": " + String(hasAVX ? "true" : "false") + ",\n";
        json += "        \"avx2\": " + String(hasAVX2 ? "true" : "false") + ",\n";
        json += "        \"avx512f\": " + String(hasAVX512F ? "true" : "false") + ",\n";
        json += "        \"neon\": " + String(hasNeon ? "true" : "false") + "\n";
        json += "      }\n";
        json += "    },\n";
        json += "    \"memory\": {\n";
        json += "      \"totalBytes\": " + String(totalRAM) + ",\n";
        json += "      \"totalMB\": " + String(totalRAM / (1024 * 1024)) + ",\n";
        json += "      \"totalGB\": " + String(totalRAM / (1024.0 * 1024.0 * 1024.0), 2) + "\n";
        json += "    }\n";
        json += "  }\n";
        json += "}";
        return json;
    }
    
    /**
     * Export as CSV header and row
     */
    String toCSVHeader() const {
        return "os_name,os_version,computer_name,cpu_model,cpu_vendor,cpu_speed_mhz,"
               "physical_cores,logical_cores,total_ram_bytes,total_ram_gb,"
               "has_sse2,has_sse3,has_sse41,has_avx,has_avx2,has_avx512f,has_neon";
    }
    
    String toCSVRow() const {
        return escapeCSV(osName) + "," +
               escapeCSV(osVersion) + "," +
               escapeCSV(computerName) + "," +
               escapeCSV(cpuModel) + "," +
               escapeCSV(cpuVendor) + "," +
               String(cpuSpeedMHz) + "," +
               String(numPhysicalCores) + "," +
               String(numLogicalCores) + "," +
               String(totalRAM) + "," +
               String(totalRAM / (1024.0 * 1024.0 * 1024.0), 2) + "," +
               String(hasSSE2 ? 1 : 0) + "," +
               String(hasSSE3 ? 1 : 0) + "," +
               String(hasSSE41 ? 1 : 0) + "," +
               String(hasAVX ? 1 : 0) + "," +
               String(hasAVX2 ? 1 : 0) + "," +
               String(hasAVX512F ? 1 : 0) + "," +
               String(hasNeon ? 1 : 0);
    }
    
    /**
     * Get a short summary string
     */
    String getSummary() const {
        String summary;
        summary << cpuModel << " (" << numPhysicalCores << "C/" << numLogicalCores << "T) @ " 
                << cpuSpeedMHz << "MHz, " << formatBytes(totalRAM) << " RAM, " << osName;
        return summary;
    }
    
private:
    static String formatBytes(int64 bytes) {
        if (bytes >= 1024LL * 1024LL * 1024LL * 1024LL) {
            return String(bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0), 2) + " TB";
        } else if (bytes >= 1024LL * 1024LL * 1024LL) {
            return String(bytes / (1024.0 * 1024.0 * 1024.0), 2) + " GB";
        } else if (bytes >= 1024LL * 1024LL) {
            return String(bytes / (1024.0 * 1024.0), 2) + " MB";
        } else if (bytes >= 1024LL) {
            return String(bytes / 1024.0, 2) + " KB";
        } else {
            return String(bytes) + " bytes";
        }
    }
    
    static String escapeJSON(const String& str) {
        return str.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n");
    }
    
    static String escapeCSV(const String& str) {
        if (str.containsAnyOf(",\"\n\r")) {
            return "\"" + str.replace("\"", "\"\"") + "\"";
        }
        return str;
    }
};
