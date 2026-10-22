#include "Sequencer.h"

namespace basti
{
    //==========================================================================
    PatternData::PatternData()
    {
        clear();

        // Start-Pattern, damit sofort etwas klingt
        for (int s : { 0, 6, 10 })       set (Kick, s, true);
        for (int s : { 4, 12 })          set (Snare, s, true);
        for (int s = 0; s < numSteps; s += 2) set (ClosedHat, s, true);
        set (OpenHat, 14, true);
        set (ClosedHat, 14, false);
        for (int s : { 0, 8 })           set (accentRow, s, true);
    }

    bool PatternData::get (int row, int step) const noexcept
    {
        if (! juce::isPositiveAndBelow (row, numPatternRows) || ! juce::isPositiveAndBelow (step, numSteps))
            return false;

        return (rows[(size_t) row].load() >> step) & 1u;
    }

    void PatternData::set (int row, int step, bool on) noexcept
    {
        if (! juce::isPositiveAndBelow (row, numPatternRows) || ! juce::isPositiveAndBelow (step, numSteps))
            return;

        const auto mask = 1u << step;
        if (on) rows[(size_t) row].fetch_or (mask);
        else    rows[(size_t) row].fetch_and (~mask);
    }

    void PatternData::toggle (int row, int step) noexcept
    {
        if (juce::isPositiveAndBelow (row, numPatternRows) && juce::isPositiveAndBelow (step, numSteps))
            rows[(size_t) row].fetch_xor (1u << step);
    }

    void PatternData::clear() noexcept
    {
        for (auto& r : rows)
            r.store (0);
    }

    uint32_t PatternData::getRowBits (int row) const noexcept
    {
        return juce::isPositiveAndBelow (row, numPatternRows) ? rows[(size_t) row].load() : 0u;
    }

    void PatternData::setRowBits (int row, uint32_t bits) noexcept
    {
        if (juce::isPositiveAndBelow (row, numPatternRows))
            rows[(size_t) row].store (bits & 0xFFFFu);
    }

    juce::ValueTree PatternData::toValueTree() const
    {
        juce::ValueTree tree (treeType);

        for (int r = 0; r < numPatternRows; ++r)
            tree.setProperty ("row" + juce::String (r), (int) getRowBits (r), nullptr);

        return tree;
    }

    void PatternData::fromValueTree (const juce::ValueTree& tree)
    {
        if (! tree.hasType (treeType))
            return;

        for (int r = 0; r < numPatternRows; ++r)
            setRowBits (r, (uint32_t) (int) tree.getProperty ("row" + juce::String (r), 0));
    }

    //==========================================================================
    StepSequencer::StepSequencer (juce::AudioProcessorValueTreeState& apvts, PatternData& p)
        : pattern (p)
    {
        runParam    = apvts.getRawParameterValue (ids::seqRun);
        syncParam   = apvts.getRawParameterValue (ids::seqHostSync);
        tempoParam  = apvts.getRawParameterValue (ids::seqTempo);
        swingParam  = apvts.getRawParameterValue (ids::seqSwing);
        accentParam = apvts.getRawParameterValue (ids::accentAmount);
    }

    void StepSequencer::prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        internalRunning = false;
        currentStep = -1;
    }

    void StepSequencer::process (juce::AudioPlayHead* playHead, int numSamples, DrumEngine& engine)
    {
        const bool hostSync = syncParam->load() > 0.5f;
        const bool run      = runParam->load() > 0.5f;

        bool   hostPlaying = false;
        double hostBpm = 0.0, hostPpq = 0.0;
        bool   hasPpq = false;

        if (playHead != nullptr)
        {
            if (const auto pos = playHead->getPosition())
            {
                hostPlaying = pos->getIsPlaying();

                if (const auto bpm = pos->getBpm())
                    hostBpm = *bpm;

                if (const auto ppq = pos->getPpqPosition())
                {
                    hostPpq = *ppq;
                    hasPpq = true;
                }
            }
        }

        double startPos = 0.0, bpm = tempoParam->load();

        if (hostSync && hostPlaying && hasPpq && hostBpm > 0.0)
        {
            // DAW-Transport: Position direkt aus PPQ (1 Viertel = 4 Steps)
            bpm = hostBpm;
            startPos = hostPpq * 4.0;
            internalRunning = false;
        }
        else if (run)
        {
            // interner Takt (z.B. Standalone oder DAW gestoppt)
            if (! internalRunning)
            {
                internalRunning  = true;
                internalPosition = 0.0;
            }

            if (hostSync && hostBpm > 0.0)
                bpm = hostBpm;

            startPos = internalPosition;
        }
        else
        {
            internalRunning = false;
            currentStep = -1;
            return;
        }

        const auto stepsPerSample = bpm / 60.0 * 4.0 / sampleRate;
        const auto endPos = startPos + numSamples * stepsPerSample;
        const auto swing  = (double) swingParam->load();   // Verzoegerung jeder 2. 16tel (Anteil eines Steps)

        // alle Step-Zeitpunkte t_k = k (+ swing bei ungeraden k) im Fenster [startPos, endPos)
        for (auto k = (int64_t) std::floor (startPos) - 1; k <= (int64_t) std::floor (endPos); ++k)
        {
            const auto t = (double) k + ((k & 1) != 0 ? swing : 0.0);

            if (t >= startPos && t < endPos)
            {
                const auto offset = juce::jlimit (0, numSamples - 1, (int) ((t - startPos) / stepsPerSample));
                const auto step   = (int) (((k % numSteps) + numSteps) % numSteps);
                fireStep (step, offset, engine);
            }
        }

        if (internalRunning)
            internalPosition = endPos;
    }

    void StepSequencer::fireStep (int step, int sampleOffset, DrumEngine& engine)
    {
        currentStep = step;

        const bool  accent   = pattern.get (accentRow, step);
        const float velocity = 0.8f * (accent ? 1.0f + accentParam->load() : 1.0f);

        for (int i = 0; i < numInstruments; ++i)
            if (pattern.get (i, step))
                engine.addTrigger ({ sampleOffset, i, velocity, 0 });
    }
}
