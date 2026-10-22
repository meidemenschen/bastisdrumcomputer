#pragma once

#include <JuceHeader.h>

namespace basti
{
    //==========================================================================
    // Instrumente (Reihenfolge = Reihenfolge in Engine, Sequenzer und GUI)
    //==========================================================================
    enum Instrument : int
    {
        Kick = 0,
        Snare,
        Clap,
        ClosedHat,
        OpenHat,
        LowTom,
        MidTom,
        HighTom,
        NumInstruments
    };

    constexpr int numInstruments  = NumInstruments;
    constexpr int numSteps        = 16;
    constexpr int accentRow       = numInstruments;      // letzte Zeile im Pattern = Accent
    constexpr int numPatternRows  = numInstruments + 1;

    /** Anzeigename fuer GUI ("Kick", "Snare", ...). */
    juce::String getInstrumentName (int instrument);

    /** Kurzes Praefix fuer Parameter-IDs ("kick", "snare", ...). */
    juce::String getInstrumentPrefix (int instrument);

    /** Baut eine Parameter-ID aus Instrument + Suffix, z.B. "kick_decay". */
    juce::String paramId (int instrument, const juce::String& suffix);

    //==========================================================================
    // Globale Parameter-IDs
    //==========================================================================
    namespace ids
    {
        inline const juce::String masterVolume { "master_volume" };
        inline const juce::String accentAmount { "accent_amount" };
        inline const juce::String seqRun       { "seq_run" };
        inline const juce::String seqHostSync  { "seq_host_sync" };
        inline const juce::String seqTempo     { "seq_tempo" };
        inline const juce::String seqSwing     { "seq_swing" };

        // Suffixe fuer Instrument-Parameter
        inline const juce::String level  { "level" };
        inline const juce::String pan    { "pan" };
        inline const juce::String pitch  { "pitch" };   // Kick (Hz)
        inline const juce::String tune   { "tune" };    // Snare/Toms (Halbtoene)
        inline const juce::String decay  { "decay" };
        inline const juce::String tone   { "tone" };
        inline const juce::String snappy { "snappy" };
        inline const juce::String mode   { "mode" };    // Toms: Tom / Conga
    }

    /** Welche Synthese-Regler (neben Level/Pan) hat welches Instrument? */
    juce::StringArray getSynthParamSuffixes (int instrument);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}
