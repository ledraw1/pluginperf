#pragma once
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

// Minimal CSV writer for plugperf
// Usage:
//   CsvSink sink; sink.open(path); sink.header(); sink.row({"a","b",...});
// If path is empty, writes to stdout.
struct CsvSink {
    std::ostream* out = nullptr;
    std::unique_ptr<std::ofstream> owned;

    bool open(const std::string& path) {
        if (path.empty()) {
            out = &std::cout;
            return true;
        }
        owned = std::make_unique<std::ofstream>(path, std::ios::out | std::ios::trunc);
        if (!owned || !(*owned)) return false;
        out = owned.get();
        return true;
    }

    // Column order must match what main.cpp writes
    void header() {
        (*out)
            << "plugin_name,plugin_path,format,sr,channels,warmup,iterations,block_size,"
            << "mean_us,median_us,p95_us,min_us,max_us,std_dev_us,cv_pct,"
            << "approx_rt_cpu_pct,dsp_load_pct,latency_samples\n";
    }

    void row(const std::vector<std::string>& cols) {
        for (size_t i = 0; i < cols.size(); ++i) {
            // naive CSV escaping for commas and quotes
            const std::string& c = cols[i];
            bool needsQuotes = (c.find(',') != std::string::npos) || (c.find('"') != std::string::npos) || (c.find('\n') != std::string::npos);
            if (needsQuotes) {
                (*out) << '"';
                for (char ch : c) {
                    if (ch == '"') (*out) << '"'; // escape quotes by doubling
                    (*out) << ch;
                }
                (*out) << '"';
            } else {
                (*out) << c;
            }
            if (i + 1 < cols.size()) (*out) << ',';
        }
        (*out) << '\n';
    }
};
