#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "UI/DrumLookAndFeel.h"
#include "UI/LogoComponent.h"
#include "UI/StepGrid.h"
#include "UI/InstrumentStrip.h"

//==============================================================================
class BastisDrumAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BastisDrumAudioProcessorEditor (BastisDrumAudioProcessor&);
    ~BastisDrumAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    BastisDrumAudioProcessor& processorRef;
    basti::ui::DrumLookAndFeel lookAndFeel;

    basti::ui::LogoComponent logo;

    // Transport / Master
    juce::TextButton runButton   { "RUN" };
    juce::TextButton syncButton  { "HOST SYNC" };
    juce::TextButton clearButton { "CLEAR" };
    std::unique_ptr<ButtonAttachment> runAttachment, syncAttachment;

    std::unique_ptr<basti::ui::LabelledKnob> tempoKnob, swingKnob, accentKnob, masterKnob;

    juce::OwnedArray<basti::ui::InstrumentStrip> strips;
    basti::ui::StepGrid stepGrid;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BastisDrumAudioProcessorEditor)
};
