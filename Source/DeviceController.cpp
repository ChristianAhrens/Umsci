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

#include "DeviceController.h"


JUCE_IMPLEMENT_SINGLETON(DeviceController)

/**
 * Wires NanoOcp1::SoundscapeController callbacks to post JUCE messages so that
 * onStateChanged and onRemoteObjectReceived are delivered on the message thread.
 */
DeviceController::DeviceController()
{
    m_ocp1IPAddress = juce::IPAddress("127.0.0.1");

    // Wire base-class callbacks → message-thread dispatch
    NanoOcp1::SoundscapeController::onStateChanged = [this](NanoOcp1::Ocp1Controller::State s)
    {
        postMessage(new StateChangeMessage(static_cast<State>(s)));
    };

    NanoOcp1::SoundscapeController::onRemoteObjectReceived = [this](const NanoOcp1::SoundscapeController::RemoteObject& ro) -> bool
    {
        postMessage(new RemoteObjectReceivedMessage(fromCtrlObj(ro)));
        return true;
    };
}

DeviceController::~DeviceController()
{
    disconnect();
    clearSingletonInstance();
}

//==============================================================================
void DeviceController::handleMessage(const juce::Message& message)
{
    if (auto const scm = dynamic_cast<const StateChangeMessage*>(&message))
    {
        if (onStateChanged)
            onStateChanged(scm->getState());
    }
    else if (auto const rorm = dynamic_cast<const RemoteObjectReceivedMessage*>(&message))
    {
        if (onRemoteObjectReceived)
            onRemoteObjectReceived(rorm->getRemoteObject());
    }
}

//==============================================================================
bool DeviceController::connect()
{
    if (getState() != State::Disconnected)
    {
        DBG(juce::String(__FUNCTION__) << " - nothing to do as we're not disconnected");
        return false;
    }
    DBG(__FUNCTION__);
    NanoOcp1::SoundscapeController::connect(m_ocp1IPAddress.toString().toStdString(), m_ocp1Port, m_ocp1Timeout);
    return true;
}

void DeviceController::disconnect()
{
    DBG(__FUNCTION__);
    NanoOcp1::SoundscapeController::disconnect();
}

void DeviceController::setConnectionParameters(juce::IPAddress ip, int port, int timeoutMs)
{
    DBG(juce::String(__FUNCTION__) << " " << ip.toString() << ":" << port);
    m_ocp1IPAddress = ip;
    m_ocp1Port      = port;
    jassert(0 < timeoutMs);
    m_ocp1Timeout   = timeoutMs;

    if (getState() != State::Disconnected)
    {
        disconnect();
        connect();
    }
}

const std::tuple<juce::IPAddress, int, int> DeviceController::getConnectionParameters()
{
    return { m_ocp1IPAddress, m_ocp1Port, m_ocp1Timeout };
}

void DeviceController::setDeviceIOSize(std::uint16_t inputs, std::uint16_t outputs)
{
    NanoOcp1::SoundscapeController::setDeviceIOSize(
        juce::jlimit(std::uint16_t(1), sc_MAX_INPUTS_CHANNELS, inputs),
        juce::jlimit(std::uint16_t(1), sc_MAX_OUTPUT_CHANNELS, outputs));
}

const DeviceController::State DeviceController::getState() const
{
    return static_cast<State>(NanoOcp1::SoundscapeController::getState());
}

//==============================================================================
bool DeviceController::SetActiveRemoteObjects(const std::vector<RemoteObject>& remObjs)
{
    std::vector<NanoOcp1::SoundscapeController::RemoteObject> ctrlObjs;
    ctrlObjs.reserve(remObjs.size());
    for (auto const& o : remObjs)
        ctrlObjs.push_back(toCtrlObj(o));
    return NanoOcp1::SoundscapeController::setActiveRemoteObjects(ctrlObjs);
}

const std::vector<DeviceController::RemoteObject>& DeviceController::GetActiveRemoteObjects()
{
    // Sync our cache from the base class on every call.
    auto const& base = NanoOcp1::SoundscapeController::getActiveRemoteObjects();
    m_activeRemoteObjects.clear();
    m_activeRemoteObjects.reserve(base.size());
    for (auto const& o : base)
        m_activeRemoteObjects.push_back(fromCtrlObj(o));
    return m_activeRemoteObjects;
}

bool DeviceController::SetObjectValue(const RemoteObject& remObj)
{
    DBG(juce::String(__FUNCTION__) << " " << RemoteObject::GetObjectDescription(remObj.Id)
        << "(" << remObj.Addr.toNiceString() << ")");
    return NanoOcp1::SoundscapeController::setObjectValue(toCtrlObj(remObj));
}

//==============================================================================
NanoOcp1::SoundscapeController::RemObjAddr DeviceController::toCtrlAddr(const RemObjAddr& a)
{
    return { a.pri, a.sec };
}

DeviceController::RemObjAddr DeviceController::fromCtrlAddr(const NanoOcp1::SoundscapeController::RemObjAddr& a)
{
    return { a.pri, a.sec };
}

NanoOcp1::SoundscapeController::RemoteObject DeviceController::toCtrlObj(const RemoteObject& o)
{
    return { static_cast<NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent>(o.Id),
             toCtrlAddr(o.Addr),
             o.Var };
}

DeviceController::RemoteObject DeviceController::fromCtrlObj(const NanoOcp1::SoundscapeController::RemoteObject& o)
{
    return { static_cast<RemoteObject::RemObjIdent>(o.Id),
             fromCtrlAddr(o.Addr),
             o.Var };
}
