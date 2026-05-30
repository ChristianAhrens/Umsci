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

#include "CustomParameterControlComponent.h"


CustomParameterControlComponent::CustomParameterControlComponent()
    : JUCEAppBasics::ParameterControlComponent()
{
}

CustomParameterControlComponent::~CustomParameterControlComponent() = default;

void CustomParameterControlComponent::paint(juce::Graphics& g)
{
    JUCEAppBasics::ParameterControlComponent::paint(g);
}

void CustomParameterControlComponent::setCustomConfig(const CustomParameterConfig& config)
{
    if (m_config == config)
        return;

    m_config = config;

    std::vector<JUCEAppBasics::ParameterControlInfo> infos;
    infos.reserve(config.parameters.size());
    for (int i = 0; i < int(config.parameters.size()); ++i)
        infos.push_back(config.parameters[i].toParameterControlInfo(i));

    setParameters(infos);
}

void CustomParameterControlComponent::setControlSize(int sizeIndex)
{
    switch (sizeIndex)
    {
    case 0:
        setControlsSize(JUCEAppBasics::ParameterControlComponent::ControlsSize::S);
        break;
    case 2:
        setControlsSize(JUCEAppBasics::ParameterControlComponent::ControlsSize::L);
        break;
    default:
        setControlsSize(JUCEAppBasics::ParameterControlComponent::ControlsSize::M);
        break;
    }
}
