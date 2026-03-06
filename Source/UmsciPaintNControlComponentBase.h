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

protected:
    //==============================================================================
    juce::Point<float> GetPointForRealCoordinate(const std::array<float, 3>& realCoordinate);
    std::array<float, 3> GetRealCoordinateForPoint(const juce::Point<float>& screenPoint);

private:
    //==============================================================================
    juce::Rectangle<float> getContentBounds() const;

    juce::Rectangle<float> m_boundsRealRef;
    ControlsSize           m_controlsSize = ControlsSize::S;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciPaintNControlComponentBase)
};

