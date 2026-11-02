#include "PluginEditor.h"

namespace saunaui
{
static juce::Font font (float h, int style = juce::Font::bold, float hs = 0.9f)
{
    return juce::Font (juce::FontOptions (h, style).withHorizontalScale (hs));
}
static juce::Font mono (float h) { return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain)); }
static juce::Point<float> polar (juce::Point<float> c, float r, float a) { return { c.x + r * std::sin (a), c.y - r * std::cos (a) }; }
static void arc (juce::Graphics& g, juce::Point<float> c, float r, float a0, float a1, float w)
{
    if (a1 - a0 < 0.01f) return;
    juce::Path p; p.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
    g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// Cedar planks with soft grain.
static void planks (juce::Graphics& g, juce::Rectangle<float> r, float plankH, juce::Colour base, uint32_t seed)
{
    juce::Random rnd ((juce::int64) seed);
    for (float y = r.getY(); y < r.getBottom(); y += plankH)
    {
        const auto c = base.brighter (rnd.nextFloat() * 0.18f - 0.06f);
        const auto band = juce::Rectangle<float> (r.getX(), y, r.getWidth(), juce::jmin (plankH, r.getBottom() - y));
        g.setGradientFill (juce::ColourGradient (c.brighter (0.05f), 0, band.getY(), c.darker (0.15f), 0, band.getBottom(), false));
        g.fillRect (band);
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        for (int k = 0; k < 4; ++k)
        {
            const float gy = band.getY() + rnd.nextFloat() * band.getHeight();
            juce::Path grain; grain.startNewSubPath (r.getX(), gy);
            for (float x = r.getX(); x < r.getRight(); x += 60) grain.lineTo (x + 60, gy + std::sin ((x + seed) * 0.013f) * 2.5f);
            g.strokePath (grain, juce::PathStrokeType (1.0f));
        }
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawHorizontalLine ((int) band.getBottom() - 1, r.getX(), r.getRight());
    }
}

// ============================================================================ LookAndFeel
LookAndFeel::LookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, col::bench);
    setColour (juce::ComboBox::textColourId, col::steam);
    setColour (juce::ComboBox::outlineColourId, col::line);
    setColour (juce::ComboBox::arrowColourId, col::peach);
    setColour (juce::PopupMenu::backgroundColourId, col::cedarDark);
    setColour (juce::PopupMenu::textColourId, col::steam);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::peach);
    setColour (juce::PopupMenu::highlightedTextColourId, col::bench);
    setColour (juce::TextButton::textColourOffId, col::steam);
    setColour (juce::TextButton::textColourOnId, col::bench);
    setColour (juce::Label::textColourId, col::steam);
}

juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int) { return font (13.0f); }
juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&) { return font (15.0f); }
juce::Font LookAndFeel::getPopupMenuFont() { return font (16.0f); }

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    if (s.getProperties().contains ("steam"))
    {
        // Sauna-stove dial: a ring of glowing coals, then a stone knob.
        const int n = 31, lit = juce::roundToInt (pos * (n - 1));
        for (int i = 0; i < n; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / (float) (n - 1);
            const auto p = polar (c, r - 5.0f, a);
            const bool on = i <= lit && pos > 0.001f;
            const float t = (float) i / (float) (n - 1);
            const auto lc = ! on ? juce::Colour (0xff3a2219) : t < 0.5f ? col::ember.interpolatedWith (col::peach, t * 2) : col::peach.interpolatedWith (col::steam, (t - 0.5f) * 2);
            if (on) { g.setColour (lc.withAlpha (0.3f)); g.fillEllipse (p.x - 6, p.y - 6, 12, 12); }
            g.setColour (lc); g.fillEllipse (p.x - 3.5f, p.y - 3.5f, 7, 7);
        }
        const float cr = r * 0.64f;
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (c.x - cr, c.y - cr + 6, cr * 2, cr * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8d7a70), c.x - cr * 0.4f, c.y - cr * 0.5f, juce::Colour (0xff2c2422), c.x + cr * 0.6f, c.y + cr, true));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawEllipse (c.x - cr + 1, c.y - cr + 1, cr * 2 - 2, cr * 2 - 2, 1.5f);
        g.setColour (col::steam); g.drawLine ({ polar (c, cr * 0.25f, ang), polar (c, cr * 0.86f, ang) }, 4.0f);
        return;
    }

    g.setColour (juce::Colour (0xff3a2219)); arc (g, c, r - 3.0f, a0, a1, 3.5f);
    g.setColour (col::peach.withAlpha (0.25f)); arc (g, c, r - 3.0f, a0, ang, 7.0f);
    g.setColour (col::peach); arc (g, c, r - 3.0f, a0, ang, 3.5f);
    const auto eff = s.getProperties()["steamed"];
    if (! eff.isVoid())
    {
        const float e = (float) (double) eff;
        if (std::abs (e - pos) > 0.004f) { g.setColour (col::rose); arc (g, c, r + 0.5f, a0, a0 + e * (a1 - a0), 1.6f); }
    }
    const float cr = r * 0.64f;
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (c.x - cr, c.y - cr + 3, cr * 2, cr * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffa0683f), c.x - cr * 0.3f, c.y - cr * 0.5f, juce::Colour (0xff2e180e), c.x + cr * 0.6f, c.y + cr, true));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    g.setColour (juce::Colours::white.withAlpha (0.15f)); g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 1.0f);
    g.setColour (col::steam); g.drawLine ({ polar (c, cr * 0.3f, ang), polar (c, cr * 0.88f, ang) }, 2.0f);
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hi, bool)
{
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (col::bench); g.fillRoundedRectangle (r, 7.0f);
    g.setColour (hi ? col::peach.withAlpha (0.5f) : col::line); g.drawRoundedRectangle (r, 7.0f, 1.0f);
    const auto c = r.getCentre();
    if (on) { g.setColour (col::ember.withAlpha (0.35f)); g.fillEllipse (c.x - 8, c.y - 8, 16, 16); }
    g.setColour (on ? col::ember : juce::Colour (0xff3a2219)); g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9, 9);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool plunge = b.getProperties().contains ("plunge"), on = b.getToggleState();
    const auto top = plunge && on ? juce::Colour (0xff2f6f8f) : col::cedarHi;
    const auto bot = plunge && on ? juce::Colour (0xff1b4357) : col::cedarDark;
    g.setGradientFill (juce::ColourGradient (top, 0, r.getY(), bot, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r.translated (0, down ? 1.0f : 0.0f), 7.0f);
    g.setColour (hi ? col::peach.withAlpha (0.5f) : col::line); g.drawRoundedRectangle (r, 7.0f, 1.0f);
    if (plunge)
    {
        const auto led = juce::Rectangle<float> (r.getX() + 10, r.getCentreY() - 4.5f, 9, 9);
        g.setColour (on ? juce::Colour (0xff9fe3ff) : juce::Colour (0xff3a2219)); g.fillEllipse (led);
    }
}

// ============================================================================ Knob
Knob::Knob (SaunaProcessor& p, const sauna::KnobDef& k, std::function<void (const juce::String&)> info) : def (k)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (230);
    slider.setDoubleClickReturnValue (true, k.def);
    slider.setTitle (juce::String (k.joke) + ": " + k.what);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, k.id, slider);
    read.setFont (mono (16.0f)); read.setColour (juce::Label::textColourId, col::lcdInk); read.setJustificationType (juce::Justification::centred);
    joke.setFont (font (12.5f)); joke.setJustificationType (juce::Justification::centredTop); joke.setMinimumHorizontalScale (0.72f);
    joke.setText (juce::String (k.joke).toUpperCase(), juce::dontSendNotification);
    what.setFont (font (10.5f, juce::Font::bold, 0.92f)); what.setColour (juce::Label::textColourId, col::steam3);
    what.setJustificationType (juce::Justification::centredTop); what.setMinimumHorizontalScale (0.7f);
    what.setText (juce::String (k.what).toUpperCase(), juce::dontSendNotification);
    for (auto* l : { &read, &joke, &what }) { l->setInterceptsMouseClicks (false, false); l->setBorderSize ({}); addAndMakeVisible (*l); }
    auto update = [this, info] (bool tell)
    {
        read.setText (sauna::formatValue (def, (float) slider.getValue()), juce::dontSendNotification);
        if (tell) info (juce::String (def.joke).toUpperCase() + "  ->  " + def.what + " " + read.getText());
    };
    slider.onValueChange = [this, update] { update (slider.isMouseButtonDown() || slider.hasKeyboardFocus (false)); };
    slider.onDragStart = [update] { update (true); };
    update (false);
}

