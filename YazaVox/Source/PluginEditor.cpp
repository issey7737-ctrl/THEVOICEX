#include "PluginEditor.h"

using namespace yv;

namespace yvui
{
//==============================================================================
static juce::String idOf (int i) { return kDefs[i].id; }
static bool isOnId (int i) { return juce::String (kDefs[i].id).endsWith ("On"); }
static juce::Font bold (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
static juce::Font plain (float h) { return juce::Font (juce::FontOptions (h)); }

LookAndFeel::LookAndFeel()
{
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::outlineColourId, col::edge);
    setColour (juce::ComboBox::backgroundColourId, col::panelHi);
    setColour (juce::ComboBox::arrowColourId, col::red);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff120a0b));
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::redDark);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, col::redDark);
    setColour (juce::Label::textColourId, col::textDim);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1c0f11));
    setColour (juce::TextEditor::textColourId, col::text);
    setColour (juce::TextEditor::highlightColourId, col::redDark);
    setColour (juce::CaretComponent::caretColourId, col::redBright);
}

juce::Label* LookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float d = std::min (b.getWidth(), b.getHeight()) - 4.f;
    if (d < 8.f) return;
    const auto c = b.getCentre();
    const float r = d * 0.5f, track = std::max (2.5f, d * 0.075f);
    const float ang = a0 + pos * (a1 - a0);
    const bool en = s.isEnabled();

    // halo
    if (en && pos > 0.001f)
    {
        juce::ColourGradient halo (col::red.withAlpha (0.18f), c.x, c.y, juce::Colours::transparentBlack, c.x, c.y - r * 1.25f, true);
        g.setGradientFill (halo);
        g.fillEllipse (c.x - r * 1.25f, c.y - r * 1.25f, r * 2.5f, r * 2.5f);
    }
    // piste
    juce::Path bgArc;
    bgArc.addCentredArc (c.x, c.y, r - track * 0.5f, r - track * 0.5f, 0.f, a0, a1, true);
    g.setColour (juce::Colour (0xff2a1a1c));
    g.strokePath (bgArc, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    // valeur : depuis le centre pour les plages bipolaires
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zero = bipolar ? (float) s.valueToProportionOfLength (0.0) : 0.f;
    const float aZero = a0 + zero * (a1 - a0);
    juce::Path vArc;
    vArc.addCentredArc (c.x, c.y, r - track * 0.5f, r - track * 0.5f, 0.f, bipolar ? aZero : a0, ang, true);
    g.setGradientFill (juce::ColourGradient (col::redDark, c.x - r, c.y + r, col::redBright, c.x + r, c.y - r, false));
    g.strokePath (vArc, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    // corps
    const float rb = r - track * 1.7f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2224), c.x, c.y - rb, juce::Colour (0xff0d0909), c.x, c.y + rb, false));
    g.fillEllipse (c.x - rb, c.y - rb, rb * 2.f, rb * 2.f);
    g.setColour (juce::Colour (0xff3a1c20));
    g.drawEllipse (c.x - rb, c.y - rb, rb * 2.f, rb * 2.f, 1.f);
    // index
    juce::Path p;
    p.addRoundedRectangle (-1.6f, -rb + 2.f, 3.2f, rb * 0.42f, 1.4f);
    g.setColour (en ? col::redBright : col::textDim);
    g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    if (b.getButtonText().isEmpty())
    {
        // petit interrupteur lumineux (ON de module / de groupe)
        const float d = std::min (r.getWidth(), r.getHeight()) - 2.f;
        const auto c = r.getCentre();
        if (on)
        {
            g.setColour (col::red.withAlpha (0.25f));
            g.fillEllipse (c.x - d * 0.85f, c.y - d * 0.85f, d * 1.7f, d * 1.7f);
        }
        g.setGradientFill (juce::ColourGradient (on ? col::redBright : juce::Colour (0xff2b2022), c.x, c.y - d / 2, on ? col::redDark : juce::Colour (0xff120c0d), c.x, c.y + d / 2, false));
        g.fillEllipse (c.x - d / 2, c.y - d / 2, d, d);
        g.setColour (over ? col::redBright : col::edge.brighter (0.3f));
        g.drawEllipse (c.x - d / 2, c.y - d / 2, d, d, 1.2f);
        return;
    }
    const auto pill = r.reduced (2.f);
    g.setColour (on ? col::redDark : juce::Colour (0xff1a1213));
    g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);
    g.setColour (on ? col::redBright : (over ? col::textDim : col::edge.brighter (0.3f)));
    g.drawRoundedRectangle (pill, pill.getHeight() * 0.5f, 1.2f);
    g.setColour (on ? juce::Colours::white : col::textDim);
    g.setFont (bold (std::clamp (pill.getHeight() * 0.5f, 9.f, 15.f)));
    g.drawText (b.getButtonText(), pill.toNearestInt(), juce::Justification::centred);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool hot = b.getToggleState() || down;
    g.setGradientFill (juce::ColourGradient (hot ? col::red : (over ? juce::Colour (0xff2a1b1d) : juce::Colour (0xff1d1415)), 0, r.getY(),
                                             hot ? col::redDark : juce::Colour (0xff130d0e), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (over || hot ? col::redBright.withAlpha (0.8f) : col::edge.brighter (0.4f));
    g.drawRoundedRectangle (r, 6.f, 1.f);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.f);
    g.setColour (juce::Colour (0xff170f10));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (box.hasKeyboardFocus (false) ? col::redBright : col::edge.brighter (0.4f));
    g.drawRoundedRectangle (r, 6.f, 1.f);
    juce::Path arrow;
    const float ax = (float) w - 14.f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour (col::red);
    g.fillPath (arrow);
}
juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox& b) { return bold (std::clamp ((float) b.getHeight() * 0.5f, 10.f, 16.f)); }
juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return bold (std::clamp ((float) h * 0.42f, 10.f, 15.f)); }

