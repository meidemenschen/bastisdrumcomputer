#include "DrumEngine.h"

namespace basti
{
    DrumEngine::DrumEngine (juce::AudioProcessorValueTreeState& apvts)
        : state (apvts)
    {
        voices = { &kick, &snare, &clap, &closedHat, &openHat, &lowTom, &midTom, &highTom };

        for (int i = 0; i < numInstruments; ++i)
        {
            auto& p = params[(size_t) i];
            p.level = state.getRawParameterValue (paramId (i, ids::level));
            p.pan   = state.getRawParameterValue (paramId (i, ids::pan));

            const auto suffixes = getSynthParamSuffixes (i);
            std::atomic<float>** slots[] = { &p.a, &p.b, &p.c, &p.d };

            for (int s = 0; s < suffixes.size() && s < 4; ++s)
                *slots[s] = state.getRawParameterValue (paramId (i, suffixes[s]));

            jassert (p.level != nullptr && p.pan != nullptr && p.a != nullptr);
        }

        masterVolume = state.getRawParameterValue (ids::masterVolume);
        jassert (masterVolume != nullptr);
    }

    void DrumEngine::prepare (double sampleRate, int maxBlockSize)
    {
        for (auto* v : voices)
            v->prepare (sampleRate);

        voiceBuffer.setSize (numInstruments, juce::jmax (1, maxBlockSize));
        lastGainL.fill (0.0f);
        lastGainR.fill (0.0f);
        lastMasterGain = juce::Decibels::decibelsToGain (masterVolume->load(), -60.0f);
        numEvents = 0;
    }

    void DrumEngine::reset()
    {
        for (auto* v : voices)
            v->reset();

        numEvents = 0;
    }

    //==========================================================================
    void DrumEngine::updateVoiceParameters()
    {
        const auto& kp = params[Kick];
        kick.setParams ({ kp.a->load(), kp.b->load(), kp.c->load() });            // pitch, decay, tone

        const auto& sp = params[Snare];
        snare.setParams ({ sp.a->load(), sp.b->load(), sp.c->load(), sp.d->load() }); // tune, decay, tone, snappy

        const auto& cp = params[Clap];
        clap.setParams ({ cp.a->load(), cp.b->load() });                         // tone, decay

        const auto& ch = params[ClosedHat];
        closedHat.setParams ({ ch.a->load(), ch.b->load() });                    // tone, decay

        const auto& oh = params[OpenHat];
        openHat.setParams ({ oh.a->load(), oh.b->load() });                      // tone, decay

        dsp::TomVoice* toms[] = { &lowTom, &midTom, &highTom };
        for (int t = 0; t < 3; ++t)
        {
            const auto& tp = params[(size_t) (LowTom + t)];
            toms[t]->setParams ({ tp.a->load(), tp.b->load(), tp.c->load() > 0.5f }); // tune, decay, mode
        }
    }

    void DrumEngine::beginBlock()
    {
        numEvents = 0;
        updateVoiceParameters();
    }

    void DrumEngine::addTrigger (const TriggerEvent& e) noexcept
    {
        if (! juce::isPositiveAndBelow (e.instrument, numInstruments))
            return;

        if (numEvents < (int) events.size())
            events[(size_t) numEvents++] = e;

        // Closed Hat schneidet die Open Hat ab (wie beim Original-Schaltungsprinzip)
        if (e.instrument == ClosedHat && e.velocity > 0.0f && numEvents < (int) events.size())
            events[(size_t) numEvents++] = { e.sampleOffset, OpenHat, -1.0f, 0 };
    }

    //==========================================================================
    void DrumEngine::render (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (voiceBuffer.getNumSamples() < numSamples)   // sollte nach prepare() nie passieren
            voiceBuffer.setSize (numInstruments, numSamples, false, false, true);

        std::stable_sort (events.begin(), events.begin() + numEvents,
                          [] (const TriggerEvent& a, const TriggerEvent& b) { return a.sampleOffset < b.sampleOffset; });

        voiceBuffer.clear (0, numSamples);

        for (int i = 0; i < numInstruments; ++i)
        {
            auto& v   = voice (i);
            auto* out = voiceBuffer.getWritePointer (i);
            int pos = 0;

            for (int e = 0; e < numEvents; ++e)
            {
                const auto& ev = events[(size_t) e];
                if (ev.instrument != i)
                    continue;

                const auto offset = juce::jlimit (0, numSamples - 1, ev.sampleOffset);
                v.render (out + pos, offset - pos);
                pos = offset;

                if (ev.velocity < 0.0f)
                {
                    v.choke();
                }
                else
                {
                    v.trigger (ev.velocity, ev.variant);
                    ++triggerCounts[(size_t) i];
                }
            }

            v.render (out + pos, numSamples - pos);
        }

        numEvents = 0;

        // Mixdown mit Level/Pan (Gain-Rampen gegen Zipper-Rauschen)
        buffer.clear (0, numSamples);
        const auto numOut = buffer.getNumChannels();

        for (int i = 0; i < numInstruments; ++i)
        {
            const auto& p   = params[(size_t) i];
            const auto lvl  = p.level->load();
            const auto gain = lvl * lvl;                                          // quadratische Kennlinie
            const auto angle = (p.pan->load() + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            const auto gL = gain * std::cos (angle) * juce::MathConstants<float>::sqrt2;
            const auto gR = gain * std::sin (angle) * juce::MathConstants<float>::sqrt2;

            const auto* src = voiceBuffer.getReadPointer (i);

            if (numOut >= 2)
            {
                buffer.addFromWithRamp (0, 0, src, numSamples, lastGainL[(size_t) i], gL);
                buffer.addFromWithRamp (1, 0, src, numSamples, lastGainR[(size_t) i], gR);
            }
            else if (numOut == 1)
            {
                buffer.addFromWithRamp (0, 0, src, numSamples, lastGainL[(size_t) i] * 0.5f + lastGainR[(size_t) i] * 0.5f,
                                        (gL + gR) * 0.5f);
            }

            lastGainL[(size_t) i] = gL;
            lastGainR[(size_t) i] = gR;
        }

        const auto master = juce::Decibels::decibelsToGain (masterVolume->load(), -60.0f);
        buffer.applyGainRamp (0, numSamples, lastMasterGain, master);
        lastMasterGain = master;
    }

    //==========================================================================
    int DrumEngine::instrumentForMidiNote (int note, int& variant) noexcept
    {
        variant = 0;

        switch (note)
        {
            case 35: case 36:          return Kick;        // Acoustic / Bass Drum 1 (C1)
            case 37: case 38: case 40: return Snare;       // Side Stick, Snare 1/2 (D1, E1)
            case 39:                   return Clap;        // Hand Clap (D#1)
            case 42: case 44:          return ClosedHat;   // Closed / Pedal Hi-Hat (F#1, G#1)
            case 46:                   return OpenHat;     // Open Hi-Hat (A#1)
            case 41: case 43: case 45: return LowTom;      // Floor Toms / Low Tom
            case 47: case 48:          return MidTom;      // Low-Mid / Hi-Mid Tom
            case 50:                   return HighTom;     // High Tom
            case 64: variant = 1;      return LowTom;      // Low Conga
            case 63: variant = 1;      return MidTom;      // Open Hi Conga
            case 62: variant = 1;      return HighTom;     // Mute Hi Conga
            default:                   return -1;
        }
    }
}
