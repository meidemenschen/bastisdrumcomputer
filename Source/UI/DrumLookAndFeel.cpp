#include "DrumLookAndFeel.h"

namespace basti::ui
{
    juce::Colour colours::forInstrument (int instrument)
    {
        static const juce::Colour palette[] = {
            juce::Colour (0xff38d1c4),   // Kick
            juce::Colour (0xff3fc0e0),   // Snare
            juce::Colour (0xff4aa8f0),   // Clap
            juce::Colour (0xff5b93ff),   // Closed Hat
            juce::Colour (0xff6f84ff),   // Open Hat
            juce::Colour (0xff8478f5),   // Low Tom
            juce::Colour (0xff946ff0),   // Mid Tom
            juce::Colour (0xffa468ea),   // High Tom
            juce::Colour (0xffb48cff),   // Accent
        };

        return palette[juce::jlimit (0, (int) std::size (palette) - 1, instrument)];
    }

    //==========================================================================
    DrumLookAndFeel::DrumLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, colours::background);

        setColour (juce::Slider::textBoxTextColourId,       colours::textDim);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId,  colours::accent.withAlpha (0.4f));
        setColour (juce::Slider::rotarySliderFillColourId,  colours::accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, colours::outline);

        setColour (juce::Label::textColourId, colours::text);

        setColour (juce::TextButton::buttonColourId,   colours::panelLight);
        setColour (juce::TextButton::buttonOnColourId, colours::accent);
        setColour (juce::TextButton::textColourOffId,  colours::text);
        setColour (juce::TextButton::textColourOnId,   colours::background);

        setColour (juce::ComboBox::backgroundColourId, colours::panelLight);
        setColour (juce::ComboBox::outlineColourId,    colours::outline);
        setColour (juce::ComboBox::textColourId,       colours::text);
        setColour (juce::ComboBox::arrowColourId,      colours::textDim);

        setColour (juce::PopupMenu::backgroundColourId,            colours::panel);
        setColour (juce::PopupMenu::textColourId,                  colours::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.3f));
        setColour (juce::PopupMenu::highlightedTextColourId,       colours::text);
    }

    juce::Font DrumLookAndFeel::uiFont (float height, bool bold)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }

    //==========================================================================
    void DrumLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float pos, float startAngle, float endAngle, juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto angle  = startAngle + pos * (endAngle - startAngle);
        const auto fill   = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const auto track  = radius * 0.14f;

        // Hintergrund-Bogen
        juce::Path bg;
        bg.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, endAngle, true);
        g.setColour (colours::outline);
        g.strokePath (bg, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Wert-Bogen (bei bipolaren Reglern von der Mitte aus)
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        const auto from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

        juce::Path value;
        value.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f,
                             juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (slider.isEnabled() ? fill : fill.withSaturation (0.0f));
        g.strokePath (value, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Knopf
        const auto knobR = radius * 0.62f;
        const auto knob  = juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (centre);
        g.setGradientFill (juce::ColourGradient (colours::panelLight.brighter (0.25f), knob.getX(), knob.getY(),
                                                 colours::panel.darker (0.3f), knob.getRight(), knob.getBottom(), false));
        g.fillEllipse (knob);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (knob, 1.0f);

        // Zeiger
        juce::Path pointer;
        pointer.addRoundedRectangle (-1.25f, -knobR, 2.5f, knobR * 0.55f, 1.0f);
        g.setColour (colours::text);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
    }

    void DrumLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& bgColour,
                                                bool isMouseOver, bool isDown)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto colour = button.getToggleState() ? button.findColour (juce::TextButton::buttonOnColourId) : bgColour;

        if (isDown)          colour = colour.darker (0.2f);
        else if (isMouseOver) colour = colour.brighter (0.08f);

        g.setColour (colour);
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (colours::outline);
        g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
    }

    void DrumLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
    {
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                                : juce::TextButton::textColourOffId));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 2), juce::Justification::centred, 1);
    }

    juce::Label* DrumLookAndFeel::createSliderTextBox (juce::Slider& slider)
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (slider);
        l->setFont (uiFont (11.0f));
        l->setColour (juce::Label::outlineColourId,           juce::Colours::transparentBlack);
        l->setColour (juce::Label::backgroundColourId,        juce::Colours::transparentBlack);
        l->setColour (juce::Label::textColourId,              colours::textDim);
        l->setColour (juce::Label::outlineWhenEditingColourId, colours::accent);
        l->setColour (juce::TextEditor::backgroundColourId,   colours::panelLight);
        return l;
    }

    juce::Font DrumLookAndFeel::getComboBoxFont (juce::ComboBox&)                 { return uiFont (13.0f); }
    juce::Font DrumLookAndFeel::getPopupMenuFont()                                { return uiFont (14.0f); }
    juce::Font DrumLookAndFeel::getTextButtonFont (juce::TextButton&, int height) { return uiFont (juce::jmin (14.0f, (float) height * 0.5f), true); }
}