//==============================================================================
static void initLabel (juce::Label& l, const juce::String& t)
{
    l.setText (t, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setInterceptsMouseClicks (false, false);
    l.setMinimumHorizontalScale (0.6f);
}

KnobCell::KnobCell (YazaVoxProcessor& p, int idx)
{
    const auto& d = kDefs[idx];
    initLabel (name, d.label);
    addAndMakeVisible (name);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setTextValueSuffix (d.unit);
    slider.setNumDecimalPlacesToDisplay (d.step >= 1.f ? 0 : (d.step >= 0.1f ? 1 : 2));
    slider.setDoubleClickReturnValue (true, d.def);
    slider.setMouseDragSensitivity (220);
    addAndMakeVisible (slider);
    att = std::make_unique<SliderAtt> (p.apvts, idOf (idx), slider);
}
void KnobCell::resized()
{
    auto r = getLocalBounds();
    const int lh = std::clamp (r.getHeight() / 7, 11, 18);
    name.setFont (bold ((float) lh * 0.78f));
    name.setBounds (r.removeFromTop (lh));
    const int th = std::clamp (r.getHeight() / 6, 13, 20);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, std::max (40, r.getWidth() - 2), th);
    slider.getLookAndFeel().setColour (juce::Slider::textBoxTextColourId, col::text);
    slider.setBounds (r);
}

ToggleCell::ToggleCell (YazaVoxProcessor& p, int idx)
{
    btn.setButtonText (kDefs[idx].label);
    addAndMakeVisible (btn);
    att = std::make_unique<ButtonAtt> (p.apvts, idOf (idx), btn);
}
void ToggleCell::resized()
{
    auto r = getLocalBounds();
    const int h = std::clamp (r.getHeight() / 3, 20, 32);
    btn.setBounds (r.withSizeKeepingCentre (r.getWidth() - 4, h));
}

ChoiceCell::ChoiceCell (YazaVoxProcessor& p, int idx)
{
    initLabel (name, kDefs[idx].label);
    addAndMakeVisible (name);
    juce::StringArray items; items.addTokens (kDefs[idx].unit, "|", "");
    box.addItemList (items, 1);
    addAndMakeVisible (box);
    att = std::make_unique<ComboAtt> (p.apvts, idOf (idx), box);
}
void ChoiceCell::resized()
{
    auto r = getLocalBounds();
    const int lh = std::clamp (r.getHeight() / 7, 11, 18);
    name.setFont (bold ((float) lh * 0.78f));
    name.setBounds (r.removeFromTop (lh));
    const int h = std::clamp (r.getHeight() / 3, 22, 32);
    box.setBounds (r.withSizeKeepingCentre (r.getWidth() - 6, h).translated (0, -r.getHeight() / 8));
}

