#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

namespace yvui
{
using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

namespace col
{
    const juce::Colour bg        { 0xff060606 };
    const juce::Colour panel     { 0xff101010 };
    const juce::Colour panelHi   { 0xff171212 };
    const juce::Colour edge      { 0xff2b0d10 };
    const juce::Colour redDark   { 0xff6b0f1a };
    const juce::Colour red       { 0xffd4152a };
    const juce::Colour redBright { 0xffff3b4e };
    const juce::Colour text      { 0xffe9e3e3 };
    const juce::Colour textDim   { 0xff8d7f80 };
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

class KnobCell : public juce::Component
{
public:
    KnobCell (YazaVoxProcessor&, int paramIndex);
    void resized() override;
    juce::Label name;
    juce::Slider slider;
private:
    std::unique_ptr<SliderAtt> att;
};

class ToggleCell : public juce::Component
{
public:
    ToggleCell (YazaVoxProcessor&, int paramIndex);
    void resized() override;
    juce::ToggleButton btn;
private:
    std::unique_ptr<ButtonAtt> att;
};

class ChoiceCell : public juce::Component
{
public:
    ChoiceCell (YazaVoxProcessor&, int paramIndex);
    void resized() override;
    juce::Label name;
    juce::ComboBox box;
private:
    std::unique_ptr<ComboAtt> att;
};

class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (juce::String t) : title (std::move (t)) {}
    void push (float peakLin);
    void paint (juce::Graphics&) override;
private:
    juce::String title;
    float level = 0.f, hold = 0.f; int holdCount = 0, clipCount = 0;
};

class GrMeter : public juce::Component
{
public:
    void push (float grDb);
    void paint (juce::Graphics&) override;
private:
    float gr = 0.f;
};

class EqView : public juce::Component
{
public:
    explicit EqView (YazaVoxProcessor&);
    void update();   // 30 Hz : lit l'analyseur
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    static constexpr int kOrder = 12, kN = 1 << kOrder;
    struct BandIdx { int f, g, q; };
    float xOf (double f) const;
    double fOf (float x) const;
    float yOfDb (double db) const;
    int hit (juce::Point<float>) const;
    void setNorm (int idx, float real, bool gesture);

    YazaVoxProcessor& proc;
    juce::dsp::FFT fft { kOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) kN, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, kN> ring {};
    std::array<float, 2 * kN> work {};
    std::array<float, kN / 2> spec {};
    int wpos = 0;
    int dragBand = -1;
    std::array<BandIdx, 4> bands;
};

class ModulePanel : public juce::Component
{
public:
    ModulePanel (YazaVoxProcessor&, int module, int rowCells);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

    juce::Component* extraTop = nullptr;
    float extraFrac = 0.f;
    juce::Component* headerExtra = nullptr;

private:
    struct Group
    {
        juce::String name;
        int toggleIdx = -1;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<ButtonAtt> toggleAtt;
        std::vector<std::unique_ptr<juce::Component>> cells;
        juce::Rectangle<int> header;
    };
    YazaVoxProcessor& proc;
    int module, rowCells, onIdx = -1;
    std::unique_ptr<juce::ToggleButton> power;
    std::unique_ptr<ButtonAtt> powerAtt;
    std::vector<std::unique_ptr<Group>> groups;
};

class YazaVoxEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit YazaVoxEditor (YazaVoxProcessor&);
    ~YazaVoxEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectPreset (int i);

    YazaVoxProcessor& proc;
    LookAndFeel laf;

    juce::ComboBox presets;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, abBtn { "A" }, copyBtn { "A>B" }, resetBtn { "RESET" };
    juce::ToggleButton bypassBtn { "BYPASS" };
    std::unique_ptr<ButtonAtt> bypassAtt;
    KnobCell inKnob, outKnob;
    LevelMeter inMeter { "IN" }, outMeter { "OUT" };

    GrMeter grMeter;
    EqView eqView;
    ModulePanel comp, eq, fx, dly, rev;
    juce::Rectangle<int> logoRect, headerRect;
    int tick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YazaVoxEditor)
};
} // namespace yvui

using YazaVoxEditor = yvui::YazaVoxEditor;
