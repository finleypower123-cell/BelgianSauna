#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SaunaProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using PID = juce::ParameterID;
    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "steam", 1 }, "STEAM (macro)", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return juce::String (sauna::steamStage (v)) + " " + juce::String (juce::roundToInt (v * 100)) + "%"; })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.retainCharacters ("0123456789.").getFloatValue() / 100.0f; })));
    layout.add (std::make_unique<juce::AudioParameterBool> (PID { "plunge", 1 }, "Cold plunge (bypass)", false));
    for (int s = 0; s < sauna::NUM_SECTIONS; ++s)
    {
        const auto& sec = sauna::SECTIONS[s];
        layout.add (std::make_unique<juce::AudioParameterBool> (PID { sec.onId, 1 }, juce::String (sec.name) + " on", true));
        for (int i = 0; i < sec.numKnobs; ++i)
        {
            const sauna::KnobDef* k = &sauna::KNOBS[sec.firstKnob + i];
            layout.add (std::make_unique<juce::AudioParameterFloat> (PID { k->id, 1 }, juce::String (k->what) + " (" + k->joke + ")",
                juce::NormalisableRange<float> (0.0f, 1.0f), k->def,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([k] (float v, int) { return juce::String (sauna::formatValue (*k, v)); })));
        }
    }
    return layout;
}

SaunaProcessor::SaunaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BelgianSauna", createLayout())
{
    plungeParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("plunge"));
    steam = apvts.getRawParameterValue ("steam");
    plunge = apvts.getRawParameterValue ("plunge");
    for (int i = 0; i < sauna::NUM_KNOBS; ++i) knobRaw[i] = apvts.getRawParameterValue (sauna::KNOBS[i].id);
    for (int s = 0; s < sauna::NUM_SECTIONS; ++s) onRaw[s] = apvts.getRawParameterValue (sauna::SECTIONS[s].onId);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) apvts.addParameterListener (rp->getParameterID(), this);
    loadPreset (0);
    presetEdited = false;
}

SaunaProcessor::~SaunaProcessor()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) apvts.removeParameterListener (rp->getParameterID(), this);
}

bool SaunaProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out == juce::AudioChannelSet::stereo()) return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    if (out == juce::AudioChannelSet::mono()) return in == juce::AudioChannelSet::mono();
    return false;
}

sauna::Values SaunaProcessor::userValues() const
{
    sauna::Values v {};
    for (int i = 0; i < sauna::NUM_KNOBS; ++i) v.v[i] = knobRaw[i]->load();
    for (int s = 0; s < sauna::NUM_SECTIONS; ++s) v.on[s] = onRaw[s]->load() > 0.5f;
    return v;
}

void SaunaProcessor::pushValues()
{
    engine.setValues (sauna::applySteam (userValues(), steam->load()));
}

void SaunaProcessor::parameterChanged (const juce::String&, float)
{
    dirty = true;
    if (! loading.load()) presetEdited = true;
}

void SaunaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, juce::jmax (512, samplesPerBlock));
    pushValues();
    dirty = false;
    scratch.assign ((size_t) juce::jmax (512, samplesPerBlock), 0.0f);
    setLatencySamples (0);
}

void SaunaProcessor::run (juce::AudioBuffer<float>& buffer, bool hostBypass)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples(), ins = getTotalNumInputChannels(), outs = getTotalNumOutputChannels();
    if (n <= 0 || outs <= 0) return;
    if (dirty.exchange (false)) pushValues();
    engine.setBypassed (hostBypass || plunge->load() > 0.5f);

    float* L = buffer.getWritePointer (0);
    const float* inR = ins > 1 ? buffer.getReadPointer (1) : buffer.getReadPointer (0);
    if (outs > 1) engine.process (L, inR, L, buffer.getWritePointer (1), n);
    else
        for (int s = 0; s < n; s += (int) scratch.size())
        {
            const int c = juce::jmin ((int) scratch.size(), n - s);
            engine.process (L + s, inR + s, L + s, scratch.data(), c);
            for (int i = 0; i < c; ++i) L[s + i] = 0.5f * (L[s + i] + scratch[(size_t) i]);
        }
    for (int ch = 2; ch < outs; ++ch) buffer.clear (ch, 0, n);
    const auto m = engine.takeMeters();
    meterIn = juce::jmax (meterIn.load(), m.in);
    meterOut = juce::jmax (meterOut.load(), m.out);
    meterGr = juce::jmin (meterGr.load(), m.gr);
}

void SaunaProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) { run (b, false); }
void SaunaProcessor::processBlockBypassed (juce::AudioBuffer<float>& b, juce::MidiBuffer&) { run (b, true); }

void SaunaProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, sauna::NUM_PRESETS - 1, index);
    const auto v = sauna::presetValues (index);
    loading = true;
    auto set = [this] (const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
    };
    for (int i = 0; i < sauna::NUM_KNOBS; ++i) set (sauna::KNOBS[i].id, v.v[i]);
    for (int s = 0; s < sauna::NUM_SECTIONS; ++s) set (sauna::SECTIONS[s].onId, 1.0f);
    set ("steam", sauna::PRESETS[index].steam);
    loading = false;
    currentPreset = index;
    presetEdited = false;
    dirty = true;
}

const juce::String SaunaProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, sauna::NUM_PRESETS) ? juce::String (sauna::PRESETS[index].name) : juce::String();
}

void SaunaProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    state.setProperty ("edited", presetEdited.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void SaunaProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            loading = true;
            auto tree = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (tree);
            for (auto* p : getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                {
                    const auto child = tree.getChildWithProperty ("id", rp->getParameterID());
                    if (child.isValid() && child.hasProperty ("value"))
                        rp->setValueNotifyingHost (rp->convertTo0to1 ((float) child.getProperty ("value")));
                }
            loading = false;
            currentPreset = juce::jlimit (0, sauna::NUM_PRESETS - 1, (int) tree.getProperty ("preset", 0));
            presetEdited = (bool) tree.getProperty ("edited", false);
            dirty = true;
        }
}

juce::AudioProcessorEditor* SaunaProcessor::createEditor() { return new SaunaEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SaunaProcessor(); }
