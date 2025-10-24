#include "PluginProcessor.h"
#include <cmath>
#include <thread>
#include <chrono>

SyntheticTestPluginProcessor::SyntheticTestPluginProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // CPU Load parameter: number of sin() calculations per sample
    addParameter (cpuLoadParam = new juce::AudioParameterInt (
        "cpuload", "CPU Load", 0, 1000, 0));
    
    // Delay parameter: artificial delay in microseconds
    addParameter (delayParam = new juce::AudioParameterInt (
        "delay", "Delay (μs)", 0, 1000, 0));
}

void SyntheticTestPluginProcessor::prepareToPlay (double, int)
{
    // Nothing to prepare
}

void SyntheticTestPluginProcessor::releaseResources()
{
    // Nothing to release
}

void SyntheticTestPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const int cpuLoad = cpuLoadParam->get();
    const int delayUs = delayParam->get();
    
    // Artificial CPU load: perform sin() calculations
    if (cpuLoad > 0)
    {
        volatile float dummy = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int i = 0; i < cpuLoad; ++i)
            {
                dummy += std::sin((float)i * 0.001f);
            }
        }
    }
    
    // Artificial delay
    if (delayUs > 0)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(delayUs));
    }
    
    // Pass through audio (copy input to output)
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        // Audio is already in the buffer, just ensure it's passed through
        // (no processing needed for passthrough)
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SyntheticTestPluginProcessor();
}
