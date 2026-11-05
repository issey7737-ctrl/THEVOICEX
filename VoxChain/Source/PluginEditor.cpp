#include "PluginEditor.h"

static juce::String T (const char* s) { return juce::String::fromUTF8 (s); }

static const juce::Colour cTone (0xff3de0ff), cDyn (0xffa27bff), cColor (0xffff9a4d), cDelay (0xffff5fa2), cReverb (0xff4dffb5);

//==============================================================================
VoxLookAndFeel::VoxLookAndFeel()
{
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff15161f));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3de0ff).withAlpha (0.35f));
}

void VoxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    const float ang = startAngle + pos * (endAngle - startAngle);

    // corps du bouton
    const float body = r * 0.72f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34384a), c.x, c.y - body, juce::Colour (0xff12131a), c.x, c.y + body, false));
    g.fillEllipse (c.x - body, c.y - body, body * 2, body * 2);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - body, c.y - body, body * 2, body * 2, 1.0f);

    // piste + arc de valeur lumineux
    juce::Path track;
    track.addCentredArc (c.x, c.y, r * 0.92f, r * 0.92f, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path v;
    v.addCentredArc (c.x, c.y, r * 0.92f, r * 0.92f, 0.0f, startAngle, juce::jmax (startAngle + 0.001f, ang), true);
    g.setColour (accent.withAlpha (0.22f));
    g.strokePath (v, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (v, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // aiguille
    const float sn = std::sin (ang), cs = std::cos (ang);
    g.setColour (juce::Colours::white);
    g.drawLine (c.x + body * 0.35f * sn, c.y - body * 0.35f * cs, c.x + body * 0.85f * sn, c.y - body * 0.85f * cs, 2.2f);
}

void VoxLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const auto accent = cDelay;
    g.setColour (on ? accent.withAlpha (0.30f) : juce::Colours::white.withAlpha (highlighted ? 0.10f : 0.06f));
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (on ? accent : juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (r, 10.0f, on ? 1.6f : 1.0f);
}

void VoxLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawRoundedRectangle (r, 12.0f, 1.0f);
    juce::Path p;
    p.addTriangle ((float) w - 26.0f, (float) h * 0.42f, (float) w - 14.0f, (float) h * 0.42f, (float) w - 20.0f, (float) h * 0.62f);
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.fillPath (p);
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name,
            juce::Colour accent, const juce::String& suffix, int decimals)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.88f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.62f));
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (18));
    slider.setBounds (b);
}

//==============================================================================
void Meter::setDb (float db)
{
    float target;
    if (reduction) target = juce::jlimit (0.f, 1.f, -db / 20.f);
    else           target = juce::jlimit (0.f, 1.f, (db + 60.f) / 60.f);
    shown = target > shown ? target : shown + (target - shown) * 0.18f; // montee instantanee, descente douce
    repaint();
}

