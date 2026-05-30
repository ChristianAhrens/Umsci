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

#include <ParameterControlComponent.h>

#include "CustomParameterConfig.h"


/**
 * @class CustomParameterControlComponent
 * @brief Umsci-specific wrapper around ParameterControlComponent that adapts
 *        CustomParameterConfig entries into generic ParameterControlInfo descriptors.
 *
 * Adds a thin visual separator on the edge adjacent to the main canvas, and
 * provides a control-size setter that accepts the same int index (0=S, 1=M, 2=L)
 * used throughout the rest of Umsci's size-setting calls.
 */
class CustomParameterControlComponent : public JUCEAppBasics::ParameterControlComponent
{
public:
    CustomParameterControlComponent();
    ~CustomParameterControlComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;

    //==============================================================================
    /** Converts CustomParameterConfig into ParameterControlInfo entries and
     *  calls setParameters() on the base class.  No-op if config is unchanged. */
    void setCustomConfig(const CustomParameterConfig& config);

    /** Sets control size using the Umsci int convention: 0=S, 1=M, 2=L. */
    void setControlSize(int sizeIndex);

private:
    CustomParameterConfig m_config;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomParameterControlComponent)
};