//==============================================================================
void LevelMeter::push (float pk)
{
    const float db = (float) linToDb (pk);
    const float v = juce::jlimit (0.f, 1.f, (db + 60.f) / 63.f);
    level = v > level ? v : level - (level - v) * 0.18f;
    if (v >= hold) { hold = v; holdCount = 45; } else if (--holdCount < 0) hold = std::max (0.f, hold - 0.02f);
    if (pk >= 0.995f) clipCount = 60; else if (clipCount > 0) --clipCount;
    repaint();
}
void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::textDim);
    g.setFont (bold (std::clamp (r.getWidth() * 0.9f, 9.f, 12.f)));
    g.drawText (title, r.removeFromBottom (13.f).toNearestInt(), juce::Justification::centred);
    r.reduce (1.f, 2.f);
    g.setColour (juce::Colour (0xff0b0707));
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r, 3.f, 1.f);
    auto in = r.reduced (2.f);
    const float hgt = in.getHeight() * level;
    juce::Rectangle<float> bar (in.getX(), in.getBottom() - hgt, in.getWidth(), hgt);
    g.setGradientFill (juce::ColourGradient (col::redBright, in.getX(), in.getY(), col::redDark, in.getX(), in.getBottom(), false));
    g.fillRoundedRectangle (bar, 2.f);
    // graduations
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    for (int d : { -48, -36, -24, -12, -6, 0 })
    {
        const float y = in.getBottom() - in.getHeight() * ((float) d + 60.f) / 63.f;
        g.drawHorizontalLine ((int) y, in.getX(), in.getRight());
    }
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.fillRect (in.getX(), in.getBottom() - in.getHeight() * hold - 1.f, in.getWidth(), 2.f);
    if (clipCount > 0) { g.setColour (juce::Colours::white); g.fillRect (in.getX(), in.getY(), in.getWidth(), 3.f); }
}

void GrMeter::push (float grDb)
{
    const float target = juce::jlimit (0.f, 24.f, -grDb);
    gr = target > gr ? target : gr - (gr - target) * 0.2f;
    repaint();
}
void GrMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::textDim);
    g.setFont (bold (std::clamp (r.getHeight() * 0.5f, 9.f, 12.f)));
    auto lab = r.removeFromLeft (26.f);
    g.drawText ("GR", lab.toNearestInt(), juce::Justification::centredLeft);
    auto txt = r.removeFromRight (46.f);
    g.setColour (col::text);
    g.drawText (juce::String (-gr, 1) + " dB", txt.toNearestInt(), juce::Justification::centredRight);
    r = r.reduced (0.f, r.getHeight() * 0.22f);
    g.setColour (juce::Colour (0xff0b0707));
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r, 3.f, 1.f);
    const float w = (r.getWidth() - 4.f) * (gr / 24.f);
    g.setGradientFill (juce::ColourGradient (col::redDark, r.getX(), 0, col::redBright, r.getRight(), 0, false));
    g.fillRoundedRectangle (r.getRight() - 2.f - w, r.getY() + 2.f, w, r.getHeight() - 4.f, 2.f);   // la barre se remplit de la droite vers la gauche
}

//==============================================================================
EqView::EqView (YazaVoxProcessor& p) : proc (p)
{
    bands = { BandIdx { lsF, lsG, lsQ }, BandIdx { lmF, lmG, lmQ }, BandIdx { hmF, hmG, hmQ }, BandIdx { hsF, hsG, hsQ } };
    spec.fill (-120.f);
}

float EqView::xOf (double f) const
{
    return (float) getWidth() * (float) ((std::log10 (std::max (20.0, f)) - std::log10 (20.0)) / 3.0);
}
double EqView::fOf (float x) const
{
    return std::pow (10.0, std::log10 (20.0) + 3.0 * (double) x / std::max (1, getWidth()));
}
float EqView::yOfDb (double db) const
{
    return (float) getHeight() * (0.5f - (float) db / 36.f);   // +-18 dB
}