void Meter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto label = b.removeFromBottom (18.0f);
    auto bar = b.withSizeKeepingCentre (14.0f, b.getHeight()).reduced (0.f, 4.f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRoundedRectangle (bar, 7.0f);

    const float fillH = bar.getHeight() * shown;
    juce::Rectangle<float> fill = reduction ? juce::Rectangle<float> (bar.getX(), bar.getY(), bar.getWidth(), fillH)
                                            : juce::Rectangle<float> (bar.getX(), bar.getBottom() - fillH, bar.getWidth(), fillH);
    if (fillH > 1.0f)
    {
        if (reduction)
            g.setGradientFill (juce::ColourGradient (cDelay, bar.getX(), bar.getY(), juce::Colour (0xffa27bff), bar.getX(), bar.getBottom(), false));
        else
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff5f6d), bar.getX(), bar.getY(),
                                                     juce::Colour (0xff3dffb0), bar.getX(), bar.getBottom(), false));
        g.fillRoundedRectangle (fill, 7.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRoundedRectangle (fill.expanded (4.0f, 0.0f), 9.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (name, label.toNearestInt(), juce::Justification::centred);
}

//==============================================================================
Knob* VoxChainEditor::addKnob (std::vector<std::unique_ptr<Knob>>& group, const char* id, const char* name, juce::Colour accent,
                               const char* suffix, int decimals)
{
    group.push_back (std::make_unique<Knob> (proc.apvts, id, T (name), accent, T (suffix), decimals));
    addAndMakeVisible (*group.back());
    return group.back().get();
}

VoxChainEditor::VoxChainEditor (VoxChainProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    addKnob (tone, "lowCut", "LOW CUT", cTone, " Hz", 0);
    addKnob (tone, "eqLow", "LOW", cTone, " dB", 1);
    addKnob (tone, "eqBody", "BODY", cTone, " dB", 1);
    addKnob (tone, "eqPres", "PRESENCE", cTone, " dB", 1);
    addKnob (tone, "eqAir", "AIR", cTone, " dB", 1);

    addKnob (dyn, "deess", "DE-ESS", cDyn, " %", 0);
    addKnob (dyn, "compThr", "THRESHOLD", cDyn, " dB", 1);
    addKnob (dyn, "compRatio", "RATIO", cDyn, " :1", 1);
    addKnob (dyn, "compAtk", "ATTACK", cDyn, " ms", 1);
    addKnob (dyn, "compRel", "RELEASE", cDyn, " ms", 0);
    addKnob (dyn, "compMakeup", "MAKEUP", cDyn, " dB", 1);
    addKnob (dyn, "compMix", "MIX", cDyn, " %", 0);

    addKnob (color, "sat", "WARMTH", cColor, " %", 0);
    addKnob (color, "dbl", "DOUBLER", cColor, " %", 0);

    addKnob (delay, "dlyTime", "TIME", cDelay, " ms", 0);
    addKnob (delay, "dlyFb", "FEEDBACK", cDelay, " %", 0);
    addKnob (delay, "dlyTone", "TONE", cDelay, " Hz", 0);
    addKnob (delay, "dlyMix", "MIX", cDelay, " %", 0);

    addKnob (reverb, "revSize", "SIZE", cReverb, " %", 0);
    addKnob (reverb, "revDamp", "DAMPING", cReverb, " %", 0);
    addKnob (reverb, "revMix", "MIX", cReverb, " %", 0);

    addKnob (levels, "inGain", "INPUT", juce::Colours::white, " dB", 1);
    addKnob (levels, "outGain", "OUTPUT", juce::Colours::white, " dB", 1);

    pingBtn.setClickingTogglesState (true);
    pingBtn.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.7f));
    pingBtn.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible (pingBtn);
    pingAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "dlyPing", pingBtn);

    presetBox.addItemList (VoxChainProcessor::presetNames(), 1);
    presetBox.setTextWhenNothingSelected (T ("Choisir un preset…"));
    presetBox.onChange = [this] { proc.applyPreset (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    addAndMakeVisible (inMeter);
    addAndMakeVisible (grMeter);
    addAndMakeVisible (outMeter);

    setSize (1180, 660);
    startTimerHz (60);
}

VoxChainEditor::~VoxChainEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoxChainEditor::timerCallback()
{
    auto toDb = [] (float lin) { return 20.0f * std::log10 (lin + 1e-6f); };
    const float inDb = toDb (proc.engine.inPk.exchange (0.f));
    const float outLin = proc.engine.outPk.exchange (0.f);
    const float outDb = toDb (outLin);
    inMeter.setDb (inDb);
    outMeter.setDb (outDb);
    grMeter.setDb (proc.engine.grDb.load());

    const float target = juce::jlimit (0.f, 1.f, (outDb + 50.f) / 50.f);
    level += (target - level) * 0.12f;
    repaint();
}

void VoxChainEditor::paint (juce::Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight();
    const float t = (float) juce::Time::getMillisecondCounterHiRes() * 0.001f;

    g.fillAll (juce::Colour (0xff0a0b11));

    auto blob = [&] (float cx, float cy, float rad, juce::Colour c)
    {
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.20f + 0.30f * level), cx, cy, c.withAlpha (0.0f), cx + rad, cy, true));
        g.fillEllipse (cx - rad, cy - rad, rad * 2, rad * 2);
    };
    blob (W * (0.18f + 0.08f * std::sin (t * 0.30f)), H * (0.25f + 0.10f * std::cos (t * 0.25f)), 460.f, cTone);
    blob (W * (0.62f + 0.10f * std::cos (t * 0.22f)), H * (0.20f + 0.08f * std::sin (t * 0.35f)), 440.f, cDyn);
    blob (W * (0.40f + 0.12f * std::sin (t * 0.18f)), H * (0.88f + 0.06f * std::cos (t * 0.30f)), 520.f, cDelay);
    blob (W * (0.90f + 0.05f * std::cos (t * 0.40f)), H * (0.80f + 0.08f * std::sin (t * 0.28f)), 380.f, cReverb);

    // logo
    g.setGradientFill (juce::ColourGradient (cTone, 24.f, 20.f, cDelay, 250.f, 20.f, false));
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("VOXCHAIN", 24, 14, 260, 38, juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.setFont (juce::FontOptions (12.5f));
    g.drawText (T ("Chaîne voix : EQ · De-esser · Compresseur · Saturation · Delay · Reverb"), 24, 48, 520, 18, juce::Justification::centredLeft);

    // cartes en verre
    for (auto& c : cards)
    {
        auto r = c.bounds.toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.055f));
        g.fillRoundedRectangle (r, 18.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (r, 18.0f, 1.0f);

        g.setColour (c.accent.withAlpha (0.18f + 0.1f * level));
        g.fillRoundedRectangle (r.getX() + 16.f, r.getY() + 14.f, 7.f, 7.f, 3.5f);
        g.setColour (c.accent);
        g.fillEllipse (r.getX() + 16.f, r.getY() + 14.f, 7.f, 7.f);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
        g.drawText (c.title, (int) r.getX() + 30, (int) r.getY() + 8, 220, 20, juce::Justification::centredLeft);
    }
}

