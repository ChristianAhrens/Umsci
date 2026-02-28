/* Copyright (c) 2026, Christian Ahrens
 *
 * This file is part of Umsci <https://github.com/ChristianAhrens/Umsci>
 *
 * This tool is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This tool is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this tool; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "UmsciSoundobjectsPaintComponent.h"

#include <CustomLookAndFeel.h>


UmsciSoundobjectsPaintComponent::UmsciSoundobjectsPaintComponent()
    : UmsciPaintNControlComponentBase()
{
}

UmsciSoundobjectsPaintComponent::~UmsciSoundobjectsPaintComponent()
{
}

void UmsciSoundobjectsPaintComponent::paint(juce::Graphics &g)
{
    auto knobSize = 14.0f;
    auto knobThickness = 4.0f;

    g.setColour(getLookAndFeel().findColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringRmsColourId));
    g.setOpacity(1.0f);

    auto font = juce::Font(juce::FontOptions(knobSize, juce::Font::plain));
    g.setFont(font);

    for (auto const sourceScreenPositionKV : m_sourceScreenPositions)
    {
        auto& sourceId = sourceScreenPositionKV.first;
        auto sourceScreenPos = sourceScreenPositionKV.second.toFloat();

        // Paint source thumb
        g.drawEllipse(juce::Rectangle<float>(sourceScreenPos.getX() - (knobSize / 2.0f), sourceScreenPos.getY() - (knobSize / 2.0f), knobSize, knobSize), knobThickness);

        // Paint source number
        juce::String textLabel = juce::String(sourceId);
        auto fontDependantWidth = float(juce::GlyphArrangement::getStringWidthInt(font, textLabel));
        g.drawText(textLabel, juce::Rectangle<float>(sourceScreenPos.getX() - (0.5f * fontDependantWidth), sourceScreenPos.getY() + 3, fontDependantWidth, knobSize * 2.0f), Justification::centred, true);
    }
}

void UmsciSoundobjectsPaintComponent::resized()
{
    PrerenderSourcesInBounds();
}

void UmsciSoundobjectsPaintComponent::setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions)
{
    if (sourcePositions.empty())
    {
        m_sourcePositions.clear();
        m_sourceScreenPositions.clear();
    }
    else
    {
        m_sourcePositions = sourcePositions;

        PrerenderSourcesInBounds();
    }

    repaint();
}

void UmsciSoundobjectsPaintComponent::PrerenderSourcesInBounds()
{
    // Speaker positions
    for (auto const sourcePositionKV : m_sourcePositions)
    {
        auto& sourceId = sourcePositionKV.first;
        auto& spourcePos = sourcePositionKV.second;
        auto& x = spourcePos.at(0);
        auto& y = spourcePos.at(1);
        auto& z = spourcePos.at(2);
        m_sourceScreenPositions[sourceId] = GetPointForRealCoordinate({ x, y, z }).toInt();
    }
}

