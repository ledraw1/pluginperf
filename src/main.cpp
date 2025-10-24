#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

#include "argparse.hpp"
#include "csv.hpp"

using namespace juce;

struct Stats { 
    double mean, median, p95, min, max, stdDev, cv, rtPct, dspLoad; 
    int latency; 
};

template <typename Sample>
static Stats measureOne(AudioPluginInstance& plug,
                        int block, int channels, double sr,
                        int warmup, int iters)
{
    ScopedNoDenormals noDenormals;

    // Recreate processing state per block size to surface reallocations
    plug.releaseResources();
    plug.setNonRealtime(false); // real-time processing mode (matches Plugin Doctor)
    plug.prepareToPlay(sr, block);

    AudioBuffer<Sample> buf(channels, block);
    MidiBuffer midi;

    // Deterministic input so SIMD/branches get exercised
    Random rng(12345);
    for (int c = 0; c < channels; ++c)
        for (int n = 0; n < block; ++n)
            buf.setSample(c, n, (Sample)((rng.nextFloat() * 2.0f - 1.0f) * 0.1f));

    // Warmup iterations
    for (int i = 0; i < warmup; ++i)
        plug.processBlock(buf, midi);

    std::vector<double> us; us.reserve((size_t)iters);
    const double tps = (double) Time::getHighResolutionTicksPerSecond();

    for (int i = 0; i < iters; ++i) {
        const int64 t0 = Time::getHighResolutionTicks();
        plug.processBlock(buf, midi);
        const int64 t1 = Time::getHighResolutionTicks();
        us.push_back((double)(t1 - t0) * 1e6 / tps); // microseconds per call
    }

    std::sort(us.begin(), us.end());

    auto pick = [&](double q) {
        if (us.empty()) return 0.0;
        const double idxF = q * (double)(us.size() - 1);
        size_t idx = (size_t) std::clamp(idxF, 0.0, (double)us.size() - 1.0);
        return us[idx];
    };

    double mean = 0.0; for (double v : us) mean += v; if (!us.empty()) mean /= (double)us.size();
    const double median = pick(0.5);
    const double p95    = pick(0.95);
    const double mn     = us.empty() ? 0.0 : us.front();
    const double mx     = us.empty() ? 0.0 : us.back();

    // Calculate standard deviation
    double variance = 0.0;
    if (!us.empty()) {
        for (double v : us) {
            double diff = v - mean;
            variance += diff * diff;
        }
        variance /= (double)us.size();
    }
    const double stdDev = std::sqrt(variance);
    
    // Coefficient of variation (relative standard deviation)
    const double cv = (mean > 0.0) ? (stdDev / mean) * 100.0 : 0.0;

    const double rtWindow_us = (double)block * 1e6 / sr; // time budget per block
    const double rtPct = rtWindow_us > 0 ? (mean / rtWindow_us) * 100.0 : 0.0;

    // Plugin Doctor style: processing time per sample as % of sample period
    const double samplePeriod_us = 1e6 / sr; // time for one sample
    const double meanPerSample_us = mean / (double)block; // avg time per sample
    const double dspLoad = (meanPerSample_us / samplePeriod_us) * 100.0;

    const int latency = plug.getLatencySamples();

    // Measurement consistency checks (warnings to stderr)
    bool hasWarnings = false;
    
    // Sanity check: min <= median <= mean
    if (mn > median || median > mean) {
        std::cerr << "WARNING [buffer=" << block << "]: Sanity check failed - "
                  << "min=" << mn << " median=" << median << " mean=" << mean << "\n";
        hasWarnings = true;
    }
    
    // Outlier detection: p95/median ratio too high suggests instability
    if (median > 0 && (p95 / median) > 3.0) {
        std::cerr << "WARNING [buffer=" << block << "]: High outlier ratio - "
                  << "p95/median=" << (p95/median) << " (suggests measurement instability)\n";
        hasWarnings = true;
    }
    
    // High CV suggests poor measurement stability
    if (cv > 30.0) {
        std::cerr << "WARNING [buffer=" << block << "]: High coefficient of variation - "
                  << "CV=" << cv << "% (consider more iterations or warmup)\n";
        hasWarnings = true;
    }
    
    // Negative or zero values are invalid
    if (mean <= 0 || median <= 0) {
        std::cerr << "ERROR [buffer=" << block << "]: Invalid measurements - "
                  << "mean=" << mean << " median=" << median << "\n";
        hasWarnings = true;
    }

    plug.releaseResources();

    return Stats{ mean, median, p95, mn, mx, stdDev, cv, rtPct, dspLoad, latency };
}

static void printInstantiationError(const String& err, const String& path)
{
    std::cerr << "CreatePluginInstance failed for \n  "
              << path << "\nReason: " << err << "\n";
}

int main (int argc, char** argv)
{
    Args args; if (!parseArgs(argc, argv, args)) return argc <= 1 ? 0 : 1;

    // Initialize JUCE message manager (required for plugin loading)
    MessageManager::getInstance();
    
    // Initialize plugin formats
    AudioPluginFormatManager fm; fm.addDefaultFormats();

    // Find the VST3 format
    AudioPluginFormat* vst3Format = nullptr;
    for (int i = 0; i < fm.getNumFormats(); ++i) {
        auto* format = fm.getFormat(i);
        if (format->getName() == "VST3") {
            vst3Format = format;
            break;
        }
    }
    
    if (!vst3Format) {
        std::cerr << "VST3 format not available\n";
        return 2;
    }

    // Scan the plugin file to get proper description
    OwnedArray<PluginDescription> foundPlugins;
    vst3Format->findAllTypesForFile(foundPlugins, args.pluginPath);
    
    if (foundPlugins.isEmpty()) {
        std::cerr << "No VST3 plugins found in: " << args.pluginPath << "\n";
        return 2;
    }

    // Use the first plugin found
    PluginDescription desc = *foundPlugins[0];
    
    String err;
    std::unique_ptr<AudioPluginInstance> instance(
        fm.createPluginInstance(desc, args.sampleRate, 512, err)
    );

    if (!instance) {
        printInstantiationError(err, desc.fileOrIdentifier);
        return 2;
    }

    auto* proc = instance.get();

    CsvSink sink;
    if (!sink.open(args.outCsv)) {
        std::cerr << "Failed to open CSV for writing: " << args.outCsv << "\n";
        return 3;
    }

    sink.header();

    const String pluginName = proc->getName();
    const String formatName = "VST3";

    // Single-precision pass (default). Double can be added later.
    proc->setProcessingPrecision(AudioProcessor::singlePrecision);

    for (int block : args.buffers) {
        if (block <= 0) continue;
        const Stats s = measureOne<float>(*proc, block, args.channels, args.sampleRate,
                                          args.warmup, args.iterations);
        sink.row({ pluginName.toStdString(),
                   args.pluginPath,
                   formatName.toStdString(),
                   std::to_string(args.sampleRate),
                   std::to_string(args.channels),
                   std::to_string(args.warmup),
                   std::to_string(args.iterations),
                   std::to_string(block),
                   std::to_string(s.mean),
                   std::to_string(s.median),
                   std::to_string(s.p95),
                   std::to_string(s.min),
                   std::to_string(s.max),
                   std::to_string(s.stdDev),
                   std::to_string(s.cv),
                   std::to_string(s.rtPct),
                   std::to_string(s.dspLoad),
                   std::to_string(s.latency) });
    }

    // Clean up plugin instance before message manager
    instance.reset();
    
    // Clean up message manager before exit
    MessageManager::deleteInstance();
    
    return 0;
}
