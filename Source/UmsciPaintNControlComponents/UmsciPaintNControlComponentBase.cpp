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

#include "UmsciPaintNControlComponentBase.h"


UmsciPaintNControlComponentBase::UmsciPaintNControlComponentBase()
    : juce::Component()
{
}

UmsciPaintNControlComponentBase::~UmsciPaintNControlComponentBase()
{
}

void UmsciPaintNControlComponentBase::setControlsSize(ControlsSize size)
{
    m_controlsSize = size;
    repaint();
}

UmsciPaintNControlComponentBase::ControlsSize UmsciPaintNControlComponentBase::getControlsSize() const
{
    return m_controlsSize;
}

float UmsciPaintNControlComponentBase::getControlsSizeMultiplier() const
{
    switch (m_controlsSize)
    {
    case ControlsSize::M:
        return 1.5f;
    case ControlsSize::L:
        return 2.0f;
    case ControlsSize::S:
    default:
        return 1.0f;
    }
}

void UmsciPaintNControlComponentBase::setBoundsRealRef(const juce::Rectangle<float>& boundsRealRef)
{
    m_boundsRealRef = boundsRealRef;
}

juce::Rectangle<float> UmsciPaintNControlComponentBase::computeBaseContentBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    if (m_boundsRealRef.isEmpty())
        return bounds;

    if (m_boundsRealRef.getAspectRatio() > bounds.getAspectRatio())
        return bounds.withSizeKeepingCentre(bounds.getWidth(), bounds.getWidth() / m_boundsRealRef.getAspectRatio());
    else
        return bounds.withSizeKeepingCentre(bounds.getHeight() * m_boundsRealRef.getAspectRatio(), bounds.getHeight());
}

juce::Rectangle<float> UmsciPaintNControlComponentBase::getContentBounds() const
{
    auto base = computeBaseContentBounds();
    if (m_zoomFactor == 1.0f && m_zoomPanOffset.isOrigin())
        return base;

    // Centre of the zoomed view in component-pixel space (normalised offset * base size).
    auto cx = base.getCentreX() + m_zoomPanOffset.x * base.getWidth();
    auto cy = base.getCentreY() + m_zoomPanOffset.y * base.getHeight();
    auto hw = base.getWidth()  * m_zoomFactor * 0.5f;
    auto hh = base.getHeight() * m_zoomFactor * 0.5f;
    return { cx - hw, cy - hh, hw * 2.0f, hh * 2.0f };
}

void UmsciPaintNControlComponentBase::applyZoomAtScreenPoint(float newFactor, juce::Point<float> screenFocus)
{
    newFactor = juce::jlimit(0.1f, 10.0f, newFactor);
    auto rz  = newFactor / m_zoomFactor;

    auto base = computeBaseContentBounds();
    if (!base.isEmpty())
    {
        // Express screenFocus as a normalised offset from the base content centre.
        auto Pnorm = juce::Point<float>((screenFocus.x - base.getCentreX()) / base.getWidth(),
                                        (screenFocus.y - base.getCentreY()) / base.getHeight());
        // Scale the existing pan offset toward/away from the focus point.
        m_zoomPanOffset = Pnorm + (m_zoomPanOffset - Pnorm) * rz;
    }

    m_zoomFactor = newFactor;
}

void UmsciPaintNControlComponentBase::mouseDoubleClick(const juce::MouseEvent&)
{
    resetZoom();
}

void UmsciPaintNControlComponentBase::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const float zoomSpeed = 0.5f;
    auto newFactor = m_zoomFactor * std::exp(wheel.deltaY * zoomSpeed);
    applyZoomAtScreenPoint(newFactor, e.position);

    onZoomChanged();

    if (onViewportZoomChanged)
        onViewportZoomChanged(m_zoomFactor, m_zoomPanOffset);
}

void UmsciPaintNControlComponentBase::mouseMagnify(const juce::MouseEvent& e, float scaleFactor)
{
    applyZoomAtScreenPoint(m_zoomFactor * scaleFactor, e.position);

    onZoomChanged();

    if (onViewportZoomChanged)
        onViewportZoomChanged(m_zoomFactor, m_zoomPanOffset);
}

