#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "VoxDsp.h"

class VoxChainProcessor : public juce::AudioProcessor
{
public:
    VoxChainProcessor();
    ~VoxChainProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VoxChain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static const juce::StringArray& presetNames();
    void applyPreset (int index); // a appeler depuis le thread de l'interface

    juce::AudioProcessorValueTreeState apvts;
    vx::Engine engine;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float val (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxChainProcessor)
};
