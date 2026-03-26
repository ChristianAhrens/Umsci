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

    // draw height annotation in lower-right corner
    if (!m_renderedFloorPositions.empty())
    {
        g.setFont(juce::Font(juce::FontOptions(12.0f * getControlsSizeMultiplier(), juce::Font::plain)));
        g.setColour(indicatorColour);
        g.setOpacity(opacity);

        auto bounds = getLocalBounds().toFloat();
        auto lineHeight = 16.0f * getControlsSizeMultiplier();
        auto annotationWidth = bounds.getWidth() * 0.25f;
        auto margin = 4.0f;

        if (!m_renderedHeightPositions.empty())
        {
            auto heightLine = juce::Rectangle<float>(annotationWidth, lineHeight)
                .withBottomY(bounds.getBottom() - margin)
                .withRightX(bounds.getRight() - margin);
            auto effectiveHeightZ = m_speakersRealBoundingCube[5]
                + (m_speakersRealBoundingCube[5] - m_speakersRealBoundingCube[2]) * m_boundingFitFactor;
            g.drawFittedText(juce::String("Height: ") + juce::String(effectiveHeightZ, 2) + juce::String(" m"),
                             heightLine.toNearestInt(), juce::Justification::bottomRight, 1);

            auto floorLine = heightLine.withBottomY(heightLine.getY());
            g.drawFittedText(juce::String("Normal: 1.20 m"),
                             floorLine.toNearestInt(), juce::Justification::bottomRight, 1);
        }
        else
        {
            auto floorLine = juce::Rectangle<float>(annotationWidth, lineHeight)
                .withBottomY(bounds.getBottom() - margin)
                .withRightX(bounds.getRight() - margin);
            g.drawFittedText(juce::String("Normal: 1.20 m"),
                             floorLine.toNearestInt(), juce::Justification::bottomRight, 1);
        }
    }

    // draw center position handle and angle stretch handle (paths prerendered in PrerenderUpmixIndicatorInBounds)
    g.setColour(indicatorColour);
    g.setOpacity(opacity);
    g.fillPath(m_centerHandlePath);
    g.fillPath(m_stretchHandlePath);

    // draw re-fit button in upper-right corner
    {
        auto refitBounds = getRefitButtonBounds();
        g.setColour(indicatorColour);
        g.setOpacity(1.0f);
        g.fillRect(refitBounds);
        g.setFont(juce::Font(juce::FontOptions(11.0f * std::min(getControlsSizeMultiplier(), 1.5f), juce::Font::plain)));
        g.setColour(labelColour);
        g.drawFittedText("Re-fit to\nbounding cube", refitBounds.reduced(4), juce::Justification::centred, 2);
    }

    // draw hint text while flashing
    if (isTimerRunning())
    {
        g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::plain)));
        g.setColour(indicatorColour);
        g.setOpacity(1.0f);
        auto hintText = m_liveMode
            ? juce::String("External position changes detected. Double-click the upmix indicator to apply its current positions.")
            : juce::String("Double-click the upmix indicator to change sound object positions to match it.");
        g.drawFittedText(
            hintText,
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
    if (m_inhibitFlashCount > 0)
        --m_inhibitFlashCount;
    else
        updateFlashState();
}

bool UmsciUpmixIndicatorPaintNControlComponent::hitTest(int x, int y)
{
    if (isTimerRunning())
        return true; // capture all clicks in the component area while hint is visible

    // stretch handle hit area — oriented bounding rectangle of the arrow shape
    if (!m_renderedFloorPositions.empty() && m_stretchHandleTangent != juce::Point<float>{})
    {
        float dx = float(x) - m_stretchHandlePos.x;
        float dy = float(y) - m_stretchHandlePos.y;
        float tx = m_stretchHandleTangent.x;
        float ty = m_stretchHandleTangent.y;
        float localTangent = dx * tx + dy * ty;
        float localRadial  = dx * (-ty) + dy * tx;   // (-ty, tx) is the radial unit direction
        if (std::abs(localTangent) <= m_subCircleRadius * 1.2f
         && std::abs(localRadial)  <= m_subCircleRadius * 0.4f)
            return true;
    }

    // center handle hit area — bounding square of the cross shape
    {
        float dx = float(x) - m_upmixCenter.x;
        float dy = float(y) - m_upmixCenter.y;
        float halfLen = m_subCircleRadius * 1.0f;
        if (std::abs(dx) <= halfLen && std::abs(dy) <= halfLen)
            return true;
    }

    return getRefitButtonBounds().contains(x, y)
        || m_upmixIndicator.contains(float(x), float(y))
        || m_upmixHeightIndicator.contains(float(x), float(y));
}

