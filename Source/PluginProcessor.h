#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SaunaEngine.h"

class SaunaProcessor : public juce::AudioProcessor,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    SaunaProcessor();
    ~SaunaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    using juce::AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return sauna::NUM_PRESETS; }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return plungeParam; }

    void loadPreset (int index);
    sauna::Values userValues() const;
    float steamValue() const { return steam->load(); }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> meterIn { 0 }, meterOut { 0 }, meterGr { 1 };
    std::atomic<int> currentPreset { 0 };
    std::atomic<bool> presetEdited { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override;
    void pushValues();
    void run (juce::AudioBuffer<float>&, bool hostBypass);

    sauna::Engine engine;
    std::atomic<bool> dirty { true }, loading { false };
    std::vector<float> scratch;
    juce::AudioParameterBool* plungeParam = nullptr;
    std::atomic<float>* steam = nullptr;
    std::atomic<float>* plunge = nullptr;
    std::atomic<float>* knobRaw[sauna::NUM_KNOBS] {};
    std::atomic<float>* onRaw[sauna::NUM_SECTIONS] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaunaProcessor)
};
