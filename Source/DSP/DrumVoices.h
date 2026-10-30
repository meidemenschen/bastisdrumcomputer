#pragma once

// Reine DSP-Synthese der einzelnen Drum-Stimmen (kein Sampling).
// Bewusst JUCE-unabhaengig, damit sich die Stimmen auch isoliert testen lassen.
//
// Jede Stimme ist monophon wie bei klassischen Analog-Drumcomputern: ein neuer
// Trigger startet die Stimme neu. Alle Stimmen spielen gleichzeitig (die Engine
// ist damit insgesamt polyphon).

#include "DspHelpers.h"

#include <array>

namespace basti::dsp
{
    //==========================================================================
    class DrumVoice
    {
    public:
        virtual ~DrumVoice() = default;

        virtual void prepare (double newSampleRate)
        {
            sampleRate = newSampleRate;
            reset();
        }

        virtual void reset() = 0;

        /** velocity: 0..~2 (Werte > 1 entstehen durch Accent). variant: z.B. Conga-Modus. */
        virtual void trigger (float velocity, int variant) = 0;

        /** Schnelles Abwuergen (z.B. Open Hat wird von Closed Hat abgeschnitten). */
        virtual void choke() {}

        virtual bool isActive() const = 0;

        /** Addiert numSamples Samples in dest (mono). */
        virtual void render (float* dest, int numSamples) = 0;

    protected:
        double sampleRate = 44100.0;
    };

    //==========================================================================
    /** Kick: gedaempfter Sinus mit schneller Pitch-Huellkurve, Klick-Transient
        und Tone-Tiefpass. Decay stufenlos von kurz/trocken bis lang/boomy. */
    class KickVoice final : public DrumVoice
    {
    public:
        struct Params { float pitchHz = 52.0f, decay = 0.6f, tone = 0.5f; };
        void setParams (const Params& p) noexcept { params = p; }

        void reset() override
        {
            amp.reset(); click.reset(); toneFilter.reset();
            phase = 0.0f; pitchEnv = 0.0f;
        }

        void trigger (float velocity, int) override
        {
            gain   = velocity;
            drive  = 1.0f + 0.8f * std::max (0.0f, velocity - 0.8f);   // Accent -> mehr Druck
            sweep  = 1.2f + 1.3f * std::min (velocity, 1.5f);          // Pitch-Sweep-Tiefe

            phase    = 0.0f;
            pitchEnv = 1.0f;
            pitchCoef = std::exp (-1.0f / (0.007f * static_cast<float> (sampleRate)));

            amp.setDecayTime (params.decay, sampleRate);
            amp.trigger (1.0f);

            click.setDecayTime (0.006f, sampleRate);
            click.trigger (0.15f + 0.6f * params.tone);

            toneFilter.setCutoff (120.0f * std::pow (60.0f, params.tone), sampleRate); // 120 Hz .. 7.2 kHz
        }

        bool isActive() const override { return amp.isActive(); }

        void render (float* dest, int numSamples) override
        {
            if (! isActive())
                return;

            const auto invSr = 1.0f / static_cast<float> (sampleRate);

            for (int i = 0; i < numSamples; ++i)
            {
                const auto freq = params.pitchHz * (1.0f + sweep * pitchEnv);
                pitchEnv *= pitchCoef;

                phase += freq * invSr;
                if (phase >= 1.0f) phase -= 1.0f;

                const auto body  = std::sin (twoPi * phase) * amp.next();
                const auto trans = toneFilter.process (noise.next() * click.next());

                dest[i] += softClip ((body + trans) * drive) * gain;
            }
        }

    private:
        Params params;
        DecayEnvelope amp, click;
        OnePoleLowpass toneFilter;
        WhiteNoise noise { 0xBADC0DEu };
        float phase = 0.0f, pitchEnv = 0.0f, pitchCoef = 0.0f;
        float sweep = 1.0f, drive = 1.0f, gain = 1.0f;
    };

