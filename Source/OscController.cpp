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

#include "OscController.h"


OscController::OscController()
{
    for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        m_addresses[i] = UmsciExternalControlComponent::s_oscDefaultAddresses[i];

    m_oscReceiver.addListener(this);
}

OscController::~OscController()
{
    m_oscReceiver.removeListener(this);
    m_oscReceiver.disconnect();
}

void OscController::openPort(int port)
{
    if (m_port == port)
        return;

    m_oscReceiver.disconnect();
    m_port = port;

    if (port > 0)
        m_oscReceiver.connect(port);
}

void OscController::setAddress(UmsciExternalControlComponent::UpmixMidiParam param,
                                const juce::String& address)
{
    m_addresses[static_cast<int>(param)] = address;
}

juce::String OscController::getAddress(UmsciExternalControlComponent::UpmixMidiParam param) const
{
    return m_addresses[static_cast<int>(param)];
}

void OscController::oscMessageReceived(const juce::OSCMessage& message)
{
    auto address = message.getAddressPattern().toString();
    if (message.isEmpty() || !message[0].isFloat32())
        return;

    auto rawValue = message[0].getFloat32();

    for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
    {
        if (m_addresses[i] == address)
        {
            auto param = static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i);
            auto [minVal, maxVal] = UmsciExternalControlComponent::s_paramRanges[i];
            auto domainValue = juce::jlimit(minVal, maxVal, rawValue);

            if (onParamValueChanged)
                onParamValueChanged(param, domainValue);
            break;
        }
    }
}
