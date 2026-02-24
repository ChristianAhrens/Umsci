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


#include "UmsciAppConfiguration.h"
#include "DeviceController.h"


class UmsciControlComponent :   public juce::Component, public UmsciAppConfiguration::XmlConfigurableElement
{
public:
    UmsciControlComponent();
    ~UmsciControlComponent() override;

    //==============================================================================
    void resized() override;
    void paint(juce::Graphics& g) override;

    std::unique_ptr<XmlElement> createStateXml() override;
    bool setStateXml(XmlElement* stateXml) override;

    //==============================================================================
    const std::pair<int, int>& getOcp1IOSize();
    void setOcp1IOSize(const std::pair<int, int>& ioSize);

private:
    //==============================================================================
    void rebuildOcp1ObjectTree();

    void setRemoteObject(const DeviceController::RemoteObject& obj);

    //==============================================================================
    std::pair<int, int>                             m_ocp1IOSize;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciControlComponent)
};

