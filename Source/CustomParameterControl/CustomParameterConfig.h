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


/**
 * @brief Configuration for a single user-defined custom OSC parameter.
 *
 * Extends the generic ParameterControlInfo with an OSC address that maps
 * the parameter to a specific OSC message path on the remote host.
 */
struct CustomParameterEntry
{
    juce::String                         name        = "Parameter";
    JUCEAppBasics::ParameterControlType  controlType = JUCEAppBasics::ParameterControlType::Continuous;
    float                                minValue    = 0.0f;
    float                                maxValue    = 1.0f;
    float                                stepSize    = 0.0f;
    int                                  stepCount   = 0;
    std::vector<std::string>             stepNames;
    juce::String                         oscAddress  = "/param/0";

    /** Converts this entry to a ParameterControlInfo suitable for the control component. */
    JUCEAppBasics::ParameterControlInfo toParameterControlInfo(int index) const
    {
        JUCEAppBasics::ParameterControlInfo info;
        info.index        = index;
        info.name         = name;
        info.type         = controlType;
        info.minValue     = minValue;
        info.maxValue     = maxValue;
        info.stepSize     = stepSize;
        info.stepCount    = stepCount;
        info.stepNames    = stepNames;
        info.currentValue = minValue; // initialised to minimum
        return info;
    }

    bool operator==(const CustomParameterEntry& o) const noexcept
    {
        return name == o.name && controlType == o.controlType
            && minValue == o.minValue && maxValue == o.maxValue
            && stepSize == o.stepSize && stepCount == o.stepCount
            && stepNames == o.stepNames && oscAddress == o.oscAddress;
    }
    bool operator!=(const CustomParameterEntry& o) const noexcept { return !(*this == o); }
};

/**
 * @brief Top-level configuration block for the custom OSC parameter control subsystem.
 */
struct CustomParameterConfig
{
    juce::String                    oscRemoteHost  = "127.0.0.1";
    int                             oscSendPort    = 9001;
    int                             oscReceivePort = 9002;
    std::vector<CustomParameterEntry> parameters;

    bool operator==(const CustomParameterConfig& o) const noexcept
    {
        return oscRemoteHost == o.oscRemoteHost
            && oscSendPort    == o.oscSendPort
            && oscReceivePort == o.oscReceivePort
            && parameters     == o.parameters;
    }
    bool operator!=(const CustomParameterConfig& o) const noexcept { return !(*this == o); }
};
