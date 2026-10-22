#include "InstrumentStrip.h"
#include "DrumLookAndFeel.h"

namespace basti::ui
{
    LabelledKnob::LabelledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                                const juce::String& labelText, juce::Colour colour)
        : attachment (state, paramID, slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 14);
        slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
        slider.setPopupDisplayEnabled (false, false, nullptr);
        slider.setDoubleClickReturnValue (true, (double) state.getParameter (paramID)->convertFrom0to1 (
                                                    state.getParameter (paramID)->getDefaultValue()));
        addAndMakeVisible (slider);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (DrumLookAndFeel::uiFont (11.0f, true));
        label.setColour (juce::Label::textColourId, colours::textDim);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }

    void LabelledKnob::resized()
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (14));
        slider.setBounds (r);
    }

    //==========================================================================
    InstrumentStrip::InstrumentStrip (juce::AudioProcessorValueTreeState& state, int instr)
        : instrument (instr), colour (colours::forInstrument (instr))
    {
        auto suffixes = getSynthParamSuffixes (instrument);
        suffixes.add (ids::level);
        suffixes.add (ids::pan);

        for (const auto& suffix : suffixes)
        {
            const auto id = paramId (instrument, suffix);

            if (suffix == ids::mode)
            {
                modeBox = std::make_unique<juce::ComboBox>();
                modeBox->addItemList ({ "Tom", "Conga" }, 1);
                addAndMakeVisible (*modeBox);
                modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, id, *modeBox);

                modeLabel = std::make_unique<juce::Label> (juce::String(), "MODE");
                modeLabel->setJustificationType (juce::Justification::centred);
                modeLabel->setFont (DrumLookAndFeel::uiFont (11.0f, true));
                modeLabel->setColour (juce::Label::textColourId, colours::textDim);
                addAndMakeVisible (*modeLabel);
                continue;
            }

            addAndMakeVisible (knobs.add (new LabelledKnob (state, id, labelForSuffix (suffix), colour)));
        }
    }

    juce::String InstrumentStrip::labelForSuffix (const juce::String& suffix)
    {
        if (suffix == ids::pitch)  return "PITCH";
        if (suffix == ids::tune)   return "TUNE";
        if (suffix == ids::decay)  return "DECAY";
        if (suffix == ids::tone)   return "TONE";
        if (suffix == ids::snappy) return "SNAPPY";
        if (suffix == ids::level)  return "LEVEL";
        if (suffix == ids::pan)    return "PAN";
        return suffix.toUpperCase();
    }

    void InstrumentStrip::paint (juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced (3.0f);

        g.setColour (colours::panel);
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (colours::outline);
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

        // Farbstreifen + Name
        auto header = bounds.removeFromTop (26.0f);
        g.setColour (colour);
        g.fillRoundedRectangle (header.removeFromTop (3.0f).reduced (10.0f, 0.0f), 1.5f);

        g.setColour (colours::text);
        g.setFont (DrumLookAndFeel::uiFont (14.0f, true));
        g.drawText (getInstrumentName (instrument).toUpperCase(), header, juce::Justification::centred);
    }

    void InstrumentStrip::resized()
    {
        auto area = getLocalBounds().reduced (8).withTrimmedTop (26);
        const int rowH  = juce::jmin (76, area.getHeight() / 3);
        const int halfW = area.getWidth() / 2;

        // Unterste Zeile immer: Level + Pan (die letzten beiden Knobs)
        auto bottom = area.removeFromBottom (rowH);
        const int synthKnobs = knobs.size() - 2;
        knobs[synthKnobs]->setBounds     (bottom.removeFromLeft (halfW).reduced (2));
        knobs[synthKnobs + 1]->setBounds (bottom.reduced (2));

        // Darueber: Synth-Regler paarweise, ein einzelner Regler wird zentriert
        for (int i = 0; i < synthKnobs; i += 2)
        {
            auto row = area.removeFromTop (rowH);

            if (i + 1 < synthKnobs)
            {
                knobs[i]->setBounds     (row.removeFromLeft (halfW).reduced (2));
                knobs[i + 1]->setBounds (row.reduced (2));
            }
            else
            {
                knobs[i]->setBounds (row.withSizeKeepingCentre (halfW, rowH).reduced (2));
            }
        }

        // Tom/Conga-Umschalter nimmt eine ganze Zeile ein
        if (modeBox != nullptr)
        {
            auto row = area.removeFromTop (rowH).reduced (6, 4);
            modeLabel->setBounds (row.removeFromTop (14));
            modeBox->setBounds (row.withSizeKeepingCentre (row.getWidth(), 26));
        }
    }
}
