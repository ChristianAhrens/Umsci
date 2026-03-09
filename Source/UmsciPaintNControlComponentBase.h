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

#pragma once

#include <JuceHeader.h>


class UmsciPaintNControlComponentBase :   public juce::Component
{
public:
    enum class ControlsSize { S, M, L };

    UmsciPaintNControlComponentBase();
    virtual ~UmsciPaintNControlComponentBase() override;

    //==============================================================================
    void setBoundsRealRef(const juce::Rectangle<float>& boundsRealRef);

    //==============================================================================
    virtual void setControlsSize(ControlsSize size);
    ControlsSize getControlsSize() const;
    float getControlsSizeMultiplier() const;

    //==============================================================================
    // Viewport zoom — factor 1.0 = no zoom; panOffset is normalised to base content dimensions.
    // setZoom() applies zoom without firing onViewportZoomChanged (used for sibling sync).
    void  setZoom(float factor, juce::Point<float> normalizedPanOffset = {});
    float getZoomFactor() const;
    void  resetZoom();

    // Fired after every user-initiated zoom change so the parent can synchronise siblings.
    std::function<void(float, juce::Point<float>)> onViewportZoomChanged;

protected:
    //==============================================================================
    juce::Point<float> GetPointForRealCoordinate(const std::array<float, 3>& realCoordinate);
    std::array<float, 3> GetRealCoordinateForPoint(const juce::Point<float>& screenPoint);

    //==============================================================================
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify(const juce::MouseEvent&, float scaleFactor) override;

    // Called after any zoom state change; base implementation calls repaint().
    // Derived classes override to re-trigger their prerender pass before repainting.
    virtual void onZoomChanged();

private:
    //==============================================================================
    juce::Rectangle<float> computeBaseContentBounds() const;
    juce::Rectangle<float> getContentBounds() const; // returns zoom-adjusted bounds

    void applyZoomAtScreenPoint(float newFactor, juce::Point<float> screenFocus);

    juce::Rectangle<float>  m_boundsRealRef;
    ControlsSize             m_controlsSize = ControlsSize::S;

    // Zoom state — normalised pan offset survives component resizes.
    float                   m_zoomFactor     = 1.0f;
    juce::Point<float>      m_zoomPanOffset;            // fraction of base content width/height

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciPaintNControlComponentBase)
};