void Knob::resized()
{
    const int w = getWidth(), dial = 54, top = juce::jmax (0, (getHeight() - 122) / 2);
    slider.setBounds ((w - dial) / 2, top, dial, dial);
    read.setBounds (0, top + 56, w, 16);
    joke.setBounds (2, top + 74, w - 4, 34);
    what.setBounds (2, top + 109, w - 4, 13);
}

void Knob::setSteamed (double norm)
{
    const auto cur = slider.getProperties()["steamed"];
    if (cur.isVoid() || std::abs ((double) cur - norm) > 0.001) { slider.getProperties().set ("steamed", norm); slider.repaint(); }
}

// ============================================================================ Section
Section::Section (SaunaProcessor& p, int index, std::function<void (const juce::String&)> info) : def (sauna::SECTIONS[index])
{
    power.setTitle (juce::String (def.name) + " on or off");
    addAndMakeVisible (power);
    powerAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, def.onId, power);
    power.onClick = [this, info] { info (juce::String (def.name).toUpperCase() + (power.getToggleState() ? " ON" : " OFF")); repaint(); };
    for (int i = 0; i < def.numKnobs; ++i) addAndMakeVisible (knobs.add (new Knob (p, sauna::KNOBS[def.firstKnob + i], info)));
}

void Section::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (r, 12.0f); g.reduceClipRegion (clip);
        planks (g, r, 34.0f, col::cedarDark, (uint32_t) (def.unit * 977 + 13));
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.4f), 0, r.getY(), juce::Colours::transparentBlack, 0, r.getY() + 18, false));
        g.fillRect (r);
    }
    g.setColour (col::line); g.drawRoundedRectangle (r, 12.0f, 1.0f);
    g.setColour (col::steam); g.setFont (font (14.5f, juce::Font::bold, 1.0f));
    g.drawText (juce::String (def.name).toUpperCase(), 48, 10, getWidth() - 58, 18, juce::Justification::centredLeft, true);
    g.setColour (col::steam3); g.setFont (font (13.0f, juce::Font::plain, 0.95f));
    g.drawText (def.what, 48, 28, getWidth() - 58, 16, juce::Justification::centredLeft, true);
    if (! power.getToggleState()) { g.setColour (col::bench.withAlpha (0.6f)); g.fillRoundedRectangle (r.withTrimmedTop (48).reduced (4), 8.0f); }
}

void Section::resized()
{
    power.setBounds (12, 12, 26, 26);
    const int n = knobs.size(), top = 52;
    const float cw = (float) (getWidth() - 16) / (float) juce::jmax (1, n);
    for (int i = 0; i < n; ++i) knobs[i]->setBounds (8 + juce::roundToInt (i * cw), top, juce::roundToInt (cw), getHeight() - top - 4);
}

void Section::refresh (const sauna::Values& steamed)
{
    for (auto* k : knobs) k->setSteamed (steamed.v[sauna::knobIndex (k->def.id)]);
    if (power.getToggleState() != lastOn) { lastOn = power.getToggleState(); repaint(); }
}

// ============================================================================ Meters / LCD
void Meters::setLevels (float in, float out, float gr)
{
    auto lv = [] (float x) { return juce::jlimit (0.0f, 1.0f, (20.0f * std::log10 (x + 1e-9f) + 60.0f) / 60.0f); };
    dIn = juce::jmax (lv (in), dIn - 0.02f); dOut = juce::jmax (lv (out), dOut - 0.02f);
    dGr = juce::jmax (juce::jlimit (0.0f, 1.0f, -20.0f * std::log10 (juce::jmax (gr, 1e-6f)) / 12.0f), dGr - 0.03f);
    repaint();
}

