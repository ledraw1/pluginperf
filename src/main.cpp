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
#include "benchmark_thread.hpp"
#include "system_info.hpp"

using namespace juce;

static bool configureChannelLayout(AudioPluginInstance& proc,
                                   int requestedChannels,
                                   int& configuredChannels)
{
    if (requestedChannels <= 0)
        return false;

    if (requestedChannels != 1 && requestedChannels != 2)
        return false;

    const int inputBusCount  = proc.getBusCount(true);
    const int outputBusCount = proc.getBusCount(false);
    const auto desiredSet = (requestedChannels == 1)
                                ? AudioChannelSet::mono()
                                : AudioChannelSet::stereo();

    bool layoutChanged = false;
    auto layout = proc.getBusesLayout();

    if (outputBusCount > 0)
    {
        if (layout.outputBuses.getReference(0) != desiredSet)
        {
            layout.outputBuses.getReference(0) = desiredSet;
            layoutChanged = true;
        }
    }

    if (inputBusCount > 0)
    {
        if (layout.inputBuses.getReference(0) != desiredSet)
        {
            layout.inputBuses.getReference(0) = desiredSet;
            layoutChanged = true;
        }
    }

    if (layoutChanged && ! proc.setBusesLayout(layout))
        return false;

    const int actualInputs  = inputBusCount  > 0 ? proc.getChannelCountOfBus(true, 0)  : 0;
    const int actualOutputs = outputBusCount > 0 ? proc.getChannelCountOfBus(false, 0) : 0;

    if (outputBusCount > 0)
    {
        if (actualOutputs != requestedChannels)
            return false;

        configuredChannels = actualOutputs;
    }
    else
    {
        configuredChannels = requestedChannels;
    }

    if (inputBusCount > 0 && actualInputs != 0 && actualInputs != requestedChannels)
        return false;

    return true;
}

// Measurement logic moved to benchmark_thread.hpp for real-time thread execution

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

    int measurementChannels = args.channels;
    if (! configureChannelLayout(*proc, args.channels, measurementChannels))
    {
        std::cerr << "Unable to configure plugin for "
                  << args.channels << " channels.\n";
        return 2;
    }

    CsvSink sink;
    if (!sink.open(args.outCsv)) {
        std::cerr << "Failed to open CSV for writing: " << args.outCsv << "\n";
        return 3;
    }

    sink.header();

    // Collect system information once
    SystemInfo sysInfo = SystemInfo::collect();
    
    const String pluginName = proc->getName();
    const String formatName = "VST3";

    // Set processing precision based on bit depth
    const bool wantsDouble = args.bitDepth == "64f";
    const bool canDouble = proc->supportsDoublePrecisionProcessing();
    const bool useDouble = wantsDouble && canDouble;
    const std::string bitDepthLabel = useDouble ? "64f" : "32f";

    if (wantsDouble && !canDouble) {
        std::cerr << "WARNING: Plugin does not support double precision processing; "
                  << "falling back to single precision measurements.\n";
    }

    if (useDouble) {
        proc->setProcessingPrecision(AudioProcessor::doublePrecision);
    } else {
        proc->setProcessingPrecision(AudioProcessor::singlePrecision);
    }

    // Run measurements on dedicated real-time thread
    for (int block : args.buffers)
    {
        if (block <= 0) continue;
        
        // Create a new thread instance for each buffer size
        // (JUCE threads can only be started once)
        BenchmarkThread benchThread;
        
        // Configure benchmark
        BenchmarkConfig config;
        config.plugin = proc;
        config.blockSize = block;
        config.channels = measurementChannels;
        config.sampleRate = args.sampleRate;
        config.warmupIterations = args.warmup;
        config.timedIterations = args.iterations;
        config.useDoublePrecision = useDouble;
        
        // Run on real-time thread
        BenchmarkResult result = benchThread.runBenchmark(config);
        
        if (!result.success)
        {
            std::cerr << "Benchmark failed for buffer size " << block 
                      << ": " << result.errorMessage << "\n";
            continue;
        }
        
        const Stats& s = result.stats;
        sink.row({ pluginName.toStdString(), args.pluginPath, formatName.toStdString(),
                   std::to_string(args.sampleRate), std::to_string(measurementChannels), bitDepthLabel,
                   std::to_string(args.warmup), std::to_string(args.iterations), std::to_string(block),
                   std::to_string(s.mean), std::to_string(s.median), std::to_string(s.p95),
                   std::to_string(s.min), std::to_string(s.max), std::to_string(s.stdDev),
                   std::to_string(s.cv), std::to_string(s.rtPct), std::to_string(s.dspLoad),
                   std::to_string(s.latency),
                   sysInfo.cpuModel.toStdString(),
                   std::to_string(sysInfo.numPhysicalCores),
                   std::to_string(sysInfo.cpuSpeedMHz),
                   std::to_string(sysInfo.totalRAM / (1024.0 * 1024.0 * 1024.0)),
                   sysInfo.osName.toStdString() });
    }

    // Clean up plugin instance before message manager
    instance.reset();
    
    // Clean up message manager before exit
    MessageManager::deleteInstance();
    
    return 0;
}
