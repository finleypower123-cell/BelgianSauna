#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace saunaui
{
namespace col
{
    const juce::Colour cedarDark { 0xff2a1710 }, cedar { 0xff4a2a1b }, cedarHi { 0xff6b3e26 }, bench { 0xff1d100b };
    const juce::Colour line { 0x33ffd9b8 }, steam { 0xfffbeee0 }, steam2 { 0xffe7cbb5 }, steam3 { 0xffb08d78 };
    const juce::Colour peach { 0xffffb38a }, rose { 0xfff2a2c0 }, haze { 0xffc9b6ff }, ember { 0xffff7a45 };
    const juce::Colour lcdBg { 0xff1a0f0a }, lcdInk { 0xffffcf8a };
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

class Knob : public juce::Component
{
public:
    Knob (SaunaProcessor&, const sauna::KnobDef&, std::function<void (const juce::String&)>);
    void resized() override;
    void setSteamed (double norm);
    const sauna::KnobDef& def;
    juce::Slider slider;
private:
    juce::Label read, joke, what;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class Section : public juce::Component
{
public:
    Section (SaunaProcessor&, int index, std::function<void (const juce::String&)>);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh (const sauna::Values& steamed);
    const sauna::SectionDef& def;
private:
    juce::ToggleButton power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAtt;
    juce::OwnedArray<Knob> knobs;
    bool lastOn = true;
};

class Meters : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void setLevels (float in, float out, float gr);
private:
    float dIn = 0, dOut = 0, dGr = 0;
};

class Lcd : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    juce::String preset, joke, info, status;
};

class Content : public juce::Component, private juce::Timer
{
public:
    explicit Content (SaunaProcessor&);
    ~Content() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int W = 1200, H = 900;
private:
    void timerCallback() override;
    void setInfo (const juce::String&);
    SaunaProcessor& proc;
    LookAndFeel lnf;
    juce::ComboBox presets;
    juce::TextButton prev { "<" }, next { ">" }, plungeBtn { "Cold plunge" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> plungeAtt;
    juce::Slider steamKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> steamAtt;
    juce::Label steamStage, steamSub;
    Lcd lcd;
    Meters meters;
    juce::OwnedArray<Section> sections;
    int infoTicks = 0;
};
} // namespace saunaui

class SaunaEditor : public juce::AudioProcessorEditor
{
public:
    explicit SaunaEditor (SaunaProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (saunaui::col::bench); }
private:
    saunaui::Content content;
    juce::ComponentBoundsConstrainer constrainer;
};
