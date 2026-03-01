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

#include "UmsciUpmixIndicatorPaintNControlComponent.h"

#include <CustomLookAndFeel.h>


UmsciUpmixIndicatorPaintNControlComponent::UmsciUpmixIndicatorPaintNControlComponent()
    : UmsciPaintNControlComponentBase(), JUCEAppBasics::TwoDFieldBase()
{
    setChannelConfiguration(juce::AudioChannelSet::create7point1point4());
}

UmsciUpmixIndicatorPaintNControlComponent::~UmsciUpmixIndicatorPaintNControlComponent()
{
}

void UmsciUpmixIndicatorPaintNControlComponent::paint(juce::Graphics &g)
{
    auto indicatorColour = getLookAndFeel().findColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringRmsColourId);
    g.setColour(indicatorColour);
    g.setOpacity(1.0f);
    g.fillPath(m_upmixIndicator);

    // draw position name labels centred inside each sub-circle
    auto labelColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    g.setColour(labelColour);
    auto font = juce::Font(juce::FontOptions(m_subCircleRadius * 1.1f, juce::Font::plain));
    g.setFont(font);
    for (const auto& label : m_renderedPositionLabels)
    {
        auto labelBounds = juce::Rectangle<float>(m_subCircleRadius * 2.0f, m_subCircleRadius * 2.0f)
                               .withCentre(label.first);
        g.drawFittedText(label.second, labelBounds.toNearestInt(), juce::Justification::centred, 1);
    }
}

void UmsciUpmixIndicatorPaintNControlComponent::resized()
{
    PrerenderUpmixIndicatorInBounds();
}

void UmsciUpmixIndicatorPaintNControlComponent::setSpeakersRealBoundingCube(const std::array<float, 6>& speakersRealBoundingCube)
{
    m_speakersRealBoundingCube = speakersRealBoundingCube;

    PrerenderUpmixIndicatorInBounds();
}

void UmsciUpmixIndicatorPaintNControlComponent::setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions)
{
    m_sourcePositions = sourcePositions;

    PrerenderUpmixIndicatorInBounds();
}

void UmsciUpmixIndicatorPaintNControlComponent::mouseDown(const juce::MouseEvent& e)
{
    auto dx = e.position.x - m_upmixCenter.x;
    auto dy = e.position.y - m_upmixCenter.y;

    // atan2(dx, -dy): 0 = 12 o'clock, clockwise positive — matches JUCE arc angle convention
    m_dragStartAngle = std::atan2(dx, -dy);
    m_dragStartDist  = std::sqrt(dx * dx + dy * dy);
    m_dragStartRot   = m_upmixRot;
    m_dragStartTrans = m_upmixTrans;

    DBG(juce::String(__FUNCTION__) << " rot:" << m_upmixRot << " trans:" << m_upmixTrans);
}

void UmsciUpmixIndicatorPaintNControlComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto dx = e.position.x - m_upmixCenter.x;
    auto dy = e.position.y - m_upmixCenter.y;

    // tangential component: angle delta drives rotation
    m_upmixRot = m_dragStartRot + std::atan2(dx, -dy) - m_dragStartAngle;

    // radial component: distance ratio drives scale
    if (m_dragStartDist > 0.0f)
        m_upmixTrans = juce::jlimit(0.1f, 10.0f, m_dragStartTrans * (std::sqrt(dx * dx + dy * dy) / m_dragStartDist));

    DBG(juce::String(__FUNCTION__) << " rot:" << m_upmixRot << " trans:" << m_upmixTrans);

    PrerenderUpmixIndicatorInBounds();
    repaint();
}

