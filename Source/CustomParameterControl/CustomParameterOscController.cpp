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

#include "CustomParameterOscController.h"


CustomParameterOscController::CustomParameterOscController()
{
    m_receiver.addListener(this);
}

CustomParameterOscController::~CustomParameterOscController()
{
    m_receiver.removeListener(this);
    disconnect();
}

void CustomParameterOscController::setConfig(const CustomParameterConfig& config)
{
    auto wasConnected = m_connected;
    if (wasConnected)
        disconnect();

    m_config = config;

    if (wasConnected)
        connect();
}

bool CustomParameterOscController::connect()
{
    disconnect();

    if (m_config.parameters.empty())
        return true;

    bool senderOk   = m_sender.connect(m_config.oscRemoteHost, m_config.oscSendPort);
    bool receiverOk = (m_config.oscReceivePort > 0)
                      && m_receiver.connect(m_config.oscReceivePort);

    m_connected = senderOk;
    return senderOk && receiverOk;
}

void CustomParameterOscController::disconnect()
{
    m_sender.disconnect();
    m_receiver.disconnect();
    m_connected = false;
}

void CustomParameterOscController::sendParameterValue(int parameterIndex, float nativeValue)
{
    if (!m_connected)
        return;
    if (parameterIndex < 0 || parameterIndex >= int(m_config.parameters.size()))
        return;

    const auto& entry = m_config.parameters[parameterIndex];
    if (entry.oscAddress.isEmpty())
        return;

    juce::OSCMessage msg(entry.oscAddress);
    if (entry.controlType == JUCEAppBasics::ParameterControlType::Toggle)
        msg.addInt32(nativeValue > 0.5f ? 1 : 0);
    else
        msg.addFloat32(nativeValue);
    m_sender.send(msg);
}

void CustomParameterOscController::pollAllValues()
{
    if (!m_connected)
        return;

    for (const auto& entry : m_config.parameters)
    {
        if (entry.oscAddress.isNotEmpty())
            m_sender.send(juce::OSCMessage(entry.oscAddress));
    }
}

void CustomParameterOscController::oscMessageReceived(const juce::OSCMessage& message)
{
    auto address = message.getAddressPattern().toString();
    if (message.isEmpty())
        return;

    float value = 0.0f;
    if (message[0].isFloat32())
        value = message[0].getFloat32();
    else if (message[0].isInt32())
        value = float(message[0].getInt32());
    else
        return;

    for (int i = 0; i < int(m_config.parameters.size()); ++i)
    {
        if (m_config.parameters[i].oscAddress == address)
        {
            m_pendingValues[i] = value;
            triggerAsyncUpdate();
            break;
        }
    }
}

void CustomParameterOscController::handleAsyncUpdate()
{
    if (!onParameterValueReceived)
        return;

    for (auto& [index, value] : m_pendingValues)
        onParameterValueReceived(index, value);

    m_pendingValues.clear();
}
