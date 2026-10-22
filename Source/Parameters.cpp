#include "Parameters.h"

namespace basti
{
    juce::String getInstrumentName (int instrument)
    {
        switch (instrument)
        {
            case Kick:      return "Kick";
            case Snare:     return "Snare";
            case Clap:      return "Clap";
            case ClosedHat: return "Closed Hat";
            case OpenHat:   return "Open Hat";
            case LowTom:    return "Low Tom";
            case MidTom:    return "Mid Tom";
            case HighTom:   return "High Tom";
            default:        return "Accent";
        }
    }

    juce::String getInstrumentPrefix (int instrument)
    {
        switch (instrument)
        {
            case Kick:      return "kick";
            case Snare:     return "snare";
            case Clap:      return "clap";
            case ClosedHat: return "chat";
            case OpenHat:   return "ohat";
            case LowTom:    return "ltom";
            case MidTom:    return "mtom";
            case HighTom:   return "htom";
            default:        return "accent";
        }
    }

    juce::String paramId (int instrument, const juce::String& suffix)
    {
        return getInstrumentPrefix (instrument) + "_" + suffix;
    }

    juce::StringArray getSynthParamSuffixes (int instrument)
    {
        switch (instrument)
        {
            case Kick:      return { ids::pitch, ids::decay, ids::tone };
            case Snare:     return { ids::tune, ids::tone, ids::snappy };
            case Clap:      return { ids::tone, ids::decay };
            case ClosedHat: return { ids::tone, ids::decay };
            case OpenHat:   return { ids::tone, ids::decay };
            case LowTom:
            case MidTom:
            case HighTom:   return { ids::tune, ids::decay, ids::mode };
            default:        return {};
        }
    }

    //==========================================================================
    namespace
    {
        using Range = juce::NormalisableRange<float>;

        Range skewed (float min, float max, float centre)
        {
            Range r (min, max);
            r.setSkewForCentre (centre);
            return r;
        }

        juce::AudioParameterFloatAttributes withLabel (const juce::String& label, int decimals = 2)
        {
            return juce::AudioParameterFloatAttributes()
                       .withLabel (label)
                       .withStringFromValueFunction ([decimals] (float v, int) { return juce::String (v, decimals); });
        }

        juce::AudioParameterFloatAttributes percent()
        {
            return juce::AudioParameterFloatAttributes()
                       .withLabel ("%")
                       .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)); })
                       .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue() / 100.0f; });
        }

        juce::AudioParameterFloatAttributes seconds()
        {
            return juce::AudioParameterFloatAttributes()
                       .withLabel ("ms")
                       .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 1000.0f)); })
                       .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue() / 1000.0f; });
        }

        struct Defaults
        {
            float decayMin, decayMax, decayCentre, decayDefault;
            float pan;
        };

        Defaults getDefaults (int instrument)
        {
            switch (instrument)
            {
                case Kick:      return { 0.05f, 2.5f,  0.5f,  0.6f,   0.0f  };
                case Snare:     return { 0.05f, 1.0f,  0.25f, 0.25f,  0.0f  };
                case Clap:      return { 0.05f, 1.0f,  0.3f,  0.3f,   0.1f  };
                case ClosedHat: return { 0.01f, 0.25f, 0.06f, 0.05f, -0.2f  };
                case OpenHat:   return { 0.1f,  2.0f,  0.5f,  0.45f, -0.2f  };
                case LowTom:    return { 0.05f, 1.5f,  0.4f,  0.45f, -0.3f  };
                case MidTom:    return { 0.05f, 1.5f,  0.4f,  0.4f,   0.0f  };
                case HighTom:   return { 0.05f, 1.5f,  0.4f,  0.35f,  0.3f  };
                default:        return { 0.05f, 1.0f,  0.3f,  0.3f,   0.0f  };
            }
        }
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        //----------------------------------------------------------------------
        // Global / Master
        //----------------------------------------------------------------------
        auto master = std::make_unique<juce::AudioProcessorParameterGroup> ("master", "Master", "|");

        master->addChild (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ids::masterVolume, 1 }, "Master Volume",
            Range (-60.0f, 6.0f, 0.1f), -3.0f, withLabel ("dB", 1)));

        master->addChild (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ids::accentAmount, 1 }, "Accent",
            Range (0.0f, 1.0f), 0.5f, percent()));

        layout.add (std::move (master));

        //----------------------------------------------------------------------
        // Sequenzer
        //----------------------------------------------------------------------
        auto seq = std::make_unique<juce::AudioProcessorParameterGroup> ("seq", "Sequencer", "|");

        seq->addChild (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ids::seqRun, 1 }, "Run", false));

        seq->addChild (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ids::seqHostSync, 1 }, "Host Sync", true));

        seq->addChild (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ids::seqTempo, 1 }, "Tempo",
            Range (40.0f, 240.0f, 0.1f), 120.0f, withLabel ("BPM", 1)));

        seq->addChild (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ids::seqSwing, 1 }, "Swing",
            Range (0.0f, 0.6f), 0.0f, percent()));

        layout.add (std::move (seq));

        //----------------------------------------------------------------------
        // Instrumente
        //----------------------------------------------------------------------
        for (int i = 0; i < numInstruments; ++i)
        {
            const auto name = getInstrumentName (i);
            const auto def  = getDefaults (i);
            auto group = std::make_unique<juce::AudioProcessorParameterGroup> (getInstrumentPrefix (i), name, "|");

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { paramId (i, ids::level), 1 }, name + " Level",
                Range (0.0f, 1.0f), 0.8f, percent()));

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { paramId (i, ids::pan), 1 }, name + " Pan",
                Range (-1.0f, 1.0f), def.pan, withLabel ("", 2)));

            for (const auto& suffix : getSynthParamSuffixes (i))
            {
                const auto id = juce::ParameterID { paramId (i, suffix), 1 };

                if (suffix == ids::pitch)
                    group->addChild (std::make_unique<juce::AudioParameterFloat> (
                        id, name + " Pitch", skewed (30.0f, 120.0f, 55.0f), 52.0f, withLabel ("Hz", 1)));
                else if (suffix == ids::tune)
                    group->addChild (std::make_unique<juce::AudioParameterFloat> (
                        id, name + " Tune", Range (-12.0f, 12.0f, 0.01f), 0.0f, withLabel ("st", 2)));
                else if (suffix == ids::decay)
                    group->addChild (std::make_unique<juce::AudioParameterFloat> (
                        id, name + " Decay", skewed (def.decayMin, def.decayMax, def.decayCentre), def.decayDefault, seconds()));
                else if (suffix == ids::tone)
                    group->addChild (std::make_unique<juce::AudioParameterFloat> (
                        id, name + " Tone", Range (0.0f, 1.0f), 0.5f, percent()));
                else if (suffix == ids::snappy)
                    group->addChild (std::make_unique<juce::AudioParameterFloat> (
                        id, name + " Snappy", Range (0.0f, 1.0f), 0.6f, percent()));
                else if (suffix == ids::mode)
                    group->addChild (std::make_unique<juce::AudioParameterChoice> (
                        id, name + " Mode", juce::StringArray { "Tom", "Conga" }, 0));
            }

            layout.add (std::move (group));
        }

        return layout;
    }
}
