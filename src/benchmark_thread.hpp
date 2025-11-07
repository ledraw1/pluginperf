#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace juce;

struct Stats {
    double mean, median, p95, min, max, stdDev, cv, rtPct, dspLoad;
    int latency;
};

struct BenchmarkConfig {
    AudioPluginInstance* plugin = nullptr;
    int blockSize = 0;
    int channels = 0;
    double sampleRate = 0.0;
    int warmupIterations = 0;
    int timedIterations = 0;
    bool useDoublePrecision = false;
};

struct BenchmarkResult {
    Stats stats;
    bool success = false;
    String errorMessage;
};

/**
 * Dedicated real-time thread for running plugin benchmarks.
 * This matches DAW behavior by running measurements on a high-priority
 * audio thread with time-constraint scheduling (macOS/iOS) or equivalent.
 */
class BenchmarkThread : public Thread
{
public:
    BenchmarkThread() : Thread("PlugPerf RT Benchmark") {}
    
    ~BenchmarkThread()
    {
        // Ensure thread is stopped on destruction
        stopThread(2000);
    }
    
    /**
     * Run a benchmark with the given configuration using a dedicated thread.
     */
    BenchmarkResult runBenchmark(const BenchmarkConfig& config)
    {
        config_ = config;
        result_ = BenchmarkResult();

        if (config_.plugin == nullptr)
        {
            result_.success = false;
            result_.errorMessage = "Invalid benchmark configuration: plugin is null";
            return result_;
        }

        auto rtOptions = createRealtimeOptions(config_);
        bool started = startRealtimeThread(rtOptions);

        if (! started)
        {
            std::cerr << "WARNING: Unable to start realtime benchmark thread; falling back to high priority.\n";
            started = startThread(Thread::Priority::high);
        }

        if (! started)
        {
            result_.success = false;
            result_.errorMessage = "Failed to start benchmark thread";
            return result_;
        }

        waitForThreadToExit(-1);
        return result_;
    }
    
private:
    void run() override
    {
        const BenchmarkConfig configCopy = config_;

        try
        {
            if (configCopy.useDoublePrecision)
                result_.stats = measureOneImpl<double>(configCopy);
            else
                result_.stats = measureOneImpl<float>(configCopy);

            result_.success = true;
        }
        catch (const std::exception& e)
        {
            result_.success = false;
            result_.errorMessage = String("Exception: ") + e.what();
            std::cerr << "ERROR: Exception: " << e.what() << "\n";
        }
        catch (...)
        {
            result_.success = false;
            result_.errorMessage = "Unknown exception";
            std::cerr << "ERROR: Unknown exception\n";
        }
    }
    
    template <typename Sample>
    Stats measureOneImpl(const BenchmarkConfig& cfg)
    {
        ScopedNoDenormals noDenormals;
        
        auto& plug = *cfg.plugin;
        const int block = cfg.blockSize;
        const int channels = cfg.channels;
        const double sr = cfg.sampleRate;
        const int warmup = cfg.warmupIterations;
        const int iters = cfg.timedIterations;
        
        // Recreate processing state per block size to surface reallocations
        plug.releaseResources();
        plug.setNonRealtime(false); // real-time processing mode
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
        {
            midi.clear();
            plug.processBlock(buf, midi);
        }
        
        std::vector<double> us;
        us.reserve((size_t)iters);
        const double tps = (double) Time::getHighResolutionTicksPerSecond();
        
        // Timed iterations
        for (int i = 0; i < iters; ++i)
        {
            midi.clear();
            const int64 t0 = Time::getHighResolutionTicks();
            plug.processBlock(buf, midi);
            const int64 t1 = Time::getHighResolutionTicks();
            us.push_back((double)(t1 - t0) * 1e6 / tps);
        }
        
        std::sort(us.begin(), us.end());
        
        auto pick = [&](double q) {
            if (us.empty()) return 0.0;
            const double idxF = q * (double)(us.size() - 1);
            size_t idx = (size_t) std::clamp(idxF, 0.0, (double)us.size() - 1.0);
            return us[idx];
        };
        
        double mean = 0.0;
        for (double v : us) mean += v;
        if (!us.empty()) mean /= (double)us.size();
        
        const double median = pick(0.5);
        const double p95 = pick(0.95);
        const double mn = us.empty() ? 0.0 : us.front();
        const double mx = us.empty() ? 0.0 : us.back();
        
        // Calculate standard deviation
        double variance = 0.0;
        if (!us.empty())
        {
            for (double v : us)
            {
                double diff = v - mean;
                variance += diff * diff;
            }
            variance /= (double)us.size();
        }
        const double stdDev = std::sqrt(variance);
        
        // Coefficient of variation (relative standard deviation)
        const double cv = (mean > 0.0) ? (stdDev / mean) * 100.0 : 0.0;
        
        const double rtWindow_us = (double)block * 1e6 / sr;
        const double rtPct = rtWindow_us > 0 ? (mean / rtWindow_us) * 100.0 : 0.0;
        
        // Plugin Doctor style: processing time per sample as % of sample period
        const double samplePeriod_us = 1e6 / sr;
        const double meanPerSample_us = mean / (double)block;
        const double dspLoad = (meanPerSample_us / samplePeriod_us) * 100.0;
        
        const int latency = plug.getLatencySamples();
        
        // Measurement consistency checks (warnings to stderr)
        if (mn > median || median > mean)
        {
            std::cerr << "WARNING [buffer=" << block << "]: Sanity check failed - "
                      << "min=" << mn << " median=" << median << " mean=" << mean << "\n";
        }
        
        if (median > 0 && (p95 / median) > 3.0)
        {
            std::cerr << "WARNING [buffer=" << block << "]: High outlier ratio - "
                      << "p95/median=" << (p95/median) << " (suggests measurement instability)\n";
        }
        
        if (cv > 30.0)
        {
            std::cerr << "WARNING [buffer=" << block << "]: High coefficient of variation - "
                      << "CV=" << cv << "% (consider more iterations or warmup)\n";
        }
        
        if (mean <= 0 || median <= 0)
        {
            std::cerr << "ERROR [buffer=" << block << "]: Invalid measurements - "
                      << "mean=" << mean << " median=" << median << "\n";
        }
        
        plug.releaseResources();
        
        return Stats{mean, median, p95, mn, mx, stdDev, cv, rtPct, dspLoad, latency};
    }
    
    static Thread::RealtimeOptions createRealtimeOptions(const BenchmarkConfig& cfg)
    {
        Thread::RealtimeOptions opts;

        opts = opts.withPriority(8);

        if (cfg.blockSize > 0 && cfg.sampleRate > 0.0)
        {
            opts = opts.withApproximateAudioProcessingTime(cfg.blockSize, cfg.sampleRate)
                       .withPeriodHz(cfg.sampleRate / (double) cfg.blockSize);
        }

        return opts;
    }

    BenchmarkConfig config_;
    BenchmarkResult result_;
};
