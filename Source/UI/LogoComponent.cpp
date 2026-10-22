#include "LogoComponent.h"

#if BASTI_HAS_EMBEDDED_FONT
 #include <BinaryData.h>
#endif

namespace basti::ui
{
    namespace
    {
        /** Erste eingebettete .ttf/.otf aus Resources/Fonts (siehe CMakeLists.txt). */
        juce::Typeface::Ptr loadEmbeddedScriptTypeface()
        {
           #if BASTI_HAS_EMBEDDED_FONT
            for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
            {
                const auto* name = BinaryData::namedResourceList[i];
                const juce::String original (BinaryData::getNamedResourceOriginalFilename (name));

                if (original.endsWithIgnoreCase (".ttf") || original.endsWithIgnoreCase (".otf"))
                {
                    int size = 0;
                    if (const auto* data = BinaryData::getNamedResource (name, size))
                        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
                }
            }
           #endif
            return nullptr;
        }
    }

    LogoComponent::LogoComponent()
        : text (juce::CharPointer_UTF8 ("Basti\xc2\xb4s Drum Machine"))
    {
        setInterceptsMouseClicks (false, false);
    }

    juce::Font LogoComponent::createScriptFont (float height)
    {
        static const auto embedded = loadEmbeddedScriptTypeface();

        if (embedded != nullptr)
            return juce::Font (juce::FontOptions (embedded).withHeight (height));

        // Fallback: typische System-Schreibschriften (Windows / macOS)
        static const juce::StringArray candidates { "Segoe Script", "Snell Roundhand", "Brush Script MT", "Lucida Handwriting" };
        static const auto installed = juce::Font::findAllTypefaceNames();

        for (const auto& name : candidates)
            if (installed.contains (name))
                return juce::Font (juce::FontOptions (name, height, juce::Font::plain));

        return juce::Font (juce::FontOptions (height, juce::Font::bold | juce::Font::italic));
    }

    void LogoComponent::resized()
    {
        const auto area = getLocalBounds().toFloat().reduced (6.0f, 2.0f);
        const auto font = createScriptFont (area.getHeight() * 0.9f);

        juce::GlyphArrangement glyphs;
        glyphs.addFittedText (font, text, area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                              juce::Justification::centredLeft, 1, 1.0f);

        textPath.clear();
        glyphs.createPath (textPath);
    }

    void LogoComponent::paint (juce::Graphics& g)
    {
        if (textPath.isEmpty())
            return;

        const auto b = textPath.getBounds();

        // 1) weicher Schlagschatten
        juce::DropShadow (juce::Colours::black.withAlpha (0.75f), 8, { 2, 4 }).drawForPath (g, textPath);

        // 2) dunkle Kontur (gibt dem Metall "Tiefe")
        g.setColour (juce::Colour (0xff0b0d10));
        g.strokePath (textPath, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 3) Metall-Verlauf: heller Himmel oben, harte Horizontlinie, dunkler Boden mit Reflexion
        juce::ColourGradient metal (juce::Colour (0xfffbfcfd), 0.0f, b.getY(),
                                    juce::Colour (0xff8d949c), 0.0f, b.getBottom(), false);
        metal.addColour (0.30, juce::Colour (0xffd5dae0));
        metal.addColour (0.48, juce::Colour (0xff9aa1aa));
        metal.addColour (0.50, juce::Colour (0xff5d646d));
        metal.addColour (0.56, juce::Colour (0xffb9c0c8));
        metal.addColour (0.80, juce::Colour (0xffeef1f4));
        g.setGradientFill (metal);
        g.fillPath (textPath);

        // 4) feine diagonale "Brushed"-Glanzlichter
        {
            juce::Graphics::ScopedSaveState save (g);
            g.reduceClipRegion (textPath);

            juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.0f), b.getX(), b.getY(),
                                        juce::Colours::white.withAlpha (0.0f), b.getRight(), b.getBottom(), false);
            sheen.addColour (0.42, juce::Colours::white.withAlpha (0.0f));
            sheen.addColour (0.50, juce::Colours::white.withAlpha (0.35f));
            sheen.addColour (0.58, juce::Colours::white.withAlpha (0.0f));
            g.setGradientFill (sheen);
            g.fillRect (b);
        }

        // 5) Glanzkante oben links
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.strokePath (textPath, juce::PathStrokeType (0.7f), juce::AffineTransform::translation (-0.4f, -0.6f));
    }
}
