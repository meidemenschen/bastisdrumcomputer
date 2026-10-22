#pragma once

#include <JuceHeader.h>

#include "../PluginProcessor.h"

namespace basti::ui
{
    /** 16-Step-Raster: eine Zeile pro Instrument plus Accent-Zeile.
        Klick schaltet einen Step, Ziehen "malt" mehrere Steps,
        Klick auf den Instrumentnamen spielt das Instrument an. */
    class StepGrid final : public juce::Component,
                           private juce::Timer
    {
    public:
        explicit StepGrid (BastisDrumAudioProcessor& processor);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;

    private:
        void timerCallback() override;

        juce::Rectangle<float> getCellBounds (int row, int step) const;
        juce::Rectangle<int>   getNameBounds (int row) const;
        bool getCellAt (juce::Point<int> pos, int& row, int& step) const;

        static constexpr int nameWidth   = 110;
        static constexpr int headerHeight = 18;

        BastisDrumAudioProcessor& processor;
        PatternData& pattern;

        bool dragValue = true;
        int  lastDragRow = -1, lastDragStep = -1;

        int lastStep = -2;
        std::array<uint32_t, numPatternRows> lastBits {};
        std::array<int, numInstruments> lastTriggerCounts {};
        std::array<float, numInstruments> ledLevels {};
    };
}