void EqView::update()
{
    auto& e = proc.engine;
    int s1, z1, s2, z2;
    const int ready = e.scopeFifo.getNumReady();
    if (ready <= 0) return;
    e.scopeFifo.prepareToRead (ready, s1, z1, s2, z2);
    auto push = [&] (int start, int n) { for (int i = 0; i < n; ++i) { ring[(size_t) wpos] = e.scopeBuf[(size_t) (start + i)]; wpos = (wpos + 1) & (kN - 1); } };
    push (s1, z1); push (s2, z2);
    e.scopeFifo.finishedRead (z1 + z2);

    for (int i = 0; i < kN; ++i) work[(size_t) i] = ring[(size_t) ((wpos + i) & (kN - 1))];
    std::fill (work.begin() + kN, work.end(), 0.f);
    window.multiplyWithWindowingTable (work.data(), (size_t) kN);
    fft.performFrequencyOnlyForwardTransform (work.data());
    for (int b = 1; b < kN / 2; ++b)
    {
        const float db = 20.f * std::log10 (work[(size_t) b] / (kN * 0.25f) + 1e-7f);
        float& s = spec[(size_t) b];
        s = db > s ? s + (db - s) * 0.5f : s + (db - s) * 0.12f;
    }
    repaint();
}

void EqView::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff130b0c), 0, 0, juce::Colour (0xff070505), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.f);

    // grille
    g.setFont (plain (std::clamp (r.getHeight() * 0.075f, 8.f, 11.f)));
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        const float x = xOf (f);
        g.setColour (juce::Colour (0xff241517));
        g.drawVerticalLine ((int) x, 0.f, r.getBottom());
        g.setColour (col::textDim.withAlpha (0.7f));
        g.drawText (f >= 1000 ? juce::String (f / 1000.0, f >= 10000 ? 0 : (f == 1000.0 || f == 2000.0 || f == 5000.0 ? 0 : 1)) + "k" : juce::String ((int) f),
                    (int) x + 2, (int) r.getBottom() - 13, 30, 12, juce::Justification::left);
    }
    for (int db : { -12, -6, 0, 6, 12 })
    {
        g.setColour (db == 0 ? juce::Colour (0xff3a1a1e) : juce::Colour (0xff1c1112));
        g.drawHorizontalLine ((int) yOfDb (db), 0.f, r.getRight());
    }

    // spectre (reference : 0 dBFS en haut, -90 en bas)
    const double sr = proc.getSampleRate_();
    juce::Path sp;
    const int W = getWidth();
    bool started = false;
    for (int px = 0; px < W; px += 2)
    {
        const double f0 = fOf ((float) px), f1 = fOf ((float) px + 2.f);
        const int b0 = std::max (1, (int) (f0 / sr * kN)), b1 = std::max (b0 + 1, std::min (kN / 2 - 1, (int) (f1 / sr * kN) + 1));
        float m = -120.f;
        for (int b = b0; b < b1; ++b) m = std::max (m, spec[(size_t) b]);
        const float y = r.getHeight() * (1.f - juce::jlimit (0.f, 1.f, (m + 90.f) / 90.f));
        if (! started) { sp.startNewSubPath (0.f, r.getBottom()); sp.lineTo ((float) px, y); started = true; }
        else sp.lineTo ((float) px, y);
    }
    if (started)
    {
        sp.lineTo ((float) W, r.getBottom()); sp.closeSubPath();
        g.setGradientFill (juce::ColourGradient (col::red.withAlpha (0.55f), 0, 0, col::redDark.withAlpha (0.05f), 0, r.getBottom(), false));
        g.fillPath (sp);
    }

    // courbe de reponse
    float v[Count];
    for (int i = 0; i < Count; ++i) v[i] = proc.getParam (i);
    const bool eqActive = v[eqOn] > 0.5f;
    juce::Path curve;
    for (int px = 0; px <= W; px += 2)
    {
        const double db = EQ::responseDb (v, sr, fOf ((float) px));
        const float y = yOfDb (juce::jlimit (-18.0, 18.0, db));
        if (px == 0) curve.startNewSubPath (0.f, y); else curve.lineTo ((float) px, y);
    }
    g.setColour (col::redBright.withAlpha (eqActive ? 0.35f : 0.12f));
    g.strokePath (curve, juce::PathStrokeType (5.f));
    g.setColour (eqActive ? juce::Colours::white : col::textDim);
    g.strokePath (curve, juce::PathStrokeType (1.8f));

    // poignees
    for (size_t i = 0; i < bands.size(); ++i)
    {
        const auto& b = bands[i];
        const juce::Point<float> c (xOf (v[b.f]), yOfDb (juce::jlimit (-18.0, 18.0, (double) v[b.g])));
        const bool on = std::abs (v[b.g]) > 0.01f;
        g.setColour (col::red.withAlpha (on ? 0.3f : 0.12f));
        g.fillEllipse (c.x - 11.f, c.y - 11.f, 22.f, 22.f);
        g.setColour (on || dragBand == (int) i ? col::redBright : col::redDark);
        g.fillEllipse (c.x - 5.f, c.y - 5.f, 10.f, 10.f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawEllipse (c.x - 5.f, c.y - 5.f, 10.f, 10.f, 1.2f);
    }
    g.setColour (col::edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.f);
}

