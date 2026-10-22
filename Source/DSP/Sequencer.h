#pragma once

#include <JuceHeader.h>

#include "DrumEngine.h"

namespace basti
{
    //==========================================================================
    /** Pattern-Speicher: 16 Steps pro Zeile als Bitmaske.
        Lock-free: GUI schreibt, Audio-Thread liest (std::atomic). */
    class PatternData
    {
    public:
        PatternData();

        bool get (int row, int step) const noexcept;
        void set (int row, int step, bool on) noexcept;
        void toggle (int row, int step) noexcept;
        void clear() noexcept;

        uint32_t getRowBits (int row) const noexcept;
        void setRowBits (int row, uint32_t bits) noexcept;

        /** Fuer DAW-Projekt speichern/laden (Teil des Plugin-States). */
        juce::ValueTree toValueTree() const;
        void fromValueTree (const juce::ValueTree& tree);

        static inline const juce::Identifier treeType { "PATTERN" };

    private:
        std::array<std::atomic<uint32_t>, numPatternRows> rows;
    };

    //==========================================================================
    /** 16-Step-Sequenzer (16tel-Noten) mit Accent, Swing und wahlweise
        Host-Sync (folgt DAW-Transport) oder internem Tempo (Run-Taster). */
    class StepSequencer
    {
    public:
        StepSequencer (juce::AudioProcessorValueTreeState& apvts, PatternData& pattern);

        void prepare (double sampleRate);

        /** Erzeugt fuer den aktuellen Block Trigger-Events in der Engine. */
        void process (juce::AudioPlayHead* playHead, int numSamples, DrumEngine& engine);

        /** Aktuell spielender Step (0..15) oder -1, wenn gestoppt. Fuer die GUI. */
        int getCurrentStep() const noexcept { return currentStep.load(); }

    private:
        void fireStep (int step, int sampleOffset, DrumEngine& engine);

        PatternData& pattern;

        std::atomic<float>* runParam    = nullptr;
        std::atomic<float>* syncParam   = nullptr;
        std::atomic<float>* tempoParam  = nullptr;
        std::atomic<float>* swingParam  = nullptr;
        std::atomic<float>* accentParam = nullptr;

        double sampleRate = 44100.0;
        double internalPosition = 0.0;   // in 16tel-Steps
        bool   internalRunning  = false;

        std::atomic<int> currentStep { -1 };
    };
}
