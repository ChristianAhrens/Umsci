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
    auto knobSize = 14.0f * getControlsSizeMultiplier();
    auto knobThickness = 4.0f * getControlsSizeMultiplier();

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

    // Paint crosshair for the currently dragged source
    if (m_draggedSourceId != -1 && m_sourceScreenPositions.count(m_draggedSourceId))
    {
        auto dragPos = m_sourceScreenPositions.at(m_draggedSourceId).toFloat();
        g.drawLine(0.0f, dragPos.getY(), float(getWidth()), dragPos.getY(), 1.0f);
        g.drawLine(dragPos.getX(), 0.0f, dragPos.getX(), float(getHeight()), 1.0f);
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

bool UmsciSoundobjectsPaintComponent::hitTest(int x, int y)
{
    auto const hitRadius = 14.0f * getControlsSizeMultiplier(); // matches mouseDown
    auto point = juce::Point<int>(x, y);
    for (auto const& kv : m_sourceScreenPositions)
        if (point.getDistanceFrom(kv.second) <= hitRadius)
            return true;
    return false;
}

void UmsciSoundobjectsPaintComponent::setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position)
{
    m_sourcePositions[sourceId] = position;
    m_sourceScreenPositions[sourceId] = GetPointForRealCoordinate({ position.at(0), position.at(1), position.at(2) }).toInt();
    repaint();
}

void UmsciSoundobjectsPaintComponent::mouseDown(const juce::MouseEvent& e)
{
    auto const hitRadius = 14.0f * getControlsSizeMultiplier(); // matches knobSize in paint()
    auto clickPos = e.getPosition();

    m_draggedSourceId = -1;
    for (auto const& kv : m_sourceScreenPositions)
    {
        if (clickPos.getDistanceFrom(kv.second) <= hitRadius)
        {
            m_draggedSourceId = kv.first;
            break;
        }
    }
}

void UmsciSoundobjectsPaintComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (m_draggedSourceId == -1)
        return;

    auto dragPos = e.getPosition().toFloat();
    dragPos.setX(juce::jlimit(0.0f, float(getWidth()),  dragPos.getX()));
    dragPos.setY(juce::jlimit(0.0f, float(getHeight()), dragPos.getY()));

    m_sourceScreenPositions[m_draggedSourceId] = dragPos.toInt();

    auto realCoord = GetRealCoordinateForPoint(dragPos);
    m_sourcePositions[m_draggedSourceId] = realCoord;

    if (onSourcePositionChanged)
        onSourcePositionChanged(m_draggedSourceId, realCoord);

    repaint();
}

void UmsciSoundobjectsPaintComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (m_draggedSourceId != -1)
    {
        m_draggedSourceId = -1;
        repaint();
    }
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