void UmsciUpmixIndicatorPaintNControlComponent::mouseDown(const juce::MouseEvent& e)
{
    if (processPinchGesture(e, true, false)) return;

    // Stretch handle takes priority over ring drags
    if (!m_renderedFloorPositions.empty() && m_stretchHandleTangent != juce::Point<float>{})
    {
        float dx = e.position.x - m_stretchHandlePos.x;
        float dy = e.position.y - m_stretchHandlePos.y;
        float tx = m_stretchHandleTangent.x;
        float ty = m_stretchHandleTangent.y;
        float localTangent = dx * tx + dy * ty;
        float localRadial  = dx * (-ty) + dy * tx;
        if (std::abs(localTangent) <= m_subCircleRadius * 1.2f
         && std::abs(localRadial)  <= m_subCircleRadius * 0.4f)
        {
            m_draggingStretchHandle = true;
            m_draggingHeightRing    = false;
            m_dragStartStretch      = m_upmixAngleStretch;
            m_dragStartAngle        = std::atan2(e.position.x - m_upmixCenter.x,
                                                 -(e.position.y - m_upmixCenter.y)) - m_upmixRot;
            return;
        }
    }
    m_draggingStretchHandle = false;

    // Center handle: square hit area matching the cross bounding box
    {
        float dx = e.position.x - m_upmixCenter.x;
        float dy = e.position.y - m_upmixCenter.y;
        float halfLen = m_subCircleRadius * 1.0f;
        if (std::abs(dx) <= halfLen && std::abs(dy) <= halfLen)
        {
            m_draggingCenterHandle = true;
            m_draggingHeightRing   = false;
            m_dragStartOffsetX     = m_upmixOffsetX;
            m_dragStartOffsetY     = m_upmixOffsetY;
            m_dragStartMousePos    = e.position;
            return;
        }
    }
    m_draggingCenterHandle = false;

    if (getRefitButtonBounds().contains(e.getPosition()))
    {
        m_upmixRot          = 0.0f;
        m_upmixTrans        = 1.0f;
        m_upmixHeightTrans  = 0.6f;
        m_upmixAngleStretch = 1.0f;
        m_upmixOffsetX      = 0.0f;
        m_upmixOffsetY      = 0.0f;
        PrerenderUpmixIndicatorInBounds();
        if (m_liveMode)
        {
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
        repaint();
        if (onTransformChanged)
            onTransformChanged();
        return;
    }

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
    if (processPinchGesture(e, false, false)) return;

    if (m_draggingStretchHandle)
    {
        auto dx = e.position.x - m_upmixCenter.x;
        auto dy = e.position.y - m_upmixCenter.y;
        // Delta-drag: accumulate the angular delta from the drag-start angle so that
        // the atan2 discontinuity at ±π never causes a snap.
        auto currentAngle = std::atan2(dx, -dy) - m_upmixRot;
        auto deltaAngle   = currentAngle - m_dragStartAngle;
        // Unwrap to (−π, π] so one revolution cannot produce a large jump.
        if (deltaAngle >  juce::MathConstants<float>::pi)  deltaAngle -= juce::MathConstants<float>::twoPi;
        if (deltaAngle < -juce::MathConstants<float>::pi)  deltaAngle += juce::MathConstants<float>::twoPi;
        if (m_naturalFloorMaxAngleDeg > 0.0f)
            m_upmixAngleStretch = juce::jlimit(0.05f,
                180.0f / m_naturalFloorMaxAngleDeg,
                m_dragStartStretch + deltaAngle / juce::degreesToRadians(m_naturalFloorMaxAngleDeg));

        PrerenderUpmixIndicatorInBounds();

        if (m_liveMode)
        {
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

        repaint();
        return;
    }

    if (m_draggingCenterHandle)
    {
        if (m_baseRadius > 0.0f)
        {
            m_upmixOffsetX = juce::jlimit(-2.0f, 2.0f,
                m_dragStartOffsetX + (e.position.x - m_dragStartMousePos.x) / m_baseRadius);
            m_upmixOffsetY = juce::jlimit(-2.0f, 2.0f,
                m_dragStartOffsetY + (e.position.y - m_dragStartMousePos.y) / m_baseRadius);
        }

        PrerenderUpmixIndicatorInBounds();

        if (m_liveMode)
        {
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

        repaint();
        return;
    }

    // Ignore drags that originated on the refit button — a tiny touch/mouse slip
    // must not bleed through into the ring rotation/scale logic.
    if (getRefitButtonBounds().contains(e.getMouseDownPosition()))
        return;

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

    if (m_liveMode)
    {
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
        updateFlashState(); // re-sync: stops any flash that PrerenderUpmixIndicatorInBounds may have started
    }

    repaint();
}

void UmsciUpmixIndicatorPaintNControlComponent::mouseUp(const juce::MouseEvent& e)
{
    if (processPinchGesture(e, false, true)) return;

    if (onTransformChanged)
        onTransformChanged();
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
        // Delegate to base class so empty-area double-click still resets zoom,
        // even when the component captures all events while flashing.
        UmsciPaintNControlComponentBase::mouseDoubleClick(e);
    }
}

void UmsciUpmixIndicatorPaintNControlComponent::timerCallback()
{
    m_flashState = !m_flashState;
    repaint();
}

void UmsciUpmixIndicatorPaintNControlComponent::onZoomChanged()
{
    PrerenderUpmixIndicatorInBounds();
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
    auto upmixIndicatorBounds = speakersScreenBoundingRect.getAspectRatio() <= 1 ? speakersScreenBoundingRect.expanded(speakersScreenBoundingRect.getWidth() * m_boundingFitFactor) : speakersScreenBoundingRect.expanded(speakersScreenBoundingRect.getHeight() * m_boundingFitFactor);

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

    // Capture the natural (pre-stretch) max floor angle for drag computation, then apply stretch.
    if (!upmixPositionAnglesDeg.empty())
        m_naturalFloorMaxAngleDeg = *std::max_element(upmixPositionAnglesDeg.begin(), upmixPositionAnglesDeg.end());
    for (auto& angleDeg : upmixPositionAnglesDeg)
        angleDeg *= m_upmixAngleStretch;
    for (auto& angleDeg : upmixHeightPositionAnglesDeg)
        angleDeg *= m_upmixAngleStretch;

    if (upmixIndicatorBounds.isEmpty())
        return;

    m_upmixCenter = upmixIndicatorBounds.getCentre();
    // Use the larger screen dimension so the indicator fills the room's dominant axis rather
    // than being constrained to the smaller one (which makes it look undersized in landscape layouts).
    m_baseRadius  = std::max(upmixIndicatorBounds.getWidth(), upmixIndicatorBounds.getHeight()) * 0.5f;
    m_upmixCenter.x += m_upmixOffsetX * m_baseRadius;
    m_upmixCenter.y += m_upmixOffsetY * m_baseRadius;
    auto cx = m_upmixCenter.x;
    auto cy = m_upmixCenter.y;
    auto baseRadius = m_baseRadius;
    auto radius = baseRadius * m_upmixTrans;
    m_subCircleRadius = 15.0f * getControlsSizeMultiplier();
    auto arcStrokeWidth = m_subCircleRadius * 0.5f;
    auto cosRot = std::cos(m_upmixRot);
    auto sinRot = std::sin(m_upmixRot);

    // builds an open rectangle path from minDeg to maxDeg (clockwise), rotated by m_upmixRot
    auto buildOpenRectPath = [&](float r, float minDeg, float maxDeg) -> juce::Path
    {
        struct Corner { float angleDeg, lx, ly; };
        const Corner corners[4] = {
            {  45.0f,  r, -r },
            { 135.0f,  r,  r },
            { 225.0f, -r,  r },
            { 315.0f, -r, -r },
        };

        // clockwise angular distance from a normalised origin to an arbitrary angle
        auto cwDist = [](float fromNorm, float toAngle) {
            float toNorm = std::fmod(toAngle, 360.0f);
            if (toNorm < 0.0f) toNorm += 360.0f;
            float d = toNorm - fromNorm;
            return d <= 0.0f ? d + 360.0f : d;
        };

        // project an angle onto the axis-aligned square perimeter (Chebyshev)
        auto toLocal = [&](float angleDeg) -> juce::Point<float> {
            auto rad = juce::degreesToRadians(angleDeg);
            auto dx = std::sin(rad);
            auto dy = -std::cos(rad);
            auto t  = r / std::max(std::abs(dx), std::abs(dy));
            return { t * dx, t * dy };
        };

        // rotate a local offset by m_upmixRot and translate to screen space
        auto toScreen = [&](juce::Point<float> local) -> juce::Point<float> {
            return { cx + local.x * cosRot - local.y * sinRot,
                     cy + local.x * sinRot + local.y * cosRot };
        };

        float minNorm = std::fmod(minDeg, 360.0f);
        if (minNorm < 0.0f) minNorm += 360.0f;
        float maxDist = cwDist(minNorm, maxDeg);

        // find the first corner clockwise from minNorm
        int firstIdx = 0;
        float firstDist = 361.0f;
        for (int k = 0; k < 4; ++k)
        {
            float d = cwDist(minNorm, corners[k].angleDeg);
            if (d < firstDist) { firstDist = d; firstIdx = k; }
        }

        juce::Path path;
        path.startNewSubPath(toScreen(toLocal(minDeg)));
        for (int j = 0; j < 4; ++j)
        {
            int k = (firstIdx + j) % 4;
            if (cwDist(minNorm, corners[k].angleDeg) < maxDist)
                path.lineTo(toScreen({ corners[k].lx, corners[k].ly }));
            else
                break;
        }
        path.lineTo(toScreen(toLocal(maxDeg)));
        return path;
    };

    // build the floor channel ring
    if (!upmixPositionAnglesDeg.empty())
    {
        auto minAngleDeg = *std::min_element(upmixPositionAnglesDeg.begin(), upmixPositionAnglesDeg.end());
        auto maxAngleDeg = *std::max_element(upmixPositionAnglesDeg.begin(), upmixPositionAnglesDeg.end());

        if (m_shape == IndicatorShape::Rectangle)
        {
            juce::PathStrokeType(arcStrokeWidth).createStrokedPath(m_upmixIndicator,
                buildOpenRectPath(radius, minAngleDeg, maxAngleDeg));
        }
        else
        {
            // build the arc segment and stroke it into a filled band in m_upmixIndicator
            // angles follow JUCE convention: 0 = 12 o'clock, clockwise positive — matching standard audio azimuth
            juce::Path arcPath;
            arcPath.addCentredArc(cx, cy, radius, radius, m_upmixRot,
                                  juce::degreesToRadians(minAngleDeg),
                                  juce::degreesToRadians(maxAngleDeg),
                                  true);
            juce::PathStrokeType(arcStrokeWidth).createStrokedPath(m_upmixIndicator, arcPath);
        }

        // add a filled sub-circle at each position and record its data for paint() and hit-testing
        for (size_t i = 0; i < upmixPositionAnglesDeg.size(); ++i)
        {
            float px, py;
            if (m_shape == IndicatorShape::Rectangle)
            {
                // project base angle (no rotation) onto axis-aligned square, then rotate the result
                auto baseAngleRad = juce::degreesToRadians(upmixPositionAnglesDeg[i]);
                auto dx = std::sin(baseAngleRad);
                auto dy = -std::cos(baseAngleRad);
                auto t = radius / std::max(std::abs(dx), std::abs(dy));
                px = cx + (t * dx) * cosRot - (t * dy) * sinRot;
                py = cy + (t * dx) * sinRot + (t * dy) * cosRot;
            }
            else
            {
                // screen coords for arc angle θ with rotation R: x = cx + r·sin(θ+R), y = cy - r·cos(θ+R)
                auto effectiveAngleRad = juce::degreesToRadians(upmixPositionAnglesDeg[i]) + m_upmixRot;
                px = cx + radius * std::sin(effectiveAngleRad);
                py = cy - radius * std::cos(effectiveAngleRad);
            }

            m_upmixIndicator.addEllipse(px - m_subCircleRadius, py - m_subCircleRadius,
                                        m_subCircleRadius * 2.0f, m_subCircleRadius * 2.0f);

            if (i < upmixPositionNames.size())
            {
                RenderedChannelPosition rcp;
                rcp.sourceId  = static_cast<std::int16_t>(
                    m_sourceStartId + getChannelNumberForChannelTypeInCurrentConfiguration(upmixPositionChannelTypes[i]) - 1);
                rcp.screenPos = juce::Point<float>(px, py);
                rcp.realPos   = GetRealCoordinateForPoint(rcp.screenPos);
                rcp.realPos[2] = 1.2f;
                rcp.label     = juce::String(upmixPositionNames[i]);
                m_renderedFloorPositions.push_back(rcp);
            }
        }
    }

    // compute stretch handle position — radially beyond the max-angle floor sub-circle
    if (!upmixPositionAnglesDeg.empty())
    {
        auto maxStretchedAngleDeg = *std::max_element(upmixPositionAnglesDeg.begin(), upmixPositionAnglesDeg.end());
        float anchorX, anchorY;
        float rectEdgeTangentX = 0.0f, rectEdgeTangentY = 0.0f;
        if (m_shape == IndicatorShape::Rectangle)
        {
            auto baseAngleRad = juce::degreesToRadians(maxStretchedAngleDeg);
            auto dx = std::sin(baseAngleRad);
            auto dy = -std::cos(baseAngleRad);
            auto t = radius / std::max(std::abs(dx), std::abs(dy));
            anchorX = cx + (t * dx) * cosRot - (t * dy) * sinRot;
            anchorY = cy + (t * dx) * sinRot + (t * dy) * cosRot;
            // Tangent along the rectangle edge in the unrotated frame:
            //   |dx| >= |dy|  →  left/right (vertical) edge  →  unrotated tangent (0, 1)
            //   |dy| >  |dx|  →  top/bottom (horizontal) edge →  unrotated tangent (1, 0)
            // Then rotate by m_upmixRot into screen space.
            float utx = (std::abs(dx) >= std::abs(dy)) ? 0.0f : 1.0f;
            float uty = (std::abs(dx) >= std::abs(dy)) ? 1.0f : 0.0f;
            rectEdgeTangentX = utx * cosRot - uty * sinRot;
            rectEdgeTangentY = utx * sinRot + uty * cosRot;
        }
        else
        {
            auto effectiveAngleRad = juce::degreesToRadians(maxStretchedAngleDeg) + m_upmixRot;
            anchorX = cx + radius * std::sin(effectiveAngleRad);
            anchorY = cy - radius * std::cos(effectiveAngleRad);
        }
        float dirX = anchorX - cx;
        float dirY = anchorY - cy;
        float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
        if (dirLen > 0.0f)
        {
            float ux = dirX / dirLen;
            float uy = dirY / dirLen;
            m_stretchHandlePos = { anchorX + ux * m_subCircleRadius * 1.5f,
                                   anchorY + uy * m_subCircleRadius * 1.5f };
            if (m_shape == IndicatorShape::Rectangle)
                // Tangent lies along the rectangle edge at the anchor point.
                m_stretchHandleTangent = { rectEdgeTangentX, rectEdgeTangentY };
            else
                // Tangent: 90° CW from radial (ux, uy) in screen space = (uy, -ux)
                m_stretchHandleTangent = { uy, -ux };
        }
    }

    // build the height channel ring — same rotation, independent scale
    if (!upmixHeightPositionAnglesDeg.empty())
    {
        auto heightRadius = baseRadius * m_upmixHeightTrans;
        auto heightArcStrokeWidth = arcStrokeWidth;

        auto minHeightAngleDeg = *std::min_element(upmixHeightPositionAnglesDeg.begin(), upmixHeightPositionAnglesDeg.end());
        auto maxHeightAngleDeg = *std::max_element(upmixHeightPositionAnglesDeg.begin(), upmixHeightPositionAnglesDeg.end());

        if (m_shape == IndicatorShape::Rectangle)
        {
            juce::PathStrokeType(heightArcStrokeWidth).createStrokedPath(m_upmixHeightIndicator,
                buildOpenRectPath(heightRadius, minHeightAngleDeg, maxHeightAngleDeg));
        }
        else
        {
            juce::Path heightArcPath;
            heightArcPath.addCentredArc(cx, cy, heightRadius, heightRadius, m_upmixRot,
                                        juce::degreesToRadians(minHeightAngleDeg),
                                        juce::degreesToRadians(maxHeightAngleDeg),
                                        true);
            juce::PathStrokeType(heightArcStrokeWidth).createStrokedPath(m_upmixHeightIndicator, heightArcPath);
        }

        for (size_t i = 0; i < upmixHeightPositionAnglesDeg.size(); ++i)
        {
            float px, py;
            if (m_shape == IndicatorShape::Rectangle)
            {
                auto baseAngleRad = juce::degreesToRadians(upmixHeightPositionAnglesDeg[i]);
                auto dx = std::sin(baseAngleRad);
                auto dy = -std::cos(baseAngleRad);
                auto t = heightRadius / std::max(std::abs(dx), std::abs(dy));
                px = cx + (t * dx) * cosRot - (t * dy) * sinRot;
                py = cy + (t * dx) * sinRot + (t * dy) * cosRot;
            }
            else
            {
                auto effectiveAngleRad = juce::degreesToRadians(upmixHeightPositionAnglesDeg[i]) + m_upmixRot;
                px = cx + heightRadius * std::sin(effectiveAngleRad);
                py = cy - heightRadius * std::cos(effectiveAngleRad);
            }

            m_upmixHeightIndicator.addEllipse(px - m_subCircleRadius, py - m_subCircleRadius,
                                              m_subCircleRadius * 2.0f, m_subCircleRadius * 2.0f);

            if (i < upmixHeightPositionNames.size())
            {
                RenderedChannelPosition rcp;
                rcp.sourceId  = static_cast<std::int16_t>(
                    m_sourceStartId + getChannelNumberForChannelTypeInCurrentConfiguration(upmixHeightPositionChannelTypes[i]) - 1);
                rcp.screenPos = juce::Point<float>(px, py);
                rcp.realPos   = GetRealCoordinateForPoint(rcp.screenPos);
                rcp.realPos[2] = m_speakersRealBoundingCube[5]
                    + (m_speakersRealBoundingCube[5] - m_speakersRealBoundingCube[2]) * m_boundingFitFactor;
                rcp.label     = juce::String(upmixHeightPositionNames[i]);
                m_renderedHeightPositions.push_back(rcp);
            }
        }
    }

    // prerender handle paths — shared builder for a single bidirectional arrow
    auto buildBiArrowPath = [](float hcx, float hcy, float axX, float axY,
                               float halfLen, float headLen, float headWidth, float lineWidth) -> juce::Path
    {
        float px = -axY, py = axX; // perpendicular to axis

        juce::Path shaft;
        shaft.startNewSubPath(hcx - axX * (halfLen - headLen), hcy - axY * (halfLen - headLen));
        shaft.lineTo        (hcx + axX * (halfLen - headLen), hcy + axY * (halfLen - headLen));

        juce::Path result;
        juce::PathStrokeType(lineWidth, juce::PathStrokeType::beveled,
                             juce::PathStrokeType::square).createStrokedPath(result, shaft);

        // arrowhead at positive end
        float tipX = hcx + axX * halfLen,  tipY = hcy + axY * halfLen;
        float bpX  = hcx + axX * (halfLen - headLen), bpY = hcy + axY * (halfLen - headLen);
        result.startNewSubPath(tipX, tipY);
        result.lineTo(bpX + px * headWidth, bpY + py * headWidth);
        result.lineTo(bpX - px * headWidth, bpY - py * headWidth);
        result.closeSubPath();

        // arrowhead at negative end
        float tnX = hcx - axX * halfLen,  tnY = hcy - axY * halfLen;
        float bnX = hcx - axX * (halfLen - headLen), bnY = hcy - axY * (halfLen - headLen);
        result.startNewSubPath(tnX, tnY);
        result.lineTo(bnX - px * headWidth, bnY - py * headWidth);
        result.lineTo(bnX + px * headWidth, bnY + py * headWidth);
        result.closeSubPath();

        return result;
    };

    // center handle: two crossed bidirectional arrows aligned to screen axes
    m_centerHandlePath.clear();
    {
        float halfLen   = m_subCircleRadius * 1.0f;
        float headLen   = m_subCircleRadius * 0.45f;
        float headWidth = m_subCircleRadius * 0.35f;
        float lineWidth = m_subCircleRadius * 0.18f;
        m_centerHandlePath.addPath(buildBiArrowPath(cx, cy, 1.0f, 0.0f, halfLen, headLen, headWidth, lineWidth));
        m_centerHandlePath.addPath(buildBiArrowPath(cx, cy, 0.0f, 1.0f, halfLen, headLen, headWidth, lineWidth));
    }

    // stretch handle: single bidirectional arrow along the tangent at the arc endpoint
    m_stretchHandlePath.clear();
    if (!upmixPositionAnglesDeg.empty() && m_stretchHandleTangent != juce::Point<float>{})
    {
        float halfLen   = m_subCircleRadius * 1.2f;
        float headLen   = m_subCircleRadius * 0.55f;
        float headWidth = m_subCircleRadius * 0.4f;
        float lineWidth = m_subCircleRadius * 0.18f;
        m_stretchHandlePath = buildBiArrowPath(
            m_stretchHandlePos.x, m_stretchHandlePos.y,
            m_stretchHandleTangent.x, m_stretchHandleTangent.y,
            halfLen, headLen, headWidth, lineWidth);
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

void UmsciUpmixIndicatorPaintNControlComponent::setLiveMode(bool liveMode)
{
    m_liveMode = liveMode;
}

bool UmsciUpmixIndicatorPaintNControlComponent::getLiveMode() const
{
    return m_liveMode;
}

void UmsciUpmixIndicatorPaintNControlComponent::setControlsSize(ControlsSize size)
{
    UmsciPaintNControlComponentBase::setControlsSize(size);
    PrerenderUpmixIndicatorInBounds();
}

void UmsciUpmixIndicatorPaintNControlComponent::setShape(IndicatorShape shape)
{
    m_shape = shape;
    PrerenderUpmixIndicatorInBounds();
    repaint();
}

UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape UmsciUpmixIndicatorPaintNControlComponent::getShape() const
{
    return m_shape;
}

void UmsciUpmixIndicatorPaintNControlComponent::setUpmixTransform(float rot, float trans, float heightTrans, float angleStretch)
{
    m_upmixRot          = rot;
    m_upmixTrans        = trans;
    m_upmixHeightTrans  = heightTrans;
    m_upmixAngleStretch = m_naturalFloorMaxAngleDeg > 0.0f
                              ? juce::jlimit(0.05f, 180.0f / m_naturalFloorMaxAngleDeg, angleStretch)
                              : angleStretch;
    PrerenderUpmixIndicatorInBounds();
    repaint();
}

float UmsciUpmixIndicatorPaintNControlComponent::getUpmixRot() const         { return m_upmixRot; }
float UmsciUpmixIndicatorPaintNControlComponent::getUpmixTrans() const       { return m_upmixTrans; }
float UmsciUpmixIndicatorPaintNControlComponent::getUpmixHeightTrans() const  { return m_upmixHeightTrans; }
float UmsciUpmixIndicatorPaintNControlComponent::getUpmixAngleStretch() const { return m_upmixAngleStretch; }

void UmsciUpmixIndicatorPaintNControlComponent::setUpmixOffset(float x, float y)
{
    m_upmixOffsetX = x;
    m_upmixOffsetY = y;
    PrerenderUpmixIndicatorInBounds();
    repaint();
}

float UmsciUpmixIndicatorPaintNControlComponent::getUpmixOffsetX() const { return m_upmixOffsetX; }
float UmsciUpmixIndicatorPaintNControlComponent::getUpmixOffsetY() const { return m_upmixOffsetY; }

void UmsciUpmixIndicatorPaintNControlComponent::notifyTransformChanged()
{
    if (m_liveMode)
    {
        m_inhibitFlashCount += static_cast<int>(m_renderedFloorPositions.size() + m_renderedHeightPositions.size());
        stopTimer();
        m_flashState = false;
        repaint();

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
    }
    if (onTransformChanged)
        onTransformChanged();
}

void UmsciUpmixIndicatorPaintNControlComponent::triggerFlashCheck()
{
    updateFlashState();
}

bool UmsciUpmixIndicatorPaintNControlComponent::setChannelConfiguration(const juce::AudioChannelSet& channelLayout)
{
    auto rVal = TwoDFieldBase::setChannelConfiguration(channelLayout);

    PrerenderUpmixIndicatorInBounds();

    repaint();

    return rVal;
}

juce::Rectangle<int> UmsciUpmixIndicatorPaintNControlComponent::getRefitButtonBounds() const
{
    auto margin = 4;
    auto buttonScale  = std::min(getControlsSizeMultiplier(), 1.5f);
    auto buttonHeight = juce::roundToInt(40.0f * buttonScale);
    auto buttonWidth  = juce::roundToInt(60.0f * buttonScale);
    return juce::Rectangle<int>(getWidth() - buttonWidth - margin, margin, buttonWidth, buttonHeight);
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

