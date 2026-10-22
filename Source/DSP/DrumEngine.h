#pragma once

#include <JuceHeader.h>

#include "DrumVoices.h"
#include "../Parameters.h"

namespace basti
{
    /** Ein Trigger-Ereignis innerhalb des aktuellen Audio-Blocks
        (aus MIDI oder dem internen Sequenzer). */
    struct TriggerEvent
    {
        int   sampleOffset = 0;
        int   instrument   = 0;
        float velocity     = 1.0f;   // < 0 bedeutet "choke"
        int   variant      = 0;      // z.B. 1 = Conga erzwingen
    };

    //==========================================================================
    /** Synthese-Engine: verwaltet alle Drum-Stimmen, liest die APVTS-Parameter,
        verarbeitet sample-genaue Trigger und mischt in den Stereo-Ausgang. */
    class DrumEngine
    {
    public:
        explicit DrumEngine (juce::AudioProcessorValueTreeState& apvts);

        void prepare (double sampleRate, int maxBlockSize);
        void reset();

        /** Am Anfang jedes Blocks: Parameter einlesen, Event-Liste leeren. */
        void beginBlock();

        /** Trigger fuer den aktuellen Block einreihen (Audio-Thread). */
        void addTrigger (const TriggerEvent& e) noexcept;

        /** Rendert alle Stimmen in buffer (wird ueberschrieben). */
        void render (juce::AudioBuffer<float>& buffer, int numSamples);

        /** GM-Drum-Map: liefert Instrument fuer MIDI-Note oder -1.
            variant wird fuer Conga-Noten auf 1 gesetzt. */
        static int instrumentForMidiNote (int note, int& variant) noexcept;

        /** Fuer die GUI: zaehlt hoch, wenn ein Instrument getriggert wurde. */
        int getTriggerCount (int instrument) const noexcept { return triggerCounts[(size_t) instrument].load(); }

    private:
        dsp::DrumVoice& voice (int instrument) noexcept { return *voices[(size_t) instrument]; }
        void updateVoiceParameters();

        juce::AudioProcessorValueTreeState& state;

        dsp::KickVoice  kick;
        dsp::SnareVoice snare;
        dsp::ClapVoice  clap;
        dsp::HiHatVoice closedHat, openHat;
        dsp::TomVoice   lowTom  { 92.0f,  175.0f },
                        midTom  { 130.0f, 255.0f },
                        highTom { 180.0f, 370.0f };

        std::array<dsp::DrumVoice*, numInstruments> voices {};

        // gecachte Parameter-Zeiger (Lesen im Audio-Thread ohne String-Lookup)
        struct InstrumentParams
        {
            std::atomic<float>* level = nullptr;
            std::atomic<float>* pan   = nullptr;
            std::atomic<float>* a     = nullptr;   // erster Synth-Parameter
            std::atomic<float>* b     = nullptr;   // zweiter
            std::atomic<float>* c     = nullptr;   // dritter (optional)
        };

        std::array<InstrumentParams, numInstruments> params {};
        std::atomic<float>* masterVolume = nullptr;

        std::array<TriggerEvent, 512> events {};
        int numEvents = 0;

        juce::AudioBuffer<float> voiceBuffer;
        std::array<float, numInstruments> lastGainL {}, lastGainR {};
        float lastMasterGain = 1.0f;

        std::array<std::atomic<int>, numInstruments> triggerCounts {};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumEngine)
    };
}
