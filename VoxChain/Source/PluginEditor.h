#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Style : "verre sombre" 2026 - fond anime qui reagit au niveau, boutons lumineux, jauges fluides.
class VoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoxLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool isDown, int, int, int, int, juce::ComboBox&) override;
};

class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name,
          juce::Colour accent, const juce::String& suffix, int decimals);
    void resized() override;

private:
    juce::Label label;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class Meter : public juce::Component
{
public:
    Meter (const juce::String& n, bool isReduction) : name (n), reduction (isReduction) {}
    void setDb (float db);
    void paint (juce::Graphics&) override;

private:
    juce::String name;
    bool reduction;
    float shown = 0.f; // 0..1
};

class VoxChainEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VoxChainEditor (VoxChainProcessor&);
    ~VoxChainEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    Knob* addKnob (std::vector<std::unique_ptr<Knob>>& group, const char* id, const char* name, juce::Colour accent,
                   const char* suffix, int decimals);
    void placeGroup (juce::Rectangle<int> card, std::vector<juce::Component*> comps, int slots = 0);

    VoxLookAndFeel laf;
    VoxChainProcessor& proc;

    std::vector<std::unique_ptr<Knob>> tone, dyn, color, delay, reverb, levels;
    juce::TextButton pingBtn { "PING-PONG" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pingAtt;
    juce::ComboBox presetBox;
    Meter inMeter { "IN", false }, grMeter { "GR", true }, outMeter { "OUT", false };

    struct Card { juce::String title; juce::Colour accent; juce::Rectangle<int> bounds; };
    std::vector<Card> cards;
    float level = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxChainEditor)
};