bool UmsciPaintNControlComponentBase::processPinchGesture(const juce::MouseEvent& e, bool isDown, bool isUp)
{
    const int idx = e.source.getIndex();

    if (idx < 0 || idx > 1)
        return false; // only track the first two simultaneous touches

    if (isDown)
    {
        m_pinchDown[idx] = true;
        m_pinchPos[idx]  = e.position;

        if (m_pinchDown[0] && m_pinchDown[1])
        {
            auto d = m_pinchPos[1] - m_pinchPos[0];
            m_pinchStartDistance = std::hypot(d.x, d.y);
            m_pinchStartZoom     = m_zoomFactor;
            if (m_pinchStartDistance > 1.0f)
                m_pinchActive = true;
        }
        // Suppress non-primary touches from triggering element interactions.
        return idx > 0;
    }
    else if (isUp)
    {
        const bool wasActive = m_pinchActive;
        m_pinchDown[idx] = false;
        if (!m_pinchDown[0] || !m_pinchDown[1])
            m_pinchActive = false;
        // Suppress the up event for non-primary touches, or whenever a pinch just ended.
        return idx > 0 || wasActive;
    }
    else // drag
    {
        m_pinchPos[idx] = e.position;

        if (m_pinchActive && m_pinchDown[0] && m_pinchDown[1])
        {
            auto d = m_pinchPos[1] - m_pinchPos[0];
            const float currentDist = std::hypot(d.x, d.y);
            if (currentDist > 1.0f && m_pinchStartDistance > 1.0f)
            {
                auto midpoint = (m_pinchPos[0] + m_pinchPos[1]) * 0.5f;
                applyZoomAtScreenPoint(m_pinchStartZoom * currentDist / m_pinchStartDistance, midpoint);
                onZoomChanged();
                if (onViewportZoomChanged)
                    onViewportZoomChanged(m_zoomFactor, m_zoomPanOffset);
            }
            return true;
        }
        return m_pinchActive;
    }
}

void UmsciPaintNControlComponentBase::simulatePinchZoom(float scaleFactor, juce::Point<float> centre)
{
    applyZoomAtScreenPoint(m_zoomFactor * scaleFactor, centre);
    onZoomChanged();
    if (onViewportZoomChanged)
        onViewportZoomChanged(m_zoomFactor, m_zoomPanOffset);
}

void UmsciPaintNControlComponentBase::onZoomChanged()
{
    repaint();
}

void UmsciPaintNControlComponentBase::setZoom(float factor, juce::Point<float> normalizedPanOffset)
{
    auto clamped = juce::jlimit(0.1f, 10.0f, factor);
    if (m_zoomFactor == clamped && m_zoomPanOffset == normalizedPanOffset)
        return;

    m_zoomFactor    = clamped;
    m_zoomPanOffset = normalizedPanOffset;
    onZoomChanged();
}

float UmsciPaintNControlComponentBase::getZoomFactor() const
{
    return m_zoomFactor;
}

void UmsciPaintNControlComponentBase::resetZoom()
{
    if (m_zoomFactor == 1.0f && m_zoomPanOffset.isOrigin())
        return;

    m_zoomFactor    = 1.0f;
    m_zoomPanOffset = {};
    onZoomChanged();

    if (onViewportZoomChanged)
        onViewportZoomChanged(m_zoomFactor, m_zoomPanOffset);
}

std::array<float, 3> UmsciPaintNControlComponentBase::GetRealCoordinateForPoint(const juce::Point<float>& screenPoint)
{
    auto contentBounds = getContentBounds();
    if (contentBounds.getWidth() == 0.0f || contentBounds.getHeight() == 0.0f)
        return { 0.0f, 0.0f, 0.0f };

    auto relativeX = (screenPoint.getX() - contentBounds.getX()) / contentBounds.getWidth();
    auto relativeY = (screenPoint.getY() - contentBounds.getY()) / contentBounds.getHeight();

    // m_boundsRealRef is screen-space-aligned: getX()/getWidth() = d&b y, getY()/getHeight() = d&b x
    auto yReal = relativeX * m_boundsRealRef.getWidth()  + m_boundsRealRef.getX();
    auto xReal = relativeY * m_boundsRealRef.getHeight() + m_boundsRealRef.getY();

    return { xReal, yReal, 0.0f };
}

juce::Point<float> UmsciPaintNControlComponentBase::GetPointForRealCoordinate(const std::array<float, 3>& realCoordinate)
{
    auto& xReal = realCoordinate.at(0);
    auto& yReal = realCoordinate.at(1);
    //auto& zReal = realCoordinate.at(2);

    if (m_boundsRealRef.getWidth() == 0.0f || m_boundsRealRef.getHeight() == 0.0f)
        return { 0.0f, 0.0f };

    // m_boundsRealRef is screen-space-aligned: getX()/getWidth() = d&b y, getY()/getHeight() = d&b x
    auto relativeX = (yReal - m_boundsRealRef.getX()) / m_boundsRealRef.getWidth();
    auto relativeY = (xReal - m_boundsRealRef.getY()) / m_boundsRealRef.getHeight();

    return getContentBounds().getRelativePoint(relativeX, relativeY);
}

