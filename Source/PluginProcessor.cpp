#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BastisDrumAudioProcessor::BastisDrumAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", basti::createParameterLayout()),
      engine (apvts),
      sequencer (apvts, pattern)
{
    accentParam = apvts.getRawParameterValue (basti::ids::accentAmount);
}

//==============================================================================
void BastisDrumAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    sequencer.prepare (sampleRate);
}

void BastisDrumAudioProcessor::releaseResources()
{
    engine.reset();
}

bool BastisDrumAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

//==============================================================================
void BastisDrumAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();

    engine.beginBlock();

    // Vorhoeren aus der GUI
    if (auto bits = pendingAuditions.exchange (0); bits != 0)
        for (int i = 0; i < basti::numInstruments; ++i)
            if ((bits >> i) & 1u)
                engine.addTrigger ({ 0, i, 0.8f, 0 });

    handleMidi (midi);
    sequencer.process (getPlayHead(), numSamples, engine);

    engine.render (buffer, numSamples);
}

void BastisDrumAudioProcessor::handleMidi (const juce::MidiBuffer& midi)
{
    const auto accent = accentParam->load();

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();

        if (! msg.isNoteOn())
            continue;

        int variant = 0;
        const auto instrument = basti::DrumEngine::instrumentForMidiNote (msg.getNoteNumber(), variant);

        if (instrument < 0)
            continue;

        // Velocity 1..119 = normale Dynamik, ab 120 zusaetzlich Accent
        auto velocity = 0.8f * msg.getFloatVelocity();
        if (msg.getVelocity() >= 120)
            velocity *= 1.0f + accent;

        engine.addTrigger ({ metadata.samplePosition, instrument, velocity, variant });
    }
}

void BastisDrumAudioProcessor::auditionInstrument (int instrument) noexcept
{
    if (juce::isPositiveAndBelow (instrument, basti::numInstruments))
        pendingAuditions.fetch_or (1u << instrument);
}

//==============================================================================
juce::AudioProcessorEditor* BastisDrumAudioProcessor::createEditor()
{
    return new BastisDrumAudioProcessorEditor (*this);
}

//==============================================================================
void BastisDrumAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    state.removeChild (state.getChildWithName (basti::PatternData::treeType), nullptr);
    state.appendChild (pattern.toValueTree(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BastisDrumAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (! xml->hasTagName (apvts.state.getType()))
            return;

        auto state = juce::ValueTree::fromXml (*xml);
        const auto patternTree = state.getChildWithName (basti::PatternData::treeType);

        if (patternTree.isValid())
        {
            pattern.fromValueTree (patternTree);
            state.removeChild (patternTree, nullptr);
        }

        apvts.replaceState (state);
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BastisDrumAudioProcessor();
}
