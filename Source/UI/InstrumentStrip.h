#pragma once

#include <JuceHeader.h>

#include "../Parameters.h"

namespace basti::ui
{
    /** Ein Regler mit Beschriftung, per Attachment an einen APVTS-Parameter gebunden. */
    struct LabelledKnob final : public juce::Component
    {
        LabelledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                      const juce::String& labelText, juce::Colour colour);

        void resized() override;

        juce::Slider slider;
        juce::Label  label;
        juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    };

    //==========================================================================
    /** Kanalzug fuer ein Instrument: Synthese-Regler (z.B. Pitch/Decay/Tone),
        bei Toms die Tom/Conga-Umschaltung, sowie Level und Pan. */
    class InstrumentStrip final : public juce::Component
    {
    public:
        InstrumentStrip (juce::AudioProcessorValueTreeState& state, int instrument);

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        static juce::String labelForSuffix (const juce::String& suffix);

        int instrument;
        juce::Colour colour;

        juce::OwnedArray<LabelledKnob> knobs;

        std::unique_ptr<juce::ComboBox> modeBox;
        std::unique_ptr<juce::Label>    modeLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    };
}
