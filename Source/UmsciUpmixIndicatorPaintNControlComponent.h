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

#include <TwoDFieldBase.h>

class UmsciUpmixIndicatorPaintNControlComponent :   public UmsciPaintNControlComponentBase, public JUCEAppBasics::TwoDFieldBase, public juce::Timer
{
public:
    UmsciUpmixIndicatorPaintNControlComponent();
    ~UmsciUpmixIndicatorPaintNControlComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;
    bool hitTest(int x, int y) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    //==============================================================================
    void timerCallback() override;

    //==============================================================================
    bool setChannelConfiguration(const juce::AudioChannelSet& channelLayout) override;

    //==============================================================================
    void setSpeakersRealBoundingCube(const std::array<float, 6>& speakersRealBoundingCube);
    void setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions);
    void setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position);

    //==============================================================================
    void setSourceStartId(int startId);
    int  getSourceStartId() const;

    //==============================================================================
    std::function<void(std::int16_t, std::array<std::float_t, 3>)> onSourcePositionChanged;

private:
    //==============================================================================
    struct RenderedChannelPosition
    {
        std::int16_t            sourceId  = 0;
        juce::Point<float>      screenPos;
        std::array<float, 3>    realPos   = { 0.0f, 0.0f, 0.0f };
        juce::String            label;
    };

    //==============================================================================
    void PrerenderUpmixIndicatorInBounds();
    void updateFlashState();

    //==============================================================================
    int                                                 m_sourceStartId = 1;

    //==============================================================================
    std::array<float, 6>                                m_speakersRealBoundingCube;
    std::map<std::int16_t, std::array<std::float_t, 3>> m_sourcePositions;

    juce::Path                  m_upmixIndicator;
    juce::Path                  m_upmixHeightIndicator;
    float                       m_upmixRot          = 0.0f;
    float                       m_upmixTrans        = 1.0f;
    float                       m_upmixHeightTrans  = 0.6f;  // 40% smaller than floor ring by default

    juce::Point<float>                       m_upmixCenter;
    float                                    m_subCircleRadius = 0.0f;
    std::vector<RenderedChannelPosition>     m_renderedFloorPositions;
    std::vector<RenderedChannelPosition>     m_renderedHeightPositions;

    bool                        m_draggingHeightRing   = false;
    float                       m_dragStartAngle       = 0.0f;
    float                       m_dragStartDist        = 0.0f;
    float                       m_dragStartRot         = 0.0f;
    float                       m_dragStartTrans       = 0.0f;
    float                       m_dragStartHeightTrans = 0.6f;

    bool                        m_flashState           = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciUpmixIndicatorPaintNControlComponent)
};

