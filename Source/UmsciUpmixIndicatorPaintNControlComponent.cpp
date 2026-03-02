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
    auto labelColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    auto font = juce::Font(juce::FontOptions(m_subCircleRadius * 1.1f, juce::Font::plain));
    g.setFont(font);

    auto opacity = (isTimerRunning() && !m_flashState) ? 0.25f : 1.0f;

    // draw floor channel ring
    g.setColour(indicatorColour);
    g.setOpacity(opacity);
    g.fillPath(m_upmixIndicator);

    g.setColour(labelColour);
    g.setOpacity(opacity);
    for (const auto& rcp : m_renderedFloorPositions)
    {
        auto labelBounds = juce::Rectangle<float>(m_subCircleRadius * 2.0f, m_subCircleRadius * 2.0f)
                               .withCentre(rcp.screenPos);
        g.drawFittedText(rcp.label, labelBounds.toNearestInt(), juce::Justification::centred, 1);
    }

    // draw height channel ring
    g.setColour(indicatorColour);
    g.setOpacity(opacity);
    g.fillPath(m_upmixHeightIndicator);

    g.setColour(labelColour);
    g.setOpacity(opacity);
    for (const auto& rcp : m_renderedHeightPositions)
    {
        auto labelBounds = juce::Rectangle<float>(m_subCircleRadius * 2.0f, m_subCircleRadius * 2.0f)
                               .withCentre(rcp.screenPos);
        g.drawFittedText(rcp.label, labelBounds.toNearestInt(), juce::Justification::centred, 1);
    }

    // draw hint text while flashing
    if (isTimerRunning())
    {
        g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::plain)));
        g.setColour(indicatorColour);
        g.setOpacity(1.0f);
        g.drawFittedText(
            "Double-click the upmix indicator to change sound object positions to match it. "
            "Double-click anywhere else to reset the upmix indicator to default position/rotation.",
            getLocalBounds().reduced(getLocalBounds().getWidth() / 5),
            juce::Justification::centred,
            4);
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

void UmsciUpmixIndicatorPaintNControlComponent::setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position)
{
    m_sourcePositions[sourceId] = position;
    updateFlashState();
}

bool UmsciUpmixIndicatorPaintNControlComponent::hitTest(int x, int y)
{
    if (isTimerRunning())
        return true; // capture all clicks in the component area while hint is visible
    return m_upmixIndicator.contains(float(x), float(y))
        || m_upmixHeightIndicator.contains(float(x), float(y));
}

void UmsciUpmixIndicatorPaintNControlComponent::mouseDown(const juce::MouseEvent& e)
{
    auto dx = e.position.x - m_upmixCenter.x;
    auto dy = e.position.y - m_upmixCenter.y;

    // atan2(dx, -dy): 0 = 12 o'clock, clockwise positive — matches JUCE arc angle convention
    m_dragStartAngle      = std::atan2(dx, -dy);
    m_dragStartDist       = std::sqrt(dx * dx + dy * dy);
    m_dragStartRot        = m_upmixRot;
    m_dragStartTrans      = m_upmixTrans;
    m_dragStartHeightTrans = m_upmixHeightTrans;

    // determine whether the drag targets the height ring or the floor ring
    m_draggingHeightRing = m_upmixHeightIndicator.contains(e.position.x, e.position.y);

    DBG(juce::String(__FUNCTION__) << " rot:" << m_upmixRot
        << " trans:" << m_upmixTrans << " heightTrans:" << m_upmixHeightTrans
        << " heightRing:" << (int)m_draggingHeightRing);
}

void UmsciUpmixIndicatorPaintNControlComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto dx = e.position.x - m_upmixCenter.x;
    auto dy = e.position.y - m_upmixCenter.y;

    // tangential component: angle delta drives rotation — shared by both rings
    m_upmixRot = m_dragStartRot + std::atan2(dx, -dy) - m_dragStartAngle;

    // radial component: distance ratio drives the scale of whichever ring was grabbed
    if (m_dragStartDist > 0.0f)
    {
        auto scaleFactor = std::sqrt(dx * dx + dy * dy) / m_dragStartDist;
        if (m_draggingHeightRing)
            m_upmixHeightTrans = juce::jlimit(0.1f, 10.0f, m_dragStartHeightTrans * scaleFactor);
        else
            m_upmixTrans = juce::jlimit(0.1f, 10.0f, m_dragStartTrans * scaleFactor);
    }

    DBG(juce::String(__FUNCTION__) << " rot:" << m_upmixRot
        << " trans:" << m_upmixTrans << " heightTrans:" << m_upmixHeightTrans);

    PrerenderUpmixIndicatorInBounds();
    repaint();
}

void UmsciUpmixIndicatorPaintNControlComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (m_upmixIndicator.contains(e.position.x, e.position.y)
     || m_upmixHeightIndicator.contains(e.position.x, e.position.y))
    {
        // snap all source positions to the current ring positions
        for (auto const& rcp : m_renderedFloorPositions)
        {
            m_sourcePositions[rcp.sourceId] = rcp.realPos;
            if (onSourcePositionChanged)
                onSourcePositionChanged(rcp.sourceId, rcp.realPos);
        }
        for (auto const& rcp : m_renderedHeightPositions)
        {
            m_sourcePositions[rcp.sourceId] = rcp.realPos;
            if (onSourcePositionChanged)
                onSourcePositionChanged(rcp.sourceId, rcp.realPos);
        }
        updateFlashState();
    }
    else
    {
        // reset indicator to default rotation and scale
        m_upmixRot         = 0.0f;
        m_upmixTrans       = 1.0f;
        m_upmixHeightTrans = 0.6f;
        PrerenderUpmixIndicatorInBounds(); // calls updateFlashState() internally
        repaint();
    }
}

void UmsciUpmixIndicatorPaintNControlComponent::timerCallback()
{
    m_flashState = !m_flashState;
    repaint();
}

void UmsciUpmixIndicatorPaintNControlComponent::PrerenderUpmixIndicatorInBounds()
{
    m_upmixIndicator.clear();
    m_upmixHeightIndicator.clear();
    m_renderedFloorPositions.clear();
    m_renderedHeightPositions.clear();

    auto speakersRealBoundingTopLeft = std::array<float, 3>{ m_speakersRealBoundingCube.at(0), m_speakersRealBoundingCube.at(1), m_speakersRealBoundingCube.at(2) };
    auto speakersRealBoundingBottomRight = std::array<float, 3>{ m_speakersRealBoundingCube.at(3), m_speakersRealBoundingCube.at(4), m_speakersRealBoundingCube.at(5) };
    auto speakersScreenBoundingRect = juce::Rectangle<float>(GetPointForRealCoordinate(speakersRealBoundingTopLeft), GetPointForRealCoordinate(speakersRealBoundingBottomRight));
    auto upmixIndicatorBounds = speakersScreenBoundingRect.getAspectRatio() <= 1 ? speakersScreenBoundingRect.expanded(speakersScreenBoundingRect.getWidth() * 0.15f) : speakersScreenBoundingRect.expanded(speakersScreenBoundingRect.getHeight() * 0.15f);

    std::vector<float> upmixPositionAnglesDeg;
    std::vector<std::string> upmixPositionNames;
    std::vector<juce::AudioChannelSet::ChannelType> upmixPositionChannelTypes;
    std::vector<float> upmixHeightPositionAnglesDeg;
    std::vector<std::string> upmixHeightPositionNames;
    std::vector<juce::AudioChannelSet::ChannelType> upmixHeightPositionChannelTypes;
    for (auto const& channelType : m_channelConfiguration.getChannelTypes())
    {
        if (m_clockwiseOrderedChannelTypes.contains(channelType))
        {
            upmixPositionAnglesDeg.push_back(getAngleForChannelTypeInCurrentConfiguration(channelType));
            upmixPositionNames.push_back(juce::AudioChannelSet::getAbbreviatedChannelTypeName(channelType).toStdString());
            upmixPositionChannelTypes.push_back(channelType);
        }
        else if (m_clockwiseOrderedHeightChannelTypes.contains(channelType))
        {
            upmixHeightPositionAnglesDeg.push_back(getAngleForChannelTypeInCurrentConfiguration(channelType));
            upmixHeightPositionNames.push_back(juce::AudioChannelSet::getAbbreviatedChannelTypeName(channelType).toStdString());
            upmixHeightPositionChannelTypes.push_back(channelType);
        }
    }

    if (upmixIndicatorBounds.isEmpty())
        return;

    m_upmixCenter = upmixIndicatorBounds.getCentre();
    auto cx = m_upmixCenter.x;
    auto cy = m_upmixCenter.y;
    auto baseRadius = std::min(upmixIndicatorBounds.getWidth(), upmixIndicatorBounds.getHeight()) * 0.5f;
    auto radius = baseRadius * m_upmixTrans;
    m_subCircleRadius = baseRadius * 0.05f;
    auto arcStrokeWidth = baseRadius * 0.025f;

    // build the floor channel ring
    if (!upmixPositionAnglesDeg.empty())
    {
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

        // add a filled sub-circle at each position and record its data for paint() and hit-testing
        // screen coords for arc angle θ with rotation R: x = cx + r·sin(θ+R), y = cy - r·cos(θ+R)
        for (size_t i = 0; i < upmixPositionAnglesDeg.size(); ++i)
        {
            auto effectiveAngleRad = juce::degreesToRadians(upmixPositionAnglesDeg[i]) + m_upmixRot;
            auto px = cx + radius * std::sin(effectiveAngleRad);
            auto py = cy - radius * std::cos(effectiveAngleRad);

            m_upmixIndicator.addEllipse(px - m_subCircleRadius, py - m_subCircleRadius,
                                        m_subCircleRadius * 2.0f, m_subCircleRadius * 2.0f);

            if (i < upmixPositionNames.size())
            {
                RenderedChannelPosition rcp;
                rcp.sourceId  = static_cast<std::int16_t>(
                    m_sourceStartId + getChannelNumberForChannelTypeInCurrentConfiguration(upmixPositionChannelTypes[i]) - 1);
                rcp.screenPos = juce::Point<float>(px, py);
                rcp.realPos   = GetRealCoordinateForPoint(rcp.screenPos);
                rcp.label     = juce::String(upmixPositionNames[i]);
                m_renderedFloorPositions.push_back(rcp);
            }
        }
    }

    // build the height channel ring — same rotation, independent scale
    if (!upmixHeightPositionAnglesDeg.empty())
    {
        auto heightRadius = baseRadius * m_upmixHeightTrans;
        auto heightArcStrokeWidth = arcStrokeWidth;

        auto minHeightAngleDeg = *std::min_element(upmixHeightPositionAnglesDeg.begin(), upmixHeightPositionAnglesDeg.end());
        auto maxHeightAngleDeg = *std::max_element(upmixHeightPositionAnglesDeg.begin(), upmixHeightPositionAnglesDeg.end());

        juce::Path heightArcPath;
        heightArcPath.addCentredArc(cx, cy, heightRadius, heightRadius, m_upmixRot,
                                    juce::degreesToRadians(minHeightAngleDeg),
                                    juce::degreesToRadians(maxHeightAngleDeg),
                                    true);
        juce::PathStrokeType(heightArcStrokeWidth).createStrokedPath(m_upmixHeightIndicator, heightArcPath);

        for (size_t i = 0; i < upmixHeightPositionAnglesDeg.size(); ++i)
        {
            auto effectiveAngleRad = juce::degreesToRadians(upmixHeightPositionAnglesDeg[i]) + m_upmixRot;
            auto px = cx + heightRadius * std::sin(effectiveAngleRad);
            auto py = cy - heightRadius * std::cos(effectiveAngleRad);

            m_upmixHeightIndicator.addEllipse(px - m_subCircleRadius, py - m_subCircleRadius,
                                              m_subCircleRadius * 2.0f, m_subCircleRadius * 2.0f);

            if (i < upmixHeightPositionNames.size())
            {
                RenderedChannelPosition rcp;
                rcp.sourceId  = static_cast<std::int16_t>(
                    m_sourceStartId + getChannelNumberForChannelTypeInCurrentConfiguration(upmixHeightPositionChannelTypes[i]) - 1);
                rcp.screenPos = juce::Point<float>(px, py);
                rcp.realPos   = GetRealCoordinateForPoint(rcp.screenPos);
                rcp.label     = juce::String(upmixHeightPositionNames[i]);
                m_renderedHeightPositions.push_back(rcp);
            }
        }
    }

    updateFlashState();
}

