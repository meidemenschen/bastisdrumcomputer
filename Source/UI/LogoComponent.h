#pragma once

#include <JuceHeader.h>

namespace basti::ui
{
    /** Schriftzug "Basti´s Drum Machine" in Metall-Schreibschrift (oben links).
        Der Text wird als Pfad gerendert und mit einem gebuerstet-metallischen
        Verlauf, dunkler Kontur, Schlagschatten und Glanzkante gefuellt. */
    class LogoComponent final : public juce::Component
    {
    public:
        LogoComponent();

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        static juce::Font createScriptFont (float height);

        juce::String text;
        juce::Path textPath;
    };
}