    //==========================================================================
    /** Snare: zwei gestimmte, gedaempfte Sinus-Koerper + hochpassgefiltertes
        Rauschen ("Snappy"). Tone verschiebt Koerper-Balance und Rauschfarbe,
        Decay streckt/staucht Koerper und Rauschfahne gemeinsam. */
    class SnareVoice final : public DrumVoice
    {
    public:
        struct Params { float tune = 0.0f, decay = 0.2f, tone = 0.5f, snappy = 0.6f; };
        void setParams (const Params& p) noexcept { params = p; }

        void reset() override
        {
            env1.reset(); env2.reset(); noiseEnv.reset();
            hp.reset(); lp.reset();
            phase1 = phase2 = 0.0f; pitchEnv = 0.0f;
        }

        void trigger (float velocity, int) override
        {
            gain = velocity;
            const auto ratio = semitonesToRatio (params.tune);
            freq1 = 185.0f * ratio;
            freq2 = 330.0f * ratio;
            phase1 = phase2 = 0.0f;

            pitchEnv  = 1.0f;
            pitchCoef = std::exp (-1.0f / (0.012f * static_cast<float> (sampleRate)));

            body1Gain = 0.9f - 0.5f * params.tone;
            body2Gain = 0.3f + 0.6f * params.tone;

            // Decay 0.2 s entspricht dem bisherigen Grundklang
            const auto decayScale = params.decay / 0.2f;

            env1.setDecayTime (0.20f * decayScale, sampleRate);  env1.trigger();
            env2.setDecayTime (0.12f * decayScale, sampleRate);  env2.trigger();

            noiseEnv.setDecayTime ((0.12f + 0.18f * params.snappy) * decayScale, sampleRate);
            noiseEnv.trigger (params.snappy * 1.4f);

            hp.setParameters (900.0f + 2600.0f * params.tone, 0.7f, sampleRate);
            lp.setCutoff (6000.0f + 6000.0f * params.tone, sampleRate);
        }

        bool isActive() const override { return env1.isActive() || noiseEnv.isActive(); }

        void render (float* dest, int numSamples) override
        {
            if (! isActive())
                return;

            const auto invSr = 1.0f / static_cast<float> (sampleRate);

            for (int i = 0; i < numSamples; ++i)
            {
                const auto bend = 1.0f + 0.15f * pitchEnv;
                pitchEnv *= pitchCoef;

                phase1 += freq1 * bend * invSr;  if (phase1 >= 1.0f) phase1 -= 1.0f;
                phase2 += freq2 * bend * invSr;  if (phase2 >= 1.0f) phase2 -= 1.0f;

                const auto body = body1Gain * std::sin (twoPi * phase1) * env1.next()
                                + body2Gain * std::sin (twoPi * phase2) * env2.next();

                const auto n = lp.process (hp.process (noise.next()).highpass) * noiseEnv.next();

                dest[i] += softClip ((0.7f * body + n) * (0.8f + 0.3f * gain)) * gain;
            }
        }

    private:
        Params params;
        DecayEnvelope env1, env2, noiseEnv;
        StateVariableFilter hp;
        OnePoleLowpass lp;
        WhiteNoise noise { 0x5EED5u };
        float phase1 = 0.0f, phase2 = 0.0f, freq1 = 185.0f, freq2 = 330.0f;
        float pitchEnv = 0.0f, pitchCoef = 0.0f;
        float body1Gain = 0.6f, body2Gain = 0.6f, gain = 1.0f;
    };

    //==========================================================================
    /** Hi-Hat: sechs frei laufende, unharmonisch verstimmte Rechteck-Oszillatoren
        (Emulation des Schmitt-Trigger-Oszillator-Netzwerks), zwei kaskadierte
        Bandpaesse + Hochpass, VCA mit kurzer (Closed) oder langer (Open) Huellkurve. */
    class HiHatVoice final : public DrumVoice
    {
    public:
        struct Params { float tone = 0.5f, decay = 0.05f; };
        void setParams (const Params& p) noexcept { params = p; }