void UmsciUpmixIndicatorPaintNControlComponent::setSourceStartId(int startId)
{
    m_sourceStartId = juce::jmax(1, startId);
    PrerenderUpmixIndicatorInBounds();
    repaint();
}

int UmsciUpmixIndicatorPaintNControlComponent::getSourceStartId() const
{
    return m_sourceStartId;
}

bool UmsciUpmixIndicatorPaintNControlComponent::setChannelConfiguration(const juce::AudioChannelSet& channelLayout)
{
    auto rVal = TwoDFieldBase::setChannelConfiguration(channelLayout);

    PrerenderUpmixIndicatorInBounds();

    repaint();

    return rVal;
}

void UmsciUpmixIndicatorPaintNControlComponent::updateFlashState()
{
    auto const tolerance = 0.01f;
    bool mismatch = false;

    auto checkPos = [&](const RenderedChannelPosition& rcp)
    {
        auto it = m_sourcePositions.find(rcp.sourceId);
        if (it == m_sourcePositions.end())
        {
            mismatch = true;
            return;
        }
        auto const& sp = it->second;
        if (std::abs(sp.at(0) - rcp.realPos.at(0)) > tolerance
         || std::abs(sp.at(1) - rcp.realPos.at(1)) > tolerance
         || std::abs(sp.at(2) - rcp.realPos.at(2)) > tolerance)
            mismatch = true;
    };

    for (auto const& rcp : m_renderedFloorPositions)
        checkPos(rcp);
    for (auto const& rcp : m_renderedHeightPositions)
        checkPos(rcp);

    if (mismatch)
    {
        if (!isTimerRunning())
            startTimer(500); // 2 Hz
    }
    else
    {
        stopTimer();
        m_flashState = false;
        repaint();
    }
}

