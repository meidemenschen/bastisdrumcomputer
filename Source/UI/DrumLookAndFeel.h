#pragma once

#include <JuceHeader.h>

namespace basti::ui
{
    /** Farbpalette: dunkles Graphit mit kuehlen Akzenten (bewusst eigenstaendig,
        keine Anlehnung an Gehaeuse-/Farbschemata bekannter Hardware). */
    namespace colours
    {
        inline const juce::Colour background   { 0xff121519 };
        inline const juce::Colour panel        { 0xff1b2027 };
        inline const juce::Colour panelLight   { 0xff252c35 };
        inline const juce::Colour outline      { 0xff2f3843 };
        inline const juce::Colour text         { 0xffd9dee6 };
        inline const juce::Colour textDim      { 0xff8a95a3 };
        inline const juce::Colour accent       { 0xff38d1c4 };   // Tuerkis
        inline const juce::Colour accentRow    { 0xffb48cff };   // Violett fuer Accent-Zeile
        inline const juce::Colour playhead     { 0x22ffffff };

        /** Je Instrument eine eigene, kuehle Farbe (Tuerkis -> Blau -> Violett). */
        juce::Colour forInstrument (int instrument);
    }

    //==========================================================================
    class DrumLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        DrumLookAndFeel();

        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPosProportional, float rotaryStartAngle,
                               float rotaryEndAngle, juce::Slider&) override;

        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                                   bool isMouseOverButton, bool isButtonDown) override;

        void drawButtonText (juce::Graphics&, juce::TextButton&, bool isMouseOverButton, bool isButtonDown) override;

        juce::Label* createSliderTextBox (juce::Slider&) override;

        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

        static juce::Font uiFont (float height, bool bold = false);
    };
}