int EqView::hit (juce::Point<float> p) const
{
    int best = -1; float bd = 18.f;
    for (size_t i = 0; i < bands.size(); ++i)
    {
        const juce::Point<float> c (xOf (proc.getParam (bands[i].f)), yOfDb (juce::jlimit (-18.0, 18.0, (double) proc.getParam (bands[i].g))));
        const float d = c.getDistanceFrom (p);
        if (d < bd) { bd = d; best = (int) i; }
    }
    return best;
}

void EqView::setNorm (int idx, float real, bool)
{
    if (auto* p = proc.apvts.getParameter (kDefs[idx].id))
        p->setValueNotifyingHost (p->convertTo0to1 (real));
}

void EqView::mouseDown (const juce::MouseEvent& e)
{
    dragBand = hit (e.position);
    if (dragBand >= 0)
    {
        proc.apvts.getParameter (kDefs[bands[(size_t) dragBand].f].id)->beginChangeGesture();
        proc.apvts.getParameter (kDefs[bands[(size_t) dragBand].g].id)->beginChangeGesture();
    }
}
void EqView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0) return;
    const auto& b = bands[(size_t) dragBand];
    const auto& df = kDefs[b.f]; const auto& dg = kDefs[b.g];
    const float f = juce::jlimit (df.lo, df.hi, (float) fOf (e.position.x));
    const float db = juce::jlimit (dg.lo, dg.hi, (0.5f - e.position.y / (float) getHeight()) * 36.f);
    setNorm (b.f, f, false);
    setNorm (b.g, std::round (db * 10.f) / 10.f, false);
}
void EqView::mouseUp (const juce::MouseEvent&)
{
    if (dragBand >= 0)
    {
        proc.apvts.getParameter (kDefs[bands[(size_t) dragBand].f].id)->endChangeGesture();
        proc.apvts.getParameter (kDefs[bands[(size_t) dragBand].g].id)->endChangeGesture();
    }
    dragBand = -1;
}
void EqView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int h = hit (e.position);
    if (h >= 0) setNorm (bands[(size_t) h].g, 0.f, false);
}
void EqView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int h = hit (e.position);
    if (h < 0) return;
    const auto& q = kDefs[bands[(size_t) h].q];
    const float nq = juce::jlimit (q.lo, q.hi, proc.getParam (bands[(size_t) h].q) * (1.f + w.deltaY * 0.5f));
    if (auto* p = proc.apvts.getParameter (q.id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (nq)); p->endChangeGesture(); }
}

