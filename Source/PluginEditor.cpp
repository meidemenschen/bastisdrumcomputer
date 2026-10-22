#include "PluginEditor.h"

using namespace basti;

namespace
{
    constexpr int editorWidth  = 1180;
    constexpr int editorHeight = 700;
    constexpr int headerHeight = 96;
    constexpr int stripHeight  = 262;
}

//==============================================================================
BastisDrumAudioProcessorEditor::BastisDrumAudioProcessorEditor (BastisDrumAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), stepGrid (p)
{
    auto& state = processorRef.getState();

    addAndMakeVisible (logo);

    // Transport
    for (auto* b : { &runButton, &syncButton })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }

    runAttachment  = std::make_unique<ButtonAttachment> (state, ids::seqRun, runButton);
    syncAttachment = std::make_unique<ButtonAttachment> (state, ids::seqHostSync, syncButton);

    runButton.setTooltip ("Interner Sequenzer-Start (wenn die DAW nicht spielt oder im Standalone-Modus)");
    syncButton.setTooltip ("Sequenzer folgt Tempo und Transport der DAW");

    clearButton.onClick = [this]
    {
        processorRef.getPattern().clear();
        stepGrid.repaint();
    };
    addAndMakeVisible (clearButton);

    tempoKnob  = std::make_unique<ui::LabelledKnob> (state, ids::seqTempo,     "TEMPO",  ui::colours::accent);
    swingKnob  = std::make_unique<ui::LabelledKnob> (state, ids::seqSwing,     "SWING",  ui::colours::accent);
    accentKnob = std::make_unique<ui::LabelledKnob> (state, ids::accentAmount, "ACCENT", ui::colours::accentRow);
    masterKnob = std::make_unique<ui::LabelledKnob> (state, ids::masterVolume, "MASTER", ui::colours::text);

    for (auto* k : { tempoKnob.get(), swingKnob.get(), accentKnob.get(), masterKnob.get() })
        addAndMakeVisible (*k);

    // Kanalzuege
    for (int i = 0; i < numInstruments; ++i)
        addAndMakeVisible (strips.add (new ui::InstrumentStrip (state, i)));

    addAndMakeVisible (stepGrid);

    // erst nach dem Anlegen aller Kinder setzen, damit alle das LookAndFeel uebernehmen
    setLookAndFeel (&lookAndFeel);
    setSize (editorWidth, editorHeight);
}

BastisDrumAudioProcessorEditor::~BastisDrumAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void BastisDrumAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (ui::colours::background.brighter (0.06f), 0.0f, 0.0f,
                                             ui::colours::background.darker (0.3f), 0.0f, bounds.getBottom(), false));
    g.fillAll();

    // Kopfleiste
    auto header = bounds.withHeight ((float) headerHeight);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (header);
    g.setColour (ui::colours::outline);
    g.drawHorizontalLine (headerHeight - 1, 0.0f, bounds.getWidth());

    g.setColour (ui::colours::textDim);
    g.setFont (ui::DrumLookAndFeel::uiFont (11.0f, true));
    g.drawText ("ANALOG-STYLE DRUM SYNTHESIZER", 28, 74, 400, 16, juce::Justification::centredLeft);

    // Rahmen um den Sequenzer
    auto seqArea = getLocalBounds().withTrimmedTop (headerHeight + stripHeight + 8).reduced (12, 6).toFloat();
    g.setColour (ui::colours::panel.withAlpha (0.7f));
    g.fillRoundedRectangle (seqArea, 8.0f);
    g.setColour (ui::colours::outline);
    g.drawRoundedRectangle (seqArea, 8.0f, 1.0f);
}

void BastisDrumAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Kopfleiste: Logo links, Transport rechts
    auto header = area.removeFromTop (headerHeight);
    logo.setBounds (header.getX() + 12, 4, 480, 70);

    auto transport = header.reduced (12, 10).withTrimmedLeft (520);

    for (auto* k : { masterKnob.get(), accentKnob.get(), swingKnob.get(), tempoKnob.get() })
        k->setBounds (transport.removeFromRight (74));

    transport.removeFromRight (10);
    auto buttons = transport.removeFromRight (300).withSizeKeepingCentre (300, 32);
    runButton.setBounds   (buttons.removeFromLeft (80));
    buttons.removeFromLeft (8);
    syncButton.setBounds  (buttons.removeFromLeft (110));
    buttons.removeFromLeft (8);
    clearButton.setBounds (buttons.removeFromLeft (80));

    // Kanalzuege
    auto stripArea = area.removeFromTop (stripHeight).reduced (10, 4);
    const auto stripW = stripArea.getWidth() / numInstruments;
    for (auto* s : strips)
        s->setBounds (stripArea.removeFromLeft (stripW));

    // Sequenzer
    stepGrid.setBounds (area.withTrimmedTop (8).reduced (24, 16));
}
