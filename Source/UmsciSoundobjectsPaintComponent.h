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

#include <set>


class UmsciSoundobjectsPaintComponent :   public UmsciPaintNControlComponentBase
{
public:
    UmsciSoundobjectsPaintComponent();
    ~UmsciSoundobjectsPaintComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;

    //==============================================================================
    bool hitTest(int x, int y) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    //==============================================================================
    void setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions);
    void setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position);

    //==============================================================================
    void setSourceIdFilter(const std::set<std::int16_t>& allowedIds);

    //==============================================================================
    std::function<void(std::int16_t, std::array<std::float_t, 3>)> onSourcePositionChanged;

private:
    //==============================================================================
    void onZoomChanged() override;
    void PrerenderSourcesInBounds();

    //==============================================================================
    std::map<std::int16_t, std::array<std::float_t, 3>> m_sourcePositions;
    std::map<std::int16_t, juce::Point<int>>            m_sourceScreenPositions;
    std::set<std::int16_t>                              m_sourceIdFilter;

    std::int16_t m_draggedSourceId{ -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciSoundobjectsPaintComponent)
};

