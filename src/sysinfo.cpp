#include <juce_core/juce_core.h>
#include <iostream>
#include "system_info.hpp"

using namespace juce;

void printUsage() {
    std::cout << R"(
sysinfo - System Information Tool

Usage:
  sysinfo [options]

Options:
  --json              Output in JSON format
  --csv               Output in CSV format
  --summary           Output brief summary
  -h, --help          Show this help message

Examples:
  # Display full system information
  sysinfo
  
  # JSON output
  sysinfo --json
  
  # CSV format (header + data)
  sysinfo --csv
  
  # Brief summary
  sysinfo --summary

)" << std::endl;
}

int main(int argc, char** argv) {
    bool jsonOutput = false;
    bool csvOutput = false;
    bool summaryOutput = false;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        String arg(argv[i]);
        
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--json") {
            jsonOutput = true;
        } else if (arg == "--csv") {
            csvOutput = true;
        } else if (arg == "--summary") {
            summaryOutput = true;
        }
    }
    
    // Collect system information
    SystemInfo info = SystemInfo::collect();
    
    // Output in requested format
    if (jsonOutput) {
        std::cout << info.toJSON() << std::endl;
    } else if (csvOutput) {
        std::cout << info.toCSVHeader() << std::endl;
        std::cout << info.toCSVRow() << std::endl;
    } else if (summaryOutput) {
        std::cout << info.getSummary() << std::endl;
    } else {
        info.print();
    }
    
    return 0;
}
