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


class UmsciSoundobjectsPaintComponent :   public juce::Component
{
public:
    UmsciSoundobjectsPaintComponent();
    ~UmsciSoundobjectsPaintComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;

    //==============================================================================
    void setBoundsRealRef(const juce::Rectangle<float>& boundsRealRef);
    void setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions);

private:
    //==============================================================================
    std::unique_ptr<juce::Label>    m_hintLabel;
    juce::Rectangle<float> m_boundsRealRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciSoundobjectsPaintComponent)
};