void UmsciUpmixIndicatorPaintNControlComponent::PrerenderUpmixIndicatorInBounds()
{
    m_upmixIndicator.clear();
    m_renderedPositionLabels.clear();

    auto speakersRealBoundingTopLeft = std::array<float, 3>{ m_speakersRealBoundingCube.at(0), m_speakersRealBoundingCube.at(1), m_speakersRealBoundingCube.at(2) };
    auto speakersRealBoundingBottomRight = std::array<float, 3>{ m_speakersRealBoundingCube.at(3), m_speakersRealBoundingCube.at(4), m_speakersRealBoundingCube.at(5) };
    auto speakersScreenBoundingRect = juce::Rectangle<float>(GetPointForRealCoordinate(speakersRealBoundingTopLeft), GetPointForRealCoordinate(speakersRealBoundingBottomRight));
    auto upmixIndicatorBounds = speakersScreenBoundingRect.getAspectRatio() <= 1 ? speakersScreenBoundingRect.expanded(speakersScreenBoundingRect.getWidth() * 0.15f) : speakersScreenBoundingRect.expanded(speakersScreenBoundingRect.getHeight() * 0.15f);

    std::vector<float> upmixPositionAnglesDeg(m_clockwiseOrderedChannelTypes.size());
    std::vector<std::string> upmixPositionNames(m_clockwiseOrderedChannelTypes.size());
    std::vector<float> upmixHeightPositionAnglesDeg(m_clockwiseOrderedHeightChannelTypes.size());
    std::vector<std::string> upmixHeightPositionNames(m_clockwiseOrderedHeightChannelTypes.size());
    for (auto const& channelType : m_channelConfiguration.getChannelTypes())
    {
        if (m_clockwiseOrderedChannelTypes.contains(channelType))
        {
            upmixPositionAnglesDeg.push_back(getAngleForChannelTypeInCurrentConfiguration(channelType));
            upmixPositionNames.push_back(juce::AudioChannelSet::getAbbreviatedChannelTypeName(channelType).toStdString());
        }
        else if (m_clockwiseOrderedHeightChannelTypes.contains(channelType))
        {
            upmixHeightPositionAnglesDeg.push_back(getAngleForChannelTypeInCurrentConfiguration(channelType));
            upmixHeightPositionNames.push_back(juce::AudioChannelSet::getAbbreviatedChannelTypeName(channelType).toStdString());
        }
    }

    if (upmixIndicatorBounds.isEmpty())
        return;

    m_upmixCenter = upmixIndicatorBounds.getCentre();
    auto cx = m_upmixCenter.x;
    auto cy = m_upmixCenter.y;
    auto baseRadius = std::min(upmixIndicatorBounds.getWidth(), upmixIndicatorBounds.getHeight()) * 0.5f;
    auto radius = baseRadius * m_upmixTrans;
    m_subCircleRadius = baseRadius * 0.06f;
    auto arcStrokeWidth = baseRadius * 0.025f;

    // determine the angular extent of the arc from the most extreme position angles
    auto minAngleDeg = *std::min_element(upmixPositionAnglesDeg.begin(), upmixPositionAnglesDeg.end());
    auto maxAngleDeg = *std::max_element(upmixPositionAnglesDeg.begin(), upmixPositionAnglesDeg.end());

    // build the arc segment and stroke it into a filled band in m_upmixIndicator
    // angles follow JUCE convention: 0 = 12 o'clock, clockwise positive — matching standard audio azimuth
    juce::Path arcPath;
    arcPath.addCentredArc(cx, cy, radius, radius, m_upmixRot,
                          juce::degreesToRadians(minAngleDeg),
                          juce::degreesToRadians(maxAngleDeg),
                          true);
    juce::PathStrokeType(arcStrokeWidth).createStrokedPath(m_upmixIndicator, arcPath);

    // add a filled sub-circle at each position and record its label for paint()
    // screen coords for arc angle θ with rotation R: x = cx + r·sin(θ+R), y = cy - r·cos(θ+R)
    for (size_t i = 0; i < upmixPositionAnglesDeg.size(); ++i)
    {
        auto effectiveAngleRad = juce::degreesToRadians(upmixPositionAnglesDeg[i]) + m_upmixRot;
        auto px = cx + radius * std::sin(effectiveAngleRad);
        auto py = cy - radius * std::cos(effectiveAngleRad);

        m_upmixIndicator.addEllipse(px - m_subCircleRadius, py - m_subCircleRadius,
                                    m_subCircleRadius * 2.0f, m_subCircleRadius * 2.0f);

        if (i < upmixPositionNames.size())
            m_renderedPositionLabels.push_back({ juce::Point<float>(px, py), juce::String(upmixPositionNames[i]) });
    }
}

