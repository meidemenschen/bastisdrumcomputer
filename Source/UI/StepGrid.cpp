#include "StepGrid.h"
#include "DrumLookAndFeel.h"

namespace basti::ui
{
    StepGrid::StepGrid (BastisDrumAudioProcessor& p)
        : processor (p), pattern (p.getPattern())
    {
        startTimerHz (30);
    }

    //==========================================================================
    juce::Rectangle<float> StepGrid::getCellBounds (int row, int step) const
    {
        const auto gridW = (float) (getWidth() - nameWidth);
        const auto rowH  = (float) (getHeight() - headerHeight) / (float) numPatternRows;
        const auto cellW = (gridW - 12.0f) / (float) numSteps;     // 3 Luecken a 4 px zwischen den Vierergruppen
        const auto groupGap = 4.0f * (float) (step / 4);

        return { (float) nameWidth + cellW * (float) step + groupGap,
                 (float) headerHeight + rowH * (float) row,
                 cellW - 3.0f, rowH - 4.0f };
    }

    juce::Rectangle<int> StepGrid::getNameBounds (int row) const
    {
        const auto rowH = (float) (getHeight() - headerHeight) / (float) numPatternRows;
        return { 0, headerHeight + (int) (rowH * (float) row), nameWidth - 10, (int) rowH - 4 };
    }

    bool StepGrid::getCellAt (juce::Point<int> pos, int& row, int& step) const
    {
        for (int r = 0; r < numPatternRows; ++r)
            for (int s = 0; s < numSteps; ++s)
                if (getCellBounds (r, s).expanded (1.5f).contains (pos.toFloat()))
                {
                    row = r;
                    step = s;
                    return true;
                }

        return false;
    }

    //==========================================================================
    void StepGrid::paint (juce::Graphics& g)
    {
        const auto current = processor.getSequencer().getCurrentStep();

        // Step-Nummern
        g.setFont (DrumLookAndFeel::uiFont (11.0f));
        for (int s = 0; s < numSteps; ++s)
        {
            const auto cell = getCellBounds (0, s);
            g.setColour (s == current ? colours::text : colours::textDim);
            g.drawText (juce::String (s + 1), cell.withY (0.0f).withHeight ((float) headerHeight - 2.0f),
                        juce::Justification::centred);
        }

        for (int r = 0; r < numPatternRows; ++r)
        {
            const bool isAccent = r == accentRow;
            const auto rowColour = colours::forInstrument (r);

            // Name + Aktivitaets-LED
            const auto nameArea = getNameBounds (r);
            if (! isAccent)
            {
                const auto led = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ (float) nameArea.getX() + 8.0f,
                                                                                   (float) nameArea.getCentreY() });
                g.setColour (colours::outline);
                g.fillEllipse (led);
                g.setColour (rowColour.withAlpha (ledLevels[(size_t) r]));
                g.fillEllipse (led);
            }

            g.setColour (isAccent ? colours::accentRow : colours::text);
            g.setFont (DrumLookAndFeel::uiFont (13.0f, isAccent));
            g.drawText (isAccent ? juce::String ("ACCENT") : getInstrumentName (r),
                        nameArea.withTrimmedLeft (20), juce::Justification::centredLeft);

            // Steps
            for (int s = 0; s < numSteps; ++s)
            {
                const auto cell = getCellBounds (r, s);
                const bool on = pattern.get (r, s);
                const bool downbeat = (s % 4) == 0;

                g.setColour (on ? rowColour : (downbeat ? colours::panelLight.brighter (0.06f) : colours::panelLight));
                g.fillRoundedRectangle (cell, 4.0f);

                if (on)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.18f));
                    g.fillRoundedRectangle (cell.withHeight (cell.getHeight() * 0.45f), 4.0f);
                }

                if (s == current)
                {
                    g.setColour (on ? juce::Colours::white.withAlpha (0.35f) : colours::playhead.withAlpha (0.12f));
                    g.fillRoundedRectangle (cell, 4.0f);
                }

                g.setColour (colours::outline);
                g.drawRoundedRectangle (cell, 4.0f, 1.0f);
            }
        }
    }

    //==========================================================================
    void StepGrid::mouseDown (const juce::MouseEvent& e)
    {
        int row = -1, step = -1;

        if (getCellAt (e.getPosition(), row, step))
        {
            dragValue = ! pattern.get (row, step);
            pattern.set (row, step, dragValue);
            lastDragRow = row;
            lastDragStep = step;
            repaint();
            return;
        }

        for (int r = 0; r < numInstruments; ++r)
            if (getNameBounds (r).contains (e.getPosition()))
                processor.auditionInstrument (r);
    }

    void StepGrid::mouseDrag (const juce::MouseEvent& e)
    {
        int row = -1, step = -1;

        if (getCellAt (e.getPosition(), row, step) && (row != lastDragRow || step != lastDragStep))
        {
            pattern.set (row, step, dragValue);
            lastDragRow = row;
            lastDragStep = step;
            repaint();
        }
    }

    //==========================================================================
    void StepGrid::timerCallback()
    {
        bool needsRepaint = false;

        const auto step = processor.getSequencer().getCurrentStep();
        if (step != lastStep)
        {
            lastStep = step;
            needsRepaint = true;
        }

        // Pattern kann sich auch durch Laden eines DAW-Projekts aendern
        for (int r = 0; r < numPatternRows; ++r)
        {
            const auto bits = pattern.getRowBits (r);
            if (bits != lastBits[(size_t) r])
            {
                lastBits[(size_t) r] = bits;
                needsRepaint = true;
            }
        }

        for (int i = 0; i < numInstruments; ++i)
        {
            const auto count = processor.getEngine().getTriggerCount (i);
            auto& led = ledLevels[(size_t) i];

            if (count != lastTriggerCounts[(size_t) i])
            {
                lastTriggerCounts[(size_t) i] = count;
                led = 1.0f;
                needsRepaint = true;
            }
            else if (led > 0.0f)
            {
                led = juce::jmax (0.0f, led - 0.12f);
                needsRepaint = true;
            }
        }

        if (needsRepaint)
            repaint();
    }
}
