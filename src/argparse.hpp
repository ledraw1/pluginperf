

#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdio>

struct Args {
    std::string pluginPath;
    double sampleRate = 48000.0;
    int channels = 2;
    std::string bitDepth = "32f"; // 32f, 64f
    std::vector<int> buffers {32,64,128,256,512,1024,2048,4096,8192,16384};
    int warmup = 40;
    int iterations = 400;
    std::string outCsv; // empty => stdout
    std::string presetJson; // StoryBored JSON preset path
    bool nonRealtime = false; // Use non-realtime processing mode
};

static inline std::vector<int> parseIntList(const std::string& s) {
    std::vector<int> v; v.reserve(16);
    std::stringstream ss(s); std::string item;
    while (std::getline(ss, item, ',')) {
        try { v.push_back(std::stoi(item)); } catch (...) {}
    }
    if (!v.empty()) std::sort(v.begin(), v.end());
    return v;
}

static inline void printHelp(const char* argv0) {
    std::fprintf(stderr,
R"HELP(
Usage: %s --plugin /path/Your.vst3 [options]

Required:
  --plugin PATH            Path to .vst3 bundle to measure

Options:
  --sr HZ                  Sample rate, e.g. 44100|48000|96000 (default 48000)
  --channels N             Channel count (default 2)
  --bits DEPTH             Bit depth: 32f|64f (default 32f)
                           32f=32-bit float, 64f=64-bit double
  --buffers CSV            Buffer sizes list (default 32..16384)
                           e.g. 32,64,128,256,512,1024,2048,4096,8192,16384
  --warmup N               Warmup iterations per size (default 40)
  --iterations N           Timed iterations per size (default 400)
  --out PATH               Write CSV to PATH (default stdout)
  --preset-json PATH       Load StoryBored JSON preset before benchmarking
  --non-realtime           Use non-realtime processing mode (default: realtime)
  -h, --help               Show this help and exit
)HELP", argv0);
}

static inline bool parseArgs(int argc, char** argv, Args& a) {
    if (argc <= 1) { printHelp(argv[0]); return false; }

    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto need = [&](const char* name){ if (i+1 >= argc) { std::fprintf(stderr, "Missing value for %s\n", name); return false; } return true; };

        if (k == "-h" || k == "--help") { printHelp(argv[0]); return false; }
        else if (k == "--plugin") { if (!need("--plugin")) return false; a.pluginPath = argv[++i]; }
        else if (k == "--sr") { if (!need("--sr")) return false; a.sampleRate = std::stod(argv[++i]); }
        else if (k == "--channels") { if (!need("--channels")) return false; a.channels = std::stoi(argv[++i]); }
        else if (k == "--bits") { if (!need("--bits")) return false; a.bitDepth = argv[++i]; }
        else if (k == "--buffers") { if (!need("--buffers")) return false; a.buffers = parseIntList(argv[++i]); }
        else if (k == "--warmup") { if (!need("--warmup")) return false; a.warmup = std::stoi(argv[++i]); }
        else if (k == "--iterations") { if (!need("--iterations")) return false; a.iterations = std::stoi(argv[++i]); }
        else if (k == "--out") { if (!need("--out")) return false; a.outCsv = argv[++i]; }
        else if (k == "--preset-json") { if (!need("--preset-json")) return false; a.presetJson = argv[++i]; }
        else if (k == "--non-realtime") { a.nonRealtime = true; }
        else { std::fprintf(stderr, "Unknown option: %s\n", k.c_str()); return false; }
    }

    if (a.pluginPath.empty()) { std::fprintf(stderr, "--plugin is required\n"); return false; }
    if (a.channels <= 0) { std::fprintf(stderr, "--channels must be > 0\n"); return false; }
    if (a.sampleRate <= 0) { std::fprintf(stderr, "--sr must be > 0\n"); return false; }
    if (a.buffers.empty()) { std::fprintf(stderr, "--buffers resulted in empty list\n"); return false; }
    if (a.bitDepth != "32f" && a.bitDepth != "64f") {
        std::fprintf(stderr, "--bits must be one of: 32f, 64f\n"); return false;
    }

    return true;
}