#pragma once

#include <JuceHeader.h>

#include "Parameters.h"
#include "DSP/DrumEngine.h"
#include "DSP/Sequencer.h"

//==============================================================================
/** Bastis Drumcomputer – Analog-Style Drum Synthesizer.
    Reine DSP-Synthese, MIDI (GM-Drum-Map) + interner 16-Step-Sequenzer. */
class BastisDrumAudioProcessor final : public juce::AudioProcessor
{
public:
    BastisDrumAudioProcessor();
    ~BastisDrumAudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }

    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram (int) override                     {}
    const juce::String getProgramName (int) override          { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState& getState() noexcept { return apvts; }
    basti::PatternData&   getPattern() noexcept             { return pattern; }
    basti::StepSequencer& getSequencer() noexcept           { return sequencer; }
    basti::DrumEngine&    getEngine() noexcept              { return engine; }

    /** Instrument direkt anspielen (z.B. Klick auf den Namen in der GUI). */
    void auditionInstrument (int instrument) noexcept;

private:
    void handleMidi (const juce::MidiBuffer& midi);

    juce::AudioProcessorValueTreeState apvts;
    basti::PatternData   pattern;
    basti::DrumEngine    engine;
    basti::StepSequencer sequencer;

    std::atomic<float>* accentParam = nullptr;

    // GUI -> Audio: angefragte Vorhoer-Trigger als Bitmaske
    std::atomic<uint32_t> pendingAuditions { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BastisDrumAudioProcessor)
};