void Meters::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (col::bench); g.fillRoundedRectangle (r, 11.0f);
    g.setColour (col::line); g.drawRoundedRectangle (r, 11.0f, 1.0f);
    const float top = 12, bottom = r.getBottom() - 24, h = bottom - top;
    g.setFont (mono (12.0f)); g.setColour (col::steam3);
    for (int i = 0; i <= 5; ++i) g.drawText (juce::String (-12 * i), 4, juce::roundToInt (top + h * i / 5.0f - 6), 28, 12, juce::Justification::centredRight);
    const char* labels[3] = { "IN", "OUT", "LIM" };
    const float vals[3] = { dIn, dOut, dGr };
    for (int i = 0; i < 3; ++i)
    {
        const juce::Rectangle<float> bar (38.0f + i * 20.0f, top, 13.0f, h);
        g.setColour (juce::Colour (0xff120a07)); g.fillRoundedRectangle (bar, 3.0f);
        if (i < 2)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (bar.withTop (bar.getBottom() - h * vals[i]).toNearestInt());
            g.setGradientFill (juce::ColourGradient (col::ember, 0, bar.getBottom(), col::steam, 0, bar.getY(), false));
            g.fillRect (bar);
        }
        else { g.setColour (juce::Colour (0xff9fe3ff)); g.fillRect (bar.withHeight (h * vals[i])); }
        g.setColour (col::bench.withAlpha (0.85f));
        for (float yy = bar.getY(); yy < bar.getBottom(); yy += 5.0f) g.fillRect (bar.getX(), yy, bar.getWidth(), 1.0f);
        g.setColour (col::steam3); g.setFont (font (11.0f));
        g.drawText (labels[i], juce::roundToInt (bar.getCentreX() - 15), juce::roundToInt (bottom + 5), 30, 14, juce::Justification::centred);
    }
}

void Lcd::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0a0503)); g.fillRoundedRectangle (r, 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff24140d), 0, 0, col::lcdBg, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r.reduced (2.0f), 9.0f);
    const int x = 16, w = getWidth() - 32;
    g.setColour (col::lcdInk); g.setFont (mono (23.0f));
    g.drawText (preset, x, 12, w, 28, juce::Justification::centredLeft, true);
    g.setColour (col::lcdInk.withAlpha (0.72f)); g.setFont (mono (16.0f));
    g.drawText (joke, x, 42, w, 22, juce::Justification::centredLeft, true);
    g.setColour (col::lcdInk.withAlpha (0.2f)); g.drawHorizontalLine (70, (float) x, (float) (x + w));
    g.setColour (col::lcdInk.withAlpha (0.75f)); g.setFont (mono (15.0f));
    g.drawText (status, x, 78, w, 20, juce::Justification::centredLeft, true);
    g.setColour (col::steam); g.setFont (mono (17.0f));
    g.drawText (info, x, 102, w, 24, juce::Justification::centredLeft, true);
    // a little steam haze across the glass
    for (int i = 0; i < 3; ++i)
    {
        g.setGradientFill (juce::ColourGradient (col::steam.withAlpha (0.05f), r.getWidth() * (0.2f + 0.3f * i), r.getHeight() * 0.3f,
                                                 juce::Colours::transparentWhite, r.getWidth() * (0.2f + 0.3f * i) + 120, r.getHeight() * 0.3f + 60, true));
        g.fillRoundedRectangle (r.reduced (2.0f), 9.0f);
    }
}