        void prepare (double newSampleRate) override
        {
            DrumVoice::prepare (newSampleRate);

            // klassisches, unharmonisches "Metall"-Frequenzset
            static constexpr std::array<float, 6> freqs { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
            for (size_t i = 0; i < oscs.size(); ++i)
            {
                oscs[i].setFrequency (freqs[i], sampleRate);
                oscs[i].phase = 0.17f * static_cast<float> (i);
            }
        }

        void reset() override
        {
            env.reset(); bp1.reset(); bp2.reset(); hp.reset();
        }

        void trigger (float velocity, int) override
        {
            gain = velocity;
            env.setDecayTime (params.decay, sampleRate);
            env.trigger();

            const auto centre = 6000.0f + 5000.0f * params.tone;   // 6 .. 11 kHz
            bp1.setParameters (centre,         1.6f, sampleRate);
            bp2.setParameters (centre * 1.15f, 1.6f, sampleRate);
            hp.setParameters  (5500.0f + 2000.0f * params.tone, 0.7f, sampleRate);
        }

        void choke() override
        {
            env.setDecayTime (0.006f, sampleRate);
        }

        bool isActive() const override { return env.isActive(); }

        void render (float* dest, int numSamples) override
        {
            if (! isActive())
                return;

            for (int i = 0; i < numSamples; ++i)
            {
                auto metal = 0.0f;
                for (auto& o : oscs)
                    metal += o.next();

                auto x = bp1.process (metal).bandpassNormalised;
                x = bp2.process (x).bandpassNormalised;
                x = hp.process (x).highpass;

                dest[i] += softClip (x * makeUpGain * env.next()) * gain;
            }
        }

    private:
        static constexpr float makeUpGain = 2.0f;

        Params params;
        std::array<PolyBlepSquare, 6> oscs;
        DecayEnvelope env;
        StateVariableFilter bp1, bp2, hp;
        float gain = 1.0f;
    };

    //==========================================================================
    /** Clap: mehrere sehr kurze Rausch-Bursts (gefiltertes Rauschen mit
        Saegezahn-artiger Huellkurve) gefolgt von einer abklingenden Rauschfahne. */
    class ClapVoice final : public DrumVoice
    {
    public:
        struct Params { float tone = 0.5f, decay = 0.3f; };
        void setParams (const Params& p) noexcept { params = p; }

        void reset() override
        {
            burstEnv.reset(); tailEnv.reset(); bp.reset(); tailBp.reset();
            counter = 0; burstsLeft = 0; tailStart = -1;
        }

        void trigger (float velocity, int) override
        {
            gain = velocity;
            const auto spacing = static_cast<int> (0.0105 * sampleRate);  // ~10 ms zwischen Bursts

            counter       = 0;
            burstSpacing  = std::max (1, spacing);
            burstsLeft    = numBursts;
            nextBurst     = 0;
            tailStart     = numBursts * burstSpacing - burstSpacing / 2;

            burstEnv.setDecayTime (0.009f, sampleRate);
            burstEnv.reset();
            tailEnv.setDecayTime (params.decay, sampleRate);
            tailEnv.reset();

            const auto centre = 800.0f + 1600.0f * params.tone;   // 0.8 .. 2.4 kHz
            bp.setParameters     (centre,         2.5f, sampleRate);
            tailBp.setParameters (centre * 0.9f,  1.5f, sampleRate);
        }

        bool isActive() const override
        {
            return burstsLeft > 0 || counter <= tailStart || burstEnv.isActive() || tailEnv.isActive();
        }

        void render (float* dest, int numSamples) override
        {
            if (! isActive())
                return;

            for (int i = 0; i < numSamples; ++i)
            {
                if (burstsLeft > 0 && counter == nextBurst)
                {
                    burstEnv.trigger (1.0f);
                    --burstsLeft;
                    nextBurst += burstSpacing;
                }

                if (counter == tailStart)
                    tailEnv.trigger (0.55f);

                ++counter;

                const auto n = noise.next();
                const auto bursts = bp.process (n).bandpassNormalised * burstEnv.next();
                const auto tail   = tailBp.process (n).bandpassNormalised * tailEnv.next();

                dest[i] += softClip ((bursts + tail) * 3.0f) * gain;
            }
        }