//==============================================================================
ModulePanel::ModulePanel (YazaVoxProcessor& p, int m, int rc) : proc (p), module (m), rowCells (rc)
{
    for (int i = 0; i < Count; ++i)
        if (kDefs[i].module == m && kDefs[i].group[0] == 0 && isOnId (i)) onIdx = i;
    power = std::make_unique<juce::ToggleButton>();
    addAndMakeVisible (*power);
    if (onIdx >= 0) powerAtt = std::make_unique<ButtonAtt> (p.apvts, idOf (onIdx), *power);

    for (int i = 0; i < Count; ++i)
    {
        const auto& d = kDefs[i];
        if (d.module != m || d.group[0] == 0) continue;
        Group* grp = nullptr;
        for (auto& g : groups) if (g->name == d.group) grp = g.get();
        if (grp == nullptr)
        {
            groups.push_back (std::make_unique<Group>());
            grp = groups.back().get();
            grp->name = d.group;
        }
        if (isOnId (i) && d.kind == KBool)
        {
            grp->toggleIdx = i;
            grp->toggle = std::make_unique<juce::ToggleButton>();
            addAndMakeVisible (*grp->toggle);
            grp->toggleAtt = std::make_unique<ButtonAtt> (p.apvts, idOf (i), *grp->toggle);
        }
        else
        {
            std::unique_ptr<juce::Component> c;
            if (d.kind == KBool) c = std::make_unique<ToggleCell> (p, i);
            else if (d.kind == KChoice) c = std::make_unique<ChoiceCell> (p, i);
            else c = std::make_unique<KnobCell> (p, i);
            addAndMakeVisible (*c);
            grp->cells.push_back (std::move (c));
        }
    }
}

void ModulePanel::refresh()
{
    const bool modOn = onIdx < 0 || proc.getParam (onIdx) > 0.5f;
    for (auto& g : groups)
    {
        const bool gOn = g->toggleIdx < 0 || proc.getParam (g->toggleIdx) > 0.5f;
        const float a = (modOn ? 1.f : 0.35f) * (gOn ? 1.f : 0.5f);
        for (auto& c : g->cells) c->setAlpha (a);
        if (g->toggle) g->toggle->setAlpha (modOn ? 1.f : 0.5f);
    }
}

void ModulePanel::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.f);
    g.setGradientFill (juce::ColourGradient (col::panelHi, 0, r.getY(), col::panel, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 10.f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r, 10.f, 1.2f);
    // liseré rouge en haut
    g.setGradientFill (juce::ColourGradient (col::red.withAlpha (0.9f), r.getX() + 12, 0, juce::Colours::transparentBlack, r.getRight() - 12, 0, false));
    g.fillRect (r.getX() + 12.f, r.getY() + 0.5f, r.getWidth() - 24.f, 1.5f);

    const int th = std::clamp ((int) (r.getHeight() * 0.1f), 22, 34);
    g.setColour (col::text);
    g.setFont (bold ((float) th * 0.62f));
    g.drawText (kModuleTitles[module], 36, 4, 120, th, juce::Justification::centredLeft);

    g.setFont (bold (std::clamp ((float) th * 0.4f, 9.f, 12.f)));
    for (auto& grp : groups)
    {
        g.setColour (col::redBright.withAlpha (0.85f));
        auto h = grp->header;
        if (grp->toggle) h.removeFromLeft (h.getHeight() + 2);
        g.drawText (grp->name, h, juce::Justification::centredLeft);
        g.setColour (col::edge);
        g.drawHorizontalLine (grp->header.getBottom(), (float) grp->header.getX(), (float) grp->header.getRight());
    }
}

void ModulePanel::resized()
{
    auto area = getLocalBounds().reduced (8, 6);
    const int th = std::clamp ((int) (getHeight() * 0.1f), 22, 34);
    auto top = area.removeFromTop (th);
    power->setBounds (top.removeFromLeft (th).reduced (th / 5));
    if (headerExtra) headerExtra->setBounds (top.removeFromRight (std::min (top.getWidth(), 240)).reduced (0, 2));
    if (extraTop) extraTop->setBounds (area.removeFromTop ((int) ((float) area.getHeight() * extraFrac)).reduced (0, 2));
    area.removeFromTop (2);

    // repartition en rangees
    std::vector<std::vector<Group*>> rows (1);
    int used = 0;
    for (auto& g : groups)
    {
        const int n = std::max (1, (int) g->cells.size());
        if (used + n > rowCells && used > 0) { rows.emplace_back(); used = 0; }
        rows.back().push_back (g.get());
        used += n;
    }
    const float cellW = (float) area.getWidth() / (float) rowCells;
    const int rowH = area.getHeight() / (int) rows.size();
    const int hh = std::clamp (rowH / 8, 14, 20);
    for (size_t ri = 0; ri < rows.size(); ++ri)
    {
        float x = (float) area.getX();
        const int y = area.getY() + (int) ri * rowH;
        for (auto* g : rows[ri])
        {
            const int n = std::max (1, (int) g->cells.size());
            const int gw = (int) std::round (cellW * (float) n);
            g->header = { (int) x + 3, y, gw - 8, hh };
            if (g->toggle) g->toggle->setBounds (g->header.getX(), g->header.getY() + 1, hh - 2, hh - 2);
            for (size_t k = 0; k < g->cells.size(); ++k)
                g->cells[k]->setBounds ((int) std::round (x + cellW * (float) k) + 1, y + hh + 3, (int) std::round (cellW) - 2, rowH - hh - 6);
            x += cellW * (float) n;
        }
    }
    repaint();
}