// ============================================================================ Content
Content::Content (SaunaProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);
    for (int i = 0; i < sauna::NUM_PRESETS; ++i)
        presets.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + juce::String (sauna::PRESETS[i].name).toUpperCase(), i + 1);
    presets.onChange = [this]
    {
        const int id = presets.getSelectedId();
        if (id > 0 && id - 1 != proc.currentPreset.load()) proc.loadPreset (id - 1);
        setInfo ("PRESET " + juce::String (id).paddedLeft ('0', 2) + " LOADED");
    };
    presets.setTitle ("Preset");
    addAndMakeVisible (presets);
    prev.onClick = [this] { proc.loadPreset ((proc.currentPreset.load() + sauna::NUM_PRESETS - 1) % sauna::NUM_PRESETS); };
    next.onClick = [this] { proc.loadPreset ((proc.currentPreset.load() + 1) % sauna::NUM_PRESETS); };
    prev.setTitle ("Previous preset"); next.setTitle ("Next preset");
    addAndMakeVisible (prev); addAndMakeVisible (next);

    plungeBtn.getProperties().set ("plunge", true);
    plungeBtn.setClickingTogglesState (true);
    plungeBtn.setColour (juce::TextButton::textColourOnId, col::steam);
    addAndMakeVisible (plungeBtn);
    plungeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "plunge", plungeBtn);
    plungeBtn.onClick = [this] { setInfo (plungeBtn.getToggleState() ? "COLD PLUNGE: THE RAW VOCAL. BRACING." : "BACK IN THE STEAM."); };

    steamKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    steamKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    steamKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    steamKnob.setMouseDragSensitivity (260);
    steamKnob.setDoubleClickReturnValue (true, 0.5);
    steamKnob.getProperties().set ("steam", true);
    steamKnob.setTitle ("STEAM macro");
    addAndMakeVisible (steamKnob);
    steamAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "steam", steamKnob);
    steamKnob.onDragStart = [this] { setInfo (juce::String ("STEAM  ->  ") + sauna::steamStage ((float) steamKnob.getValue())); };
    steamStage.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold | juce::Font::italic).withHorizontalScale (0.88f)));
    steamStage.setColour (juce::Label::textColourId, col::peach);
    steamSub.setFont (font (12.5f, juce::Font::plain)); steamSub.setColour (juce::Label::textColourId, col::steam3);
    for (auto* l : { &steamStage, &steamSub }) { l->setJustificationType (juce::Justification::centredLeft); l->setBorderSize ({}); l->setMinimumHorizontalScale (0.7f); addAndMakeVisible (*l); }

    addAndMakeVisible (lcd);
    addAndMakeVisible (meters);
    for (int i = 0; i < sauna::NUM_SECTIONS; ++i)
        addAndMakeVisible (sections.add (new Section (p, i, [this] (const juce::String& s) { setInfo (s); })));
    lcd.info = "Drag a knob. The pink arcs show where STEAM has pushed it.";
    setSize (W, H);
    resized();
    startTimerHz (30);
}

Content::~Content() { stopTimer(); setLookAndFeel (nullptr); }

void Content::setInfo (const juce::String& s) { lcd.info = s; infoTicks = 140; lcd.repaint(); }

void Content::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    planks (g, r, 46.0f, col::cedar, 7);
    // warm glow from the stove, haze from the top
    g.setGradientFill (juce::ColourGradient (col::ember.withAlpha (0.16f), r.getRight() - 120, 170, juce::Colours::transparentBlack, r.getRight() - 520, 520, true));
    g.fillRect (r);
    g.setGradientFill (juce::ColourGradient (col::steam.withAlpha (0.10f), r.getCentreX(), 0, juce::Colours::transparentWhite, r.getCentreX(), 140, false));
    g.fillRect (r);
    g.setColour (col::steam3); g.setFont (font (12.0f, juce::Font::bold, 1.0f));
    g.drawText ("EGO LABS  .  BELGIAN DIVISION  .  BUILT ON AIRWINDOWS", 28, 14, 600, 16, juce::Justification::centredLeft);
    g.setGradientFill (juce::ColourGradient (col::steam, 0, 30, col::peach, 0, 82, false));
    g.setFont (juce::Font (juce::FontOptions (50.0f, juce::Font::bold).withHorizontalScale (1.0f)));
    g.drawText ("BELGIAN SAUNA", 26, 28, 620, 56, juce::Justification::centredLeft);
    g.setColour (col::rose); g.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::italic).withHorizontalScale (0.95f)));
    g.drawText ("dream-pop vocal preamp  .  shoegaze edition  .  90 degrees and rising", 30, 80, 620, 20, juce::Justification::centredLeft);
    g.setColour (col::line); g.drawHorizontalLine (106, 16.0f, (float) W - 16);
    auto er = juce::Rectangle<float> (968, 116, 216, 136);
    g.setColour (col::bench); g.fillRoundedRectangle (er, 11.0f);
    g.setColour (col::line); g.drawRoundedRectangle (er, 11.0f, 1.0f);
    g.setColour (col::steam); g.setFont (font (13.0f, juce::Font::bold, 1.0f));
    g.drawText ("S T E A M", 1094, 126, 90, 16, juce::Justification::centredLeft);
}