    private:
        static constexpr int numBursts = 4;

        Params params;
        DecayEnvelope burstEnv, tailEnv;
        StateVariableFilter bp, tailBp;
        WhiteNoise noise { 0xC1A9u };
        int counter = 0, burstSpacing = 1, burstsLeft = 0, nextBurst = 0, tailStart = -1;
        float gain = 1.0f;
    };

    //==========================================================================
    /** Tom / Conga: gedaempfter Resonator (rotierender komplexer Zeiger, der
        per Impuls angestossen wird) mit leichtem Pitch-Abfall. Im Tom-Modus kommt
        etwas gefiltertes Rauschen dazu, im Conga-Modus ist der Ton hoeher,
        kuerzer und klarer. */
    class TomVoice final : public DrumVoice
    {
    public:
        struct Params { float tune = 0.0f, decay = 0.4f; bool conga = false; };
        void setParams (const Params& p) noexcept { params = p; }

        TomVoice (float tomBaseHz, float congaBaseHz) : tomBase (tomBaseHz), congaBase (congaBaseHz) {}

        void reset() override
        {
            re = im = 0.0f; noiseEnv.reset(); noiseFilter.reset(); pitchEnv = 0.0f;
        }

        /** variant != 0 erzwingt den Conga-Modus (GM-Conga-Noten). */
        void trigger (float velocity, int variant) override
        {
            gain = velocity;
            const bool conga = params.conga || variant != 0;
            const auto sr = static_cast<float> (sampleRate);

            baseFreq  = (conga ? congaBase : tomBase) * semitonesToRatio (params.tune);
            bendDepth = conga ? 0.06f : 0.22f;
            pitchEnv  = 1.0f;
            pitchCoef = std::exp (-1.0f / ((conga ? 0.010f : 0.045f) * sr));

            const auto decay = conga ? params.decay * 0.6f : params.decay;
            damping = std::exp (std::log (0.001f) / std::max (1.0f, decay * sr));

            // Impuls-Anregung: Zeiger auf (1, 0) -> Ausgabe startet als Sinus ab 0
            re = 1.0f;
            im = 0.0f;

            noiseEnv.setDecayTime (conga ? 0.004f : 0.05f, sampleRate);
            noiseEnv.trigger (conga ? 0.25f : 0.12f);
            noiseFilter.setCutoff (conga ? 4000.0f : 1800.0f, sampleRate);
        }

        bool isActive() const override
        {
            return (re * re + im * im) > 1.0e-8f || noiseEnv.isActive();
        }

        void render (float* dest, int numSamples) override
        {
            if (! isActive())
                return;

            const auto invSr = 1.0f / static_cast<float> (sampleRate);

            for (int i = 0; i < numSamples; ++i)
            {
                const auto w = twoPi * baseFreq * (1.0f + bendDepth * pitchEnv) * invSr;
                pitchEnv *= pitchCoef;

                const auto c = std::cos (w) * damping;
                const auto s = std::sin (w) * damping;
                const auto newRe = re * c - im * s;
                im = re * s + im * c;
                re = newRe;

                const auto n = noiseFilter.process (noise.next()) * noiseEnv.next();

                dest[i] += softClip ((im + n) * 1.1f) * gain;
            }
        }

    private:
        Params params;
        float tomBase, congaBase;
        float baseFreq = 100.0f, bendDepth = 0.2f, pitchEnv = 0.0f, pitchCoef = 0.0f;
        float damping = 0.999f, re = 0.0f, im = 0.0f, gain = 1.0f;
        DecayEnvelope noiseEnv;
        OnePoleLowpass noiseFilter;
        WhiteNoise noise { 0x7011u };
    };
}