void VoxChainEditor::placeGroup (juce::Rectangle<int> card, std::vector<juce::Component*> comps, int slots)
{
    if (comps.empty()) return;
    auto inner = card.reduced (10, 0).withTrimmedTop (34).withTrimmedBottom (10);
    const int count = slots > 0 ? slots : (int) comps.size();
    const int w = inner.getWidth() / count;
    const int h = juce::jmin (inner.getHeight(), w + 40);
    const int y = inner.getY() + (inner.getHeight() - h) / 2;
    for (size_t i = 0; i < comps.size(); ++i)
        comps[i]->setBounds (inner.getX() + (int) i * w, y, w, h);
}

void VoxChainEditor::resized()
{
    cards.clear();
    auto area = getLocalBounds().reduced (16);
    auto header = area.removeFromTop (62);
    presetBox.setBounds (header.removeFromRight (230).withSizeKeepingCentre (230, 36));
    area.removeFromTop (6);

    auto right = area.removeFromRight (150);
    area.removeFromRight (12);

    const int rowH = (area.getHeight() - 12) / 2;
    auto row1 = area.removeFromTop (rowH);
    area.removeFromTop (12);
    auto row2 = area;

    // ligne 1 : tonalite (5) + dynamique (7)
    const int w1 = (row1.getWidth() - 12) * 5 / 12;
    auto cToneR = row1.removeFromLeft (w1);
    row1.removeFromLeft (12);
    auto cDynR = row1;
    // ligne 2 : couleur (2) + delay (5 emplacements) + reverb (3)
    const int unit = (row2.getWidth() - 24) / 10;
    auto cColorR = row2.removeFromLeft (unit * 2);
    row2.removeFromLeft (12);
    auto cDelayR = row2.removeFromLeft (unit * 5);
    row2.removeFromLeft (12);
    auto cReverbR = row2;

    auto add = [this] (const char* title, juce::Colour ac, juce::Rectangle<int> r) { cards.push_back ({ juce::String (title), ac, r }); };
    add ("CLEAN & TONE", cTone, cToneR);
    add ("DYNAMICS", cDyn, cDynR);
    add ("COLOR", cColor, cColorR);
    add ("DELAY", cDelay, cDelayR);
    add ("REVERB", cReverb, cReverbR);
    add ("LEVELS", juce::Colours::white, right);

    auto ptrs = [] (std::vector<std::unique_ptr<Knob>>& v)
    {
        std::vector<juce::Component*> out;
        for (auto& k : v) out.push_back (k.get());
        return out;
    };
    placeGroup (cToneR, ptrs (tone));
    placeGroup (cDynR, ptrs (dyn));
    placeGroup (cColorR, ptrs (color));
    placeGroup (cDelayR, ptrs (delay), 5);
    placeGroup (cReverbR, ptrs (reverb));

    {   // bouton ping-pong : 5e emplacement de la carte delay
        auto inner = cDelayR.reduced (10, 0).withTrimmedTop (34).withTrimmedBottom (10);
        const int w = inner.getWidth() / 5;
        pingBtn.setBounds (inner.getX() + 4 * w + 6, inner.getCentreY() - 18, w - 12, 36);
    }

    {   // colonne de droite : jauges + gains
        auto r = right;
        r.removeFromTop (34);
        auto knobs = r.removeFromBottom (124).reduced (4, 0);
        auto meters = r.reduced (8, 4);
        const int mw = meters.getWidth() / 3;
        inMeter.setBounds (meters.removeFromLeft (mw));
        grMeter.setBounds (meters.removeFromLeft (mw));
        outMeter.setBounds (meters);
        const int kw = knobs.getWidth() / 2;
        levels[0]->setBounds (knobs.removeFromLeft (kw));
        levels[1]->setBounds (knobs);
    }
}