void Content::resized()
{
    prev.setBounds (640, 28, 38, 36);
    presets.setBounds (684, 28, 456, 36);
    next.setBounds (1146, 28, 38, 36);
    plungeBtn.setBounds (1004, 70, 180, 28);
    lcd.setBounds (16, 116, 828, 136);
    meters.setBounds (856, 116, 100, 136);
    steamKnob.setBounds (972, 120, 116, 128);
    steamStage.setBounds (1092, 152, 90, 26);
    steamSub.setBounds (1092, 180, 90, 50);
    const juce::Rectangle<int> b[8] = {
        { 16, 264, 240, 196 },  { 268, 264, 420, 196 }, { 700, 264, 240, 196 }, { 16, 472, 600, 196 },
        { 628, 472, 556, 196 }, { 16, 680, 600, 204 },  { 628, 680, 556, 204 }, { 952, 264, 232, 196 } };
    for (int i = 0; i < sections.size() && i < 8; ++i) sections[i]->setBounds (b[i]);
}

void Content::timerCallback()
{
    meters.setLevels (proc.meterIn.exchange (0.0f), proc.meterOut.exchange (0.0f), proc.meterGr.exchange (1.0f));
    const float st = proc.steamValue();
    const auto steamed = sauna::applySteam (proc.userValues(), st);
    for (auto* s : sections) s->refresh (steamed);
    steamStage.setText (sauna::steamStage (st), juce::dontSendNotification);
    int steamCount = 1;   // the output trim
    for (const auto& k : sauna::KNOBS) if (k.steamTarget >= 0) ++steamCount;
    steamSub.setText (juce::String (juce::roundToInt (st * 100)) + "%  .  steams " + juce::String (steamCount) + " controls at once", juce::dontSendNotification);

    const int pi = proc.currentPreset.load();
    const bool edited = proc.presetEdited.load();
    if (presets.getSelectedId() != pi + 1) presets.setSelectedId (pi + 1, juce::dontSendNotification);
    const juce::String name = "P" + juce::String (pi + 1).paddedLeft ('0', 2) + " " + juce::String (sauna::PRESETS[pi].name).toUpperCase() + (edited ? " *" : "");
    const juce::String joke = juce::String (sauna::PRESETS[pi].joke) + (edited ? " (Adjusted. Like the thermostat.)" : "");
    const juce::String status = "ZERO LATENCY  .  LEVEL-MATCHED  .  7 AIRWINDOWS EFFECTS IN SERIES";
    if (name != lcd.preset || joke != lcd.joke || status != lcd.status) { lcd.preset = name; lcd.joke = joke; lcd.status = status; lcd.repaint(); }
    if (infoTicks > 0 && --infoTicks == 0)
    {
        lcd.info = plungeBtn.getToggleState() ? "Cold plunge: the raw vocal. Bracing." : "Drag a knob. The pink arcs show where STEAM has pushed it.";
        lcd.repaint();
    }
}
} // namespace saunaui

SaunaEditor::SaunaEditor (SaunaProcessor& p) : AudioProcessorEditor (p), content (p)
{
    addAndMakeVisible (content);
    constrainer.setFixedAspectRatio ((double) saunaui::Content::W / saunaui::Content::H);
    constrainer.setSizeLimits (600, 450, 1800, 1350);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (1020, 765);
}

void SaunaEditor::resized()
{
    const float s = (float) getWidth() / (float) saunaui::Content::W;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, saunaui::Content::W, saunaui::Content::H);
}
