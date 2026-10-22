#pragma once

// Kleine, JUCE-unabhaengige DSP-Bausteine fuer die Drum-Stimmen.
// Alles inline und allokationsfrei -> sicher im Audio-Thread.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace basti::dsp
{
    constexpr float pi    = 3.14159265358979323846f;
    constexpr float twoPi = 2.0f * pi;

    //==========================================================================
    /** Schneller Weissrausch-Generator (LCG), Ausgabe -1..+1. */
    struct WhiteNoise
    {
        explicit WhiteNoise (uint32_t seed = 0x1234567u) : state (seed) {}

        float next() noexcept
        {
            state = state * 1664525u + 1013904223u;
            return static_cast<float> (static_cast<int32_t> (state)) * (1.0f / 2147483648.0f);
        }

        uint32_t state;
    };

    //==========================================================================
    /** Exponentielle Abkling-Huellkurve: faellt in 'seconds' auf -60 dB. */
    struct DecayEnvelope
    {
        void setDecayTime (float seconds, double sampleRate) noexcept
        {
            const auto samples = std::max (1.0, static_cast<double> (seconds) * sampleRate);
            coef = static_cast<float> (std::exp (std::log (0.001) / samples));
        }

        void trigger (float startValue = 1.0f) noexcept   { value = startValue; }
        void reset() noexcept                             { value = 0.0f; }

        float next() noexcept
        {
            const auto v = value;
            value *= coef;
            return v;
        }

        bool isActive (float threshold = 1.0e-4f) const noexcept { return value > threshold; }

        float value = 0.0f;
        float coef  = 0.0f;
    };

    //==========================================================================
    /** Einpoliger Tiefpass (6 dB/Okt). */
    struct OnePoleLowpass
    {
        void setCutoff (float hz, double sampleRate) noexcept
        {
            a = 1.0f - std::exp (-twoPi * hz / static_cast<float> (sampleRate));
        }

        float process (float x) noexcept   { z += a * (x - z); return z; }
        void reset() noexcept              { z = 0.0f; }

        float a = 1.0f, z = 0.0f;
    };

    //==========================================================================
    /** Zustandsvariablen-Filter (TPT/Zavalishin), liefert LP/BP/HP gleichzeitig.
        Stabil auch bei Modulation, 12 dB/Okt. */
    struct StateVariableFilter
    {
        struct Output { float lowpass, bandpass, highpass, bandpassNormalised; };

        void setParameters (float cutoffHz, float q, double sampleRate) noexcept
        {
            const auto sr = static_cast<float> (sampleRate);
            cutoffHz = std::clamp (cutoffHz, 10.0f, 0.49f * sr);
            g  = std::tan (pi * cutoffHz / sr);
            k  = 1.0f / std::max (q, 0.05f);
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        Output process (float v0) noexcept
        {
            const auto v3 = v0 - ic2eq;
            const auto v1 = a1 * ic1eq + a2 * v3;
            const auto v2 = ic2eq + a2 * ic1eq + a3 * v3;
            ic1eq = 2.0f * v1 - ic1eq;
            ic2eq = 2.0f * v2 - ic2eq;
            return { v2, v1, v0 - k * v1 - v2, k * v1 };
        }

        void reset() noexcept { ic1eq = ic2eq = 0.0f; }

        float g = 0.0f, k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        float ic1eq = 0.0f, ic2eq = 0.0f;
    };

    //==========================================================================
    /** Band-begrenzter Rechteck-Oszillator (PolyBLEP), frei laufend wie im
        Analog-Original -> Phase wird beim Trigger NICHT zurueckgesetzt. */
    struct PolyBlepSquare
    {
        void setFrequency (float hz, double sampleRate) noexcept
        {
            increment = hz / static_cast<float> (sampleRate);
        }

        float next() noexcept
        {
            auto out = phase < 0.5f ? 1.0f : -1.0f;
            out += blep (phase);
            out -= blep (std::fmod (phase + 0.5f, 1.0f));

            phase += increment;
            if (phase >= 1.0f)
                phase -= 1.0f;

            return out;
        }

        float phase = 0.0f, increment = 0.0f;

    private:
        float blep (float t) const noexcept
        {
            const auto dt = increment;

            if (t < dt)
            {
                t /= dt;
                return t + t - t * t - 1.0f;
            }

            if (t > 1.0f - dt)
            {
                t = (t - 1.0f) / dt;
                return t * t + t + t + 1.0f;
            }

            return 0.0f;
        }
    };

    //==========================================================================
    inline float semitonesToRatio (float semitones) noexcept
    {
        return std::exp2 (semitones / 12.0f);
    }

    /** Sanfte Saettigung fuer "analoge" Spitzen (Akzent-Schlaege). */
    inline float softClip (float x) noexcept
    {
        return std::tanh (x);
    }
}