//==============================================================================
YazaVoxEditor::YazaVoxEditor (YazaVoxProcessor& p)
    : AudioProcessorEditor (&p), proc (p), inKnob (p, inGain), outKnob (p, outGain), eqView (p),
      comp (p, MComp, 6), eq (p, MEq, 8), fx (p, MFx, 7), dly (p, MDly, 7), rev (p, MRev, 5)
{
    setLookAndFeel (&laf);

    presets.addItemList (YazaVoxProcessor::presetNames(), 1);
    presets.setSelectedItemIndex (proc.getCurrentPreset(), juce::dontSendNotification);
    presets.onChange = [this] { selectPreset (presets.getSelectedItemIndex()); };
    addAndMakeVisible (presets);

    prevBtn.onClick = [this] { const int n = YazaVoxProcessor::presetNames().size(); selectPreset ((proc.getCurrentPreset() + n - 1) % n); };
    nextBtn.onClick = [this] { selectPreset ((proc.getCurrentPreset() + 1) % YazaVoxProcessor::presetNames().size()); };
    abBtn.onClick = [this] { proc.switchAB(); };
    copyBtn.onClick = [this] { proc.copyToOther(); };
    resetBtn.onClick = [this] { proc.resetAll(); };
    for (auto* b : { &prevBtn, &nextBtn, &abBtn, &copyBtn, &resetBtn }) addAndMakeVisible (*b);

    bypassBtn.setClickingTogglesState (true);
    addAndMakeVisible (bypassBtn);
    bypassAtt = std::make_unique<ButtonAtt> (proc.apvts, "bypass", bypassBtn);

    addAndMakeVisible (inKnob); addAndMakeVisible (outKnob);
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter);

    comp.headerExtra = &grMeter; comp.addAndMakeVisible (grMeter);
    eq.extraTop = &eqView; eq.addAndMakeVisible (eqView); eq.extraFrac = 0.42f;
    for (auto* m : { &comp, &eq, &fx, &dly, &rev }) addAndMakeVisible (*m);

    setResizable (true, true);
    setResizeLimits (900, 600, 1900, 1200);
    getConstrainer()->setFixedAspectRatio (0.0);
    setSize (1180, 740);
    startTimerHz (30);
}

YazaVoxEditor::~YazaVoxEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void YazaVoxEditor::selectPreset (int i)
{
    proc.applyPreset (i);
    presets.setSelectedItemIndex (proc.getCurrentPreset(), juce::dontSendNotification);
}

void YazaVoxEditor::timerCallback()
{
    inMeter.push (proc.engine.inPk.exchange (0.f));
    outMeter.push (proc.engine.outPk.exchange (0.f));
    grMeter.push (proc.engine.grDb.load());
    eqView.update();

    if ((tick++ & 3) == 0)
    {
        for (auto* m : { &comp, &eq, &fx, &dly, &rev }) m->refresh();
        if (presets.getSelectedItemIndex() != proc.getCurrentPreset())
            presets.setSelectedItemIndex (proc.getCurrentPreset(), juce::dontSendNotification);
        const juce::String ab = proc.getAB() == 0 ? "A" : "B";
        if (abBtn.getButtonText() != ab) abBtn.setButtonText (ab);
        const juce::String cp = proc.getAB() == 0 ? "A>B" : "B>A";
        if (copyBtn.getButtonText() != cp) copyBtn.setButtonText (cp);
    }
}

void YazaVoxEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    // lueur de fond
    g.setGradientFill (juce::ColourGradient (col::red.withAlpha (0.10f), (float) getWidth() * 0.15f, 0.f,
                                             juce::Colours::transparentBlack, (float) getWidth() * 0.55f, (float) getHeight() * 0.5f, true));
    g.fillRect (getLocalBounds());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff160b0c), 0, 0, col::bg, 0, (float) headerRect.getBottom(), false));
    g.fillRect (headerRect);
    g.setColour (col::red);
    g.fillRect (0, headerRect.getBottom() - 1, getWidth(), 2);

    g.setColour (col::text);
    g.setFont (bold ((float) logoRect.getHeight() * 0.5f));
    g.drawText ("YAZA", logoRect.withWidth (logoRect.getWidth() / 2 + 6), juce::Justification::centredRight);
    g.setColour (col::redBright);
    g.drawText ("VOX", logoRect.withTrimmedLeft (logoRect.getWidth() / 2 + 10), juce::Justification::centredLeft);
    g.setColour (col::textDim);
    g.setFont (plain (std::clamp ((float) logoRect.getHeight() * 0.17f, 8.f, 11.f)));
    g.drawText ("VOCAL PROCESSING SUITE", logoRect.removeFromBottom (14), juce::Justification::centred);
}

void YazaVoxEditor::resized()
{
    auto r = getLocalBounds();
    const int W = r.getWidth(), H = r.getHeight();
    const int hh = std::clamp ((int) (H * 0.135f), 82, 120);
    headerRect = r.removeFromTop (hh);
    auto h = headerRect.reduced (14, 6);

    logoRect = h.removeFromLeft (std::clamp (W / 6, 150, 230));

    // droite : IN knob, meters, OUT knob, bypass
    auto right = h.removeFromRight (std::clamp (W * 40 / 100, 330, 520));
    const int bw = std::clamp (right.getWidth() / 5, 62, 96);
    bypassBtn.setBounds (right.removeFromRight (bw).withSizeKeepingCentre (bw, std::clamp (hh / 3, 26, 36)));
    right.removeFromRight (8);
    const int kw = std::clamp (right.getWidth() / 4, 64, 100);
    outKnob.setBounds (right.removeFromRight (kw));
    outMeter.setBounds (right.removeFromRight (right.getHeight() > 0 ? 22 : 0).reduced (0, 0));
    right.removeFromRight (4);
    inMeter.setBounds (right.removeFromRight (22));
    right.removeFromRight (4);
    inKnob.setBounds (right.removeFromRight (kw));

    // centre : presets, A/B, copy, reset
    h.removeFromRight (10);
    const int rowH = std::clamp (hh / 3, 26, 34);
    auto c = h.withSizeKeepingCentre (h.getWidth(), rowH * 2 + 6);
    auto row1 = c.removeFromTop (rowH);
    c.removeFromTop (6);
    auto row2 = c;
    prevBtn.setBounds (row1.removeFromLeft (rowH));
    nextBtn.setBounds (row1.removeFromRight (rowH));
    presets.setBounds (row1.reduced (6, 0));
    const int third = row2.getWidth() / 3;
    abBtn.setBounds (row2.removeFromLeft (third).reduced (3, 0));
    copyBtn.setBounds (row2.removeFromLeft (third).reduced (3, 0));
    resetBtn.setBounds (row2.reduced (3, 0));

    // corps
    auto body = r.reduced (10, 8);
    const int rowA = (int) ((float) body.getHeight() * 0.50f);
    auto a = body.removeFromTop (rowA);
    body.removeFromTop (8);
    auto b = body;
    const int gap = 8;
    comp.setBounds (a.removeFromLeft ((int) ((float) a.getWidth() * 0.36f)));
    a.removeFromLeft (gap);
    eq.setBounds (a);
    fx.setBounds (b.removeFromLeft ((int) ((float) b.getWidth() * 0.39f)));
    b.removeFromLeft (gap);
    dly.setBounds (b.removeFromLeft ((int) ((float) b.getWidth() * 0.55f)));
    b.removeFromLeft (gap);
    rev.setBounds (b);
}
} // namespace yvui
