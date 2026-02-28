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

#include "UmsciPaintNControlComponentBase.h"

class UmsciUpmixIndicatorPaintNControlComponent :   public UmsciPaintNControlComponentBase
{
public:
    UmsciUpmixIndicatorPaintNControlComponent();
    ~UmsciUpmixIndicatorPaintNControlComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;

    //==============================================================================
    void setSpeakersRealBoundingCube(const std::array<float, 6>& speakersRealBoundingCube);
    void setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions);

private:
    //==============================================================================
    void PrerenderUpmixIndicatorInBounds();

    //==============================================================================
    std::array<float, 6>                                m_speakersRealBoundingCube;
    std::map<std::int16_t, std::array<std::float_t, 3>> m_sourcePositions;

    juce::Path                  m_upmixIndicator;
    float                       m_upmixRot = 0.0f;
    std::vector<std::string>    m_upmixPositionNames = { "l", "r", "c", "lfe", "lss", "rss", "lsr", "rsr", "tfl", "tfr", "trl", "trr" };
    std::vector<float>          m_upmixPositionAnglesDeg = { -30.0f,30.0f,0.0f,0.0f,-100.0f,100.0f,-145.0f,145.0f,-45.0f,45.0f,-135.0f,135.0f };

    float                                                       m_subCircleRadius = 0.0f;
    std::vector<std::pair<juce::Point<float>, juce::String>>    m_renderedPositionLabels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciUpmixIndicatorPaintNControlComponent)
};

