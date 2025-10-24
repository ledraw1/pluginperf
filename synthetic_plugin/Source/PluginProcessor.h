#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * Synthetic Test Plugin for PlugPerf validation.
 * 
 * This plugin provides controllable CPU load for testing measurement accuracy.
 * Parameters:
 *   - CPU Load: Number of sin() calculations per sample (0-1000)
 *   - Delay: Artificial delay in microseconds (0-1000)
 */
class SyntheticTestPluginProcessor : public juce::AudioProcessor
{
public:
    SyntheticTestPluginProcessor();
    ~SyntheticTestPluginProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "Synthetic Test Plugin"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

private:
    juce::AudioParameterInt* cpuLoadParam;
    juce::AudioParameterInt* delayParam;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SyntheticTestPluginProcessor)
};
