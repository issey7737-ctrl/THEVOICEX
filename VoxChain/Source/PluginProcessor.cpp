#include "PluginProcessor.h"
#include "PluginEditor.h"

using F = juce::AudioParameterFloat;
using R = juce::NormalisableRange<float>;

static void addF (juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name,
                  float lo, float hi, float def, float step = 0.1f, float skew = 1.0f)
{
    l.add (std::make_unique<F> (juce::ParameterID { id, 1 }, name, R (lo, hi, step, skew), def));
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxChainProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    addF (l, "inGain", "Input Gain", -12, 12, 0);
    addF (l, "lowCut", "Low Cut", 20, 300, 80, 1.0f, 0.5f);
    addF (l, "eqLow", "EQ Low", -9, 9, 0);
    addF (l, "eqBody", "EQ Body", -9, 9, 0);
    addF (l, "eqPres", "EQ Presence", -9, 9, 0);
    addF (l, "eqAir", "EQ Air", -9, 9, 0);
    addF (l, "deess", "De-Esser", 0, 100, 30, 1.0f);
    addF (l, "compThr", "Comp Threshold", -40, 0, -18);
    addF (l, "compRatio", "Comp Ratio", 1, 10, 3);
    addF (l, "compAtk", "Comp Attack", 1, 100, 15, 0.1f, 0.5f);
    addF (l, "compRel", "Comp Release", 20, 500, 150, 1.0f, 0.5f);
    addF (l, "compMakeup", "Comp Makeup", 0, 18, 3);
    addF (l, "compMix", "Comp Mix", 0, 100, 100, 1.0f);
    addF (l, "sat", "Saturation", 0, 100, 10, 1.0f);
    addF (l, "dbl", "Doubler", 0, 100, 0, 1.0f);
    addF (l, "dlyTime", "Delay Time", 20, 1000, 250, 1.0f, 0.5f);
    addF (l, "dlyFb", "Delay Feedback", 0, 90, 25, 1.0f);
    addF (l, "dlyTone", "Delay Tone", 800, 12000, 5000, 10.0f, 0.4f);
    addF (l, "dlyMix", "Delay Mix", 0, 100, 0, 1.0f);
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "dlyPing", 1 }, "Delay Ping-Pong", true));
    addF (l, "revSize", "Reverb Size", 0, 100, 40, 1.0f);
    addF (l, "revDamp", "Reverb Damping", 0, 100, 50, 1.0f);
    addF (l, "revMix", "Reverb Mix", 0, 100, 10, 1.0f);
    addF (l, "outGain", "Output Gain", -12, 12, 0);
    return l;
}

VoxChainProcessor::VoxChainProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool VoxChainProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void VoxChainProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (0);
}

void VoxChainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) return;

    vx::Params p;
    p.inGain = val ("inGain"); p.lowCut = val ("lowCut");
    p.eqLow = val ("eqLow"); p.eqBody = val ("eqBody"); p.eqPres = val ("eqPres"); p.eqAir = val ("eqAir");
    p.deess = val ("deess") / 100.f;
    p.compThr = val ("compThr"); p.compRatio = val ("compRatio"); p.compAtk = val ("compAtk"); p.compRel = val ("compRel");
    p.compMakeup = val ("compMakeup"); p.compMix = val ("compMix") / 100.f;
    p.sat = val ("sat") / 100.f; p.dbl = val ("dbl") / 100.f;
    p.dlyTime = val ("dlyTime"); p.dlyFb = val ("dlyFb") / 100.f; p.dlyTone = val ("dlyTone"); p.dlyMix = val ("dlyMix") / 100.f;
    p.ping = val ("dlyPing") > 0.5f;
    p.revSize = val ("revSize") / 100.f; p.revDamp = val ("revDamp") / 100.f; p.revMix = val ("revMix") / 100.f;
    p.outGain = val ("outGain");
    engine.setParams (p);
    engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
}

//==============================================================================
const juce::StringArray& VoxChainProcessor::presetNames()
{
    static const juce::StringArray n { "Init (neutre)", "Rap Lead", "Pop Clean", "Podcast / Voix parlee", "Dark Trap", "Airy Ballad" };
    return n;
}

void VoxChainProcessor::applyPreset (int index)
{
    struct V { const char* id; float v; };
    static const std::vector<std::vector<V>> presets = {
        {},
        { { "lowCut", 90 }, { "eqLow", -1 }, { "eqBody", -2 }, { "eqPres", 3 }, { "eqAir", 4 }, { "deess", 55 },
          { "compThr", -24 }, { "compRatio", 4.5f }, { "compAtk", 8 }, { "compRel", 90 }, { "compMakeup", 6 }, { "sat", 22 },
          { "dlyMix", 12 }, { "dlyTime", 190 }, { "dlyFb", 22 }, { "revMix", 8 }, { "revSize", 25 }, { "dbl", 25 } },
        { { "lowCut", 100 }, { "eqBody", -1.5f }, { "eqPres", 2 }, { "eqAir", 3.5f }, { "deess", 45 },
          { "compThr", -20 }, { "compRatio", 3 }, { "compAtk", 15 }, { "compRel", 140 }, { "compMakeup", 4 }, { "sat", 8 },
          { "dlyMix", 8 }, { "revMix", 14 }, { "revSize", 45 }, { "dbl", 12 } },
        { { "lowCut", 110 }, { "eqLow", 1 }, { "eqBody", -1 }, { "eqPres", 2.5f }, { "eqAir", 1 }, { "deess", 40 },
          { "compThr", -22 }, { "compRatio", 3.5f }, { "compAtk", 10 }, { "compRel", 120 }, { "compMakeup", 6 }, { "sat", 5 },
          { "dlyMix", 0 }, { "revMix", 0 }, { "dbl", 0 } },
        { { "lowCut", 70 }, { "eqLow", 2 }, { "eqPres", 1 }, { "deess", 50 },
          { "compThr", -26 }, { "compRatio", 5 }, { "compAtk", 5 }, { "compRel", 80 }, { "compMakeup", 7 }, { "sat", 35 },
          { "dlyMix", 18 }, { "dlyTime", 375 }, { "dlyFb", 35 }, { "dlyTone", 3000 }, { "revMix", 12 }, { "revSize", 60 }, { "dbl", 18 } },
        { { "lowCut", 85 }, { "eqLow", 1 }, { "eqBody", -1 }, { "eqPres", 1.5f }, { "eqAir", 5 }, { "deess", 50 },
          { "compThr", -18 }, { "compRatio", 2.5f }, { "compAtk", 25 }, { "compRel", 220 }, { "compMakeup", 3 }, { "sat", 12 },
          { "dlyMix", 15 }, { "dlyTime", 420 }, { "dlyFb", 30 }, { "dlyTone", 4500 }, { "revMix", 28 }, { "revSize", 75 }, { "revDamp", 40 }, { "dbl", 20 } }
    };
    if (index < 0 || index >= (int) presets.size()) return;

    // retour aux valeurs par defaut (sauf les gains d'entree / sortie), puis application du preset
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            const auto id = rp->paramID;
            if (id == "inGain" || id == "outGain") continue;
            rp->setValueNotifyingHost (rp->getDefaultValue());
        }
    for (auto& v : presets[(size_t) index])
        if (auto* rp = apvts.getParameter (v.id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v.v));
}

juce::AudioProcessorEditor* VoxChainProcessor::createEditor() { return new VoxChainEditor (*this); }

void VoxChainProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VoxChainProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoxChainProcessor(); }
