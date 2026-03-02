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

#include <Ocp1DS100ObjectDefinitions.h>


JUCE_IMPLEMENT_SINGLETON(DeviceController)

DeviceController::DeviceController()
{
    CreateKnownONosMap();

    m_ocp1IPAddress = juce::IPAddress("127.0.0.1");
    m_ocp1Port = 50014;

    m_ocp1Connection = std::make_unique<NanoOcp1::NanoOcp1Client>(m_ocp1IPAddress.toString(), m_ocp1Port, false);
    m_ocp1Connection->onConnectionEstablished = [=]() {
        stopTimer();

        m_ocp1DeviceGUID = "";
        m_ocp1DeviceStackIdent = -1;
        m_connectedDbDeviceModel = DbDeviceModel::InvalidDev;

        QueryObjectValue(RemoteObject::Fixed_GUID, {});
    };
    m_ocp1Connection->onConnectionLost = [=]() {
        DeleteObjectSubscriptions();
        ClearPendingHandles();

        m_ocp1DeviceGUID = "";
        m_ocp1DeviceStackIdent = -1;
        m_connectedDbDeviceModel = DbDeviceModel::InvalidDev;

        if (getState() > State::Connecting) // we have lost the connection while having just been connected or even fully running, so try to reestablish
        {
            postMessage(new StateChangeMessage(State::Connecting));

            // prepare to restart connection attempt after some large timeout, in case something got stuck...
            startTimer(m_ocp1Timeout * 20);
        }
    };
    m_ocp1Connection->onDataReceived = [=](const juce::MemoryBlock& data) {
        return ocp1MessageReceived(data);
    };
}

DeviceController::~DeviceController()
{
    disconnect();

    // this ensures that no dangling pointers are left when the
    // singleton is deleted.
    clearSingletonInstance();
}

void DeviceController::setState(const State& s, juce::NotificationType notificationType)
{
    std::function<juce::String(State)> stateToString = [=](State state) {
        switch (state)
        {
        case Disconnected:
            return "disco";
        case Connecting:
            return "cntng";
        case Subscribing:
            return "sbscrbng";
        case Subscribed:
            return "sbscrbd";
        case GetValues:
            return "gtvls";
        case Connected:
            return "cnctd";
        default:
            return "ERR";
        }
    };

    if (m_currentState != s)
    {
        DBG(juce::String(__FUNCTION__) << " " << stateToString(s));
        m_currentState = s;

        if (onStateChanged && juce::NotificationType::sendNotification == notificationType)
            onStateChanged(s);
    }
}

const DeviceController::State DeviceController::getState() const
{
    return m_currentState;
}

bool DeviceController::connect()
{
    if (State::Disconnected != getState())
    {
        DBG(juce::String(__FUNCTION__) << " - nothing to do as we're not disconnected");
        return false;
    }
    DBG(__FUNCTION__);

    setState(State::Connecting);

    // prepare to restart connection attempt after some large timeout, in case something got stuck...
    startTimer(m_ocp1Timeout * 20);

    timerCallback(); // avoid codeclones by manually trigger the timed connection attempt once

    return true;
}

void DeviceController::disconnect()
{
    DBG(__FUNCTION__);
    
    setState(State::Disconnected);

    if (m_ocp1Connection)
        m_ocp1Connection->disconnect(m_ocp1Timeout);

    stopTimer();
}

void DeviceController::setConnectionParameters(juce::IPAddress ip, int port, int timeoutMs)
{
    DBG(juce::String(__FUNCTION__) << " new connection params: " << ip.toString() << ":" << port << " (t:" << timeoutMs << ")");
    m_ocp1IPAddress = ip;
    m_ocp1Port = port;
    m_ocp1Timeout = timeoutMs;

    if (State::Disconnected != getState())
    {
        disconnect();
        connect();
    }
}

const std::tuple<juce::IPAddress, int, int> DeviceController::getConnectionParameters()
{
    return { m_ocp1IPAddress, m_ocp1Port, m_ocp1Timeout };
}

void DeviceController::timerCallback()
{
    if (State::Connecting == getState())
    {
        m_ocp1Connection->connectToSocket(m_ocp1IPAddress.toString(), m_ocp1Port, m_ocp1Timeout);
    }
    else
    {
        jassertfalse;
        disconnect();
    }
}

void DeviceController::handleMessage(const juce::Message& message)
{
    if (auto const scm = dynamic_cast<const StateChangeMessage*>(&message))
    {
        setState(scm->getState());
    }
    else if (auto const rorm = dynamic_cast<const RemoteObjectReceivedMessage*>(&message))
    {
        if (onRemoteObjectReceived)
            onRemoteObjectReceived(rorm->getRemoteObject());
    }
}

void DeviceController::CreateKnownONosMap()
{
    // definitions without channel and record
    m_ROIsToDefsMap[RemoteObject::Fixed_GUID][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Fixed_GUID();
    m_ROIsToDefsMap[RemoteObject::Settings_DeviceName][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Settings_DeviceName();
    m_ROIsToDefsMap[RemoteObject::Status_StatusText][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Status_StatusText();
    m_ROIsToDefsMap[RemoteObject::Status_AudioNetworkSampleStatus][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Status_AudioNetworkSampleStatus();
    m_ROIsToDefsMap[RemoteObject::Error_GnrlErr][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Error_GnrlErr();
    m_ROIsToDefsMap[RemoteObject::Error_ErrorText][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Error_ErrorText();

    m_ROIsToDefsMap[RemoteObject::MatrixSettings_ReverbRoomId][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRoomId();
    m_ROIsToDefsMap[RemoteObject::MatrixSettings_ReverbPredelayFactor][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor();
    m_ROIsToDefsMap[RemoteObject::MatrixSettings_ReverbRearLevel][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRearLevel();
    m_ROIsToDefsMap[RemoteObject::Scene_SceneIndex][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneIndex();
    m_ROIsToDefsMap[RemoteObject::Scene_SceneName][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneName();
    m_ROIsToDefsMap[RemoteObject::Scene_SceneComment][RemObjAddr()] = NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneComment();

    // definitions with channels: inputChannels (sound objects)
    for (std::int16_t first = 1; first <= sc_MAX_INPUTS_CHANNELS; first++)
    {
        auto roa = RemObjAddr(first, RemObjAddr::sc_INV);
        m_ROIsToDefsMap[RemoteObject::Positioning_SourcePosition][roa] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(first);
        m_ROIsToDefsMap[RemoteObject::Positioning_SourceSpread][roa] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(first);
        m_ROIsToDefsMap[RemoteObject::Positioning_SourceDelayMode][roa] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_Mute][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_Gain][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_Delay][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Delay(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_DelayEnable][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_DelayEnable(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_EqEnable][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_EqEnable(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_Polarity][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Polarity(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_ChannelName][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_LevelMeterPreMute][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPreMute(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_LevelMeterPostMute][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPostMute(first);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_ReverbSendGain][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(first);

        // definitions with channels and records: mapping areas
        for (std::uint16_t second = MappingAreaId::First; second <= MappingAreaId::Fourth; second++)
        {
            roa.sec = second;
            m_ROIsToDefsMap[RemoteObject::CoordinateMapping_SourcePosition][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(second, first);
        }

        // definitions with channels and records: function groups
        for (std::uint16_t second = 1; second <= sc_MAX_FUNCTION_GROUPS; second++)
        {
            roa.sec = second;
            m_ROIsToDefsMap[RemoteObject::SoundObjectRouting_Mute][roa] = NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Mute(second, first);
            m_ROIsToDefsMap[RemoteObject::SoundObjectRouting_Gain][roa] = NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Gain(second, first);
        }

        // definitions with channels and records but second parameter for output channels
        for (std::uint16_t second = 1; second <= sc_MAX_OUTPUT_CHANNELS; second++)
        {
            roa.sec = second;
            m_ROIsToDefsMap[RemoteObject::MatrixNode_Enable][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Enable(first, second);
            m_ROIsToDefsMap[RemoteObject::MatrixNode_Gain][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Gain(first, second);
            m_ROIsToDefsMap[RemoteObject::MatrixNode_Delay][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Delay(first, second);
            m_ROIsToDefsMap[RemoteObject::MatrixNode_DelayEnable][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_DelayEnable(first, second);
        }
    }

    // definitions with channels: matrix outputs
    for (std::uint16_t first = 1; first <= sc_MAX_OUTPUT_CHANNELS; first++)
    {
        auto roa = RemObjAddr(first, RemObjAddr::sc_INV);
        m_ROIsToDefsMap[RemoteObject::Positioning_SpeakerPosition][roa] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Speaker_Position(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_Mute][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_Gain][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_Delay][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Delay(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_DelayEnable][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_DelayEnable(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_EqEnable][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_EqEnable(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_Polarity][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Polarity(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_ChannelName][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_LevelMeterPreMute][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(first);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_LevelMeterPostMute][roa] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPostMute(first);
    }

    // definitions with channels: function groups
    for (std::uint16_t first = 1; first <= sc_MAX_FUNCTION_GROUPS; first++)
    {
        auto roa = RemObjAddr(first, RemObjAddr::sc_INV);
        m_ROIsToDefsMap[RemoteObject::FunctionGroup_Name][roa] = NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Name(first);
        m_ROIsToDefsMap[RemoteObject::FunctionGroup_Delay][roa] = NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Delay(first);
        m_ROIsToDefsMap[RemoteObject::FunctionGroup_SpreadFactor][roa] = NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_SpreadFactor(first);
    }

    // definitions with channels: en-space zones
    for (std::uint16_t first = 1; first <= sc_MAX_REVERB_ZONES; first++)
    {
        auto roa = RemObjAddr(first, RemObjAddr::sc_INV);
        m_ROIsToDefsMap[RemoteObject::ReverbInputProcessing_Mute][roa] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Mute(first);
        m_ROIsToDefsMap[RemoteObject::ReverbInputProcessing_Gain][roa] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Gain(first);
        m_ROIsToDefsMap[RemoteObject::ReverbInputProcessing_EqEnable][roa] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_EqEnable(first);
        m_ROIsToDefsMap[RemoteObject::ReverbInputProcessing_LevelMeter][roa] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_LevelMeter(first);

        // definitions with channels and records: en-space zones with zone as first parameter = channel and sound object as second parameter = record
        for (std::uint16_t second = 1; second <= sc_MAX_INPUTS_CHANNELS; second++)
        {
            roa.sec = second;
            m_ROIsToDefsMap[RemoteObject::ReverbInput_Gain][roa] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInput_Gain(second, first);
        }
    }

    // definitions with records: mapping areas
    for (std::uint16_t first = 1; first <= MappingAreaId::Fourth; first++)
    {
        auto roa = RemObjAddr(first, RemObjAddr::sc_INV);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P1real][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P1_real(first);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P2real][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P2_real(first);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P3real][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P3_real(first);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P4real][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P4_real(first);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P1virtual][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P1_virtual(first);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P3virtual][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P3_virtual(first);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_Flip][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_Flip(first);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_Name][roa] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_Name(first);
    }

    // Build reverse ONo -> (ROI, addr) lookup for O(1) dispatch in UpdateObjectValue.
    for (auto& roisKV : m_ROIsToDefsMap)
        for (auto& objDefKV : roisKV.second)
            m_ONoToROIMap[objDefKV.second.m_targetOno] = { roisKV.first, objDefKV.first };
}

std::optional<std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>> DeviceController::GetObjectDefinition(const RemoteObject::RemObjIdent& roi, const RemObjAddr& addr, bool useDefinitionRemapping)
{
    std::int32_t first = addr.pri;
    std::int32_t second = addr.sec;

    switch (roi)
    {
    case RemoteObject::Fixed_GUID:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Fixed_GUID());
    case RemoteObject::Settings_DeviceName:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Settings_DeviceName());
    case RemoteObject::Status_StatusText:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Status_StatusText());
    case RemoteObject::Status_AudioNetworkSampleStatus:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Status_AudioNetworkSampleStatus());
    case RemoteObject::Error_GnrlErr:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Error_GnrlErr());
    case RemoteObject::Error_ErrorText:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Error_ErrorText());
    case RemoteObject::CoordinateMappingSettings_Name:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_Name(first));
    case RemoteObject::CoordinateMappingSettings_Flip:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_Flip(first));
    case RemoteObject::CoordinateMappingSettings_P1real:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P1_real(first));
    case RemoteObject::CoordinateMappingSettings_P2real:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P2_real(first));
    case RemoteObject::CoordinateMappingSettings_P3real:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P3_real(first));
    case RemoteObject::CoordinateMappingSettings_P4real:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P4_real(first));
    case RemoteObject::CoordinateMappingSettings_P1virtual:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P1_virtual(first));
    case RemoteObject::CoordinateMappingSettings_P3virtual:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P3_virtual(first));
    case RemoteObject::Positioning_SourcePosition_XY:
    case RemoteObject::Positioning_SourcePosition_X:
    case RemoteObject::Positioning_SourcePosition_Y:
    {
        if (!useDefinitionRemapping)
        {
            DBG(juce::String(__FUNCTION__) + " skipping Positioning_SourcePosition X Y XY");
            return {};
        }
    }
    [[fallthrough]];
    case RemoteObject::Positioning_SourcePosition:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(first));
    case RemoteObject::CoordinateMapping_SourcePosition_XY:
    case RemoteObject::CoordinateMapping_SourcePosition_X:
    case RemoteObject::CoordinateMapping_SourcePosition_Y:
    {
        if (!useDefinitionRemapping)
        {
            DBG(juce::String(__FUNCTION__) + " skipping CoordinateMapping_SourcePosition X Y XY");
            return {};
        }
    }
    [[fallthrough]];
    case RemoteObject::CoordinateMapping_SourcePosition:return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(second, first));
    case RemoteObject::Positioning_SourceSpread:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(first));
    case RemoteObject::Positioning_SourceDelayMode:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(first));
    case RemoteObject::Positioning_SpeakerPosition:
        if (m_ocp1DeviceStackIdent >= 1) // newer oca revision needs newer oca object definition
            return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Speaker_Position(first));
        else
            return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Speaker_Position(first));
    case RemoteObject::FunctionGroup_Name:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Name(first));
    case RemoteObject::FunctionGroup_Delay:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Delay(first));
    case RemoteObject::FunctionGroup_SpreadFactor:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_SpreadFactor(first));
    case RemoteObject::MatrixInput_Mute:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(first));
    case RemoteObject::MatrixInput_Gain:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(first));
    case RemoteObject::MatrixInput_Delay:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Delay(first));
    case RemoteObject::MatrixInput_DelayEnable:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_DelayEnable(first));
    case RemoteObject::MatrixInput_EqEnable:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_EqEnable(first));
    case RemoteObject::MatrixInput_Polarity:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Polarity(first));
    case RemoteObject::MatrixInput_ChannelName:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(first));
    case RemoteObject::MatrixInput_LevelMeterPreMute:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPreMute(first));
    case RemoteObject::MatrixInput_LevelMeterPostMute:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPostMute(first));
    case RemoteObject::MatrixInput_ReverbSendGain:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(first));
    case RemoteObject::MatrixNode_Enable:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Enable(first, second));
    case RemoteObject::MatrixNode_Gain:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Gain(first, second));
    case RemoteObject::MatrixNode_Delay:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Delay(first, second));
    case RemoteObject::MatrixNode_DelayEnable:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_DelayEnable(first, second));
    case RemoteObject::MatrixOutput_Mute:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(first));
    case RemoteObject::MatrixOutput_Gain:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(first));
    case RemoteObject::MatrixOutput_Delay:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Delay(first));
    case RemoteObject::MatrixOutput_DelayEnable:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_DelayEnable(first));
    case RemoteObject::MatrixOutput_EqEnable:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_EqEnable(first));
    case RemoteObject::MatrixOutput_Polarity:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Polarity(first));
    case RemoteObject::MatrixOutput_ChannelName:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(first));
    case RemoteObject::MatrixOutput_LevelMeterPreMute:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(first));
    case RemoteObject::MatrixOutput_LevelMeterPostMute:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPostMute(first));
    case RemoteObject::MatrixSettings_ReverbRoomId:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRoomId());
    case RemoteObject::MatrixSettings_ReverbPredelayFactor:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor());
    case RemoteObject::MatrixSettings_ReverbRearLevel:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRearLevel());
    case RemoteObject::ReverbInput_Gain:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_ReverbInput_Gain(second, first));
    case RemoteObject::ReverbInputProcessing_Mute:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Mute(first));
    case RemoteObject::ReverbInputProcessing_Gain:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Gain(first));
    case RemoteObject::ReverbInputProcessing_EqEnable:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_EqEnable(first));
    case RemoteObject::ReverbInputProcessing_LevelMeter:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_LevelMeter(first));
    case RemoteObject::Scene_SceneIndex:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneIndex());
    case RemoteObject::Scene_SceneName:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneName());
    case RemoteObject::Scene_Previous:
    case RemoteObject::Scene_Next:
    case RemoteObject::Scene_Recall:
    {
        if (!useDefinitionRemapping)
            return {};
        else
            return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_SceneAgent());
    }
    case RemoteObject::Scene_SceneComment:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneComment());
    case RemoteObject::SoundObjectRouting_Mute:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Mute(second, first));
    case RemoteObject::SoundObjectRouting_Gain:
        return std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(new NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Gain(second, first));
    default:
        DBG(juce::String(__FUNCTION__) << " " << RemoteObject::GetObjectDescription(roi) << " -> not implmented");
        return {};
    }
}

bool DeviceController::ocp1MessageReceived(const juce::MemoryBlock& data)
{
    std::unique_ptr<NanoOcp1::Ocp1Message> msgObj = NanoOcp1::Ocp1Message::UnmarshalOcp1Message(data);
    if (msgObj)
    {
        switch (msgObj->GetMessageType())
        {
        case NanoOcp1::Ocp1Message::Notification:
        {
            NanoOcp1::Ocp1Notification* notifObj = static_cast<NanoOcp1::Ocp1Notification*>(msgObj.get());

            if (UpdateObjectValue(notifObj))
                return true;

            DBG(juce::String(__FUNCTION__) << " Got an unhandled OCA notification for ONo 0x"
                << juce::String::toHexString(notifObj->GetEmitterOno()));
            return false;
        }
        case NanoOcp1::Ocp1Message::Response:
        {
            NanoOcp1::Ocp1Response* responseObj = static_cast<NanoOcp1::Ocp1Response*>(msgObj.get());

            auto handle = responseObj->GetResponseHandle();
            if (responseObj->GetResponseStatus() != 0)
            {
                DBG(juce::String(__FUNCTION__) << " Got an OCA response (handle:" << NanoOcp1::HandleToString(handle) <<
                    ") with status " << NanoOcp1::StatusToString(responseObj->GetResponseStatus()));

                auto externalId = -1;
                PopPendingSubscriptionHandle(handle);
                PopPendingGetValueHandle(handle);
                PopPendingSetValueHandle(handle, externalId);

                return false;
            }
            else if (PopPendingSubscriptionHandle(handle))
            {
                if (!HasPendingSubscriptions())
                {
                    // All subscriptions were confirmed
                    //DBG(juce::String(__FUNCTION__) << " All NanoOcp1 subscriptions were confirmed (handle:"
                    //    << NanoOcp1::HandleToString(handle) << ")");

                    postMessage(new StateChangeMessage(State::Subscribed));
                    if (HasPendingGetValues())
                        postMessage(new StateChangeMessage(State::GetValues));
                    else
                        postMessage(new StateChangeMessage(State::Connected));
                }
                return true;
            }
            else
            {
                auto GetValONo = PopPendingGetValueHandle(handle);
                if (0x00 != GetValONo)
                {
                    if (!UpdateObjectValue(GetValONo, responseObj))
                    {
                        DBG(juce::String(__FUNCTION__) << " Got an unhandled OCA getvalue response message (handle:"
                            << NanoOcp1::HandleToString(handle) + ", targetONo:0x" << juce::String::toHexString(GetValONo) << ")");
                        return false;
                    }
                    else
                    {
                        if (!HasPendingGetValues())
                        {
                            // All getvalues were confirmed
                            //DBG(juce::String(__FUNCTION__) << " All pending NanoOcp1 getvalue commands were confirmed (handle:"
                            //    << NanoOcp1::HandleToString(handle) << ")");

                            if (!HasPendingSubscriptions())
                                postMessage(new StateChangeMessage(State::Connected));
                            else
                                postMessage(new StateChangeMessage(State::Subscribing));
                        }
                        return true;
                    }
                }

                auto externalId = -1;
                auto SetValONo = PopPendingSetValueHandle(handle, externalId);
                if (0x00 != SetValONo)
                {
                    if (!HasPendingSetValues())
                    {
                        // All subscriptions were confirmed
                        //DBG(juce::String(__FUNCTION__) << " All pending NanoOcp1 setvalue commands were confirmed (handle:"
                        //    << NanoOcp1::HandleToString(handle) << ")");
                    }
                    return true;
                }

                DBG(juce::String(__FUNCTION__) << " Got an OCA response for UNKNOWN handle " << NanoOcp1::HandleToString(handle) <<
                    "; status " << NanoOcp1::StatusToString(responseObj->GetResponseStatus()) <<
                    "; paramCount " << juce::String(responseObj->GetParamCount()));

                return false;
            }
        }
        case NanoOcp1::Ocp1Message::KeepAlive:
        {
            jassertfalse; //todo
            //// provide the received message to parent node
            //if (m_messageListener)
            //{
            //    m_messageListener->OnProtocolMessageReceived(this, RemoteObject::HeartbeatPong, RemoteObjectMessageData());
            //    return true;
            //}
            //else
            //    return false;
        }
        default:
            break;
        }
    }

    return false;
}

bool DeviceController::CreateObjectSubscriptions()
{
    if (!m_ocp1Connection || State::Disconnected == getState())
        return false;

    auto handle = std::uint32_t(0);
    auto success = true;

    for (auto const& activeObj : GetActiveRemoteObjects())
    {
        // Get the object definition
        auto objDefOpt = GetObjectDefinition(activeObj.Id, activeObj.Addr);

        // Sanity checks
        jassert(objDefOpt); // Missing implementation!
        if (!objDefOpt)
            return false;
        auto& objDef = objDefOpt.value();
        if (!objDef)
            return false;

        success = success && m_ocp1Connection->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef->AddSubscriptionCommand(), handle).GetMemoryBlock());
        //DBG(juce::String(__FUNCTION__) << " " << RemoteObject::GetObjectDescription(activeObj.Id) << "("
        //    << (activeObj.Addr.pri >= 0 ? (" pri:" + juce::String(activeObj.Addr.pri)) : "")
        //    << (activeObj.Addr.sec >= 0 ? (" sec:" + juce::String(activeObj.Addr.sec)) : "")
        //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");

        AddPendingSubscriptionHandle(handle);
    }

    postMessage(new StateChangeMessage(State::Subscribing));

    return success;
}

bool DeviceController::DeleteObjectSubscriptions()
{
    return false;
}

bool DeviceController::QueryObjectValues()
{
    if (!m_ocp1Connection || State::Disconnected == getState())
        return false;

    auto success = true;

    for (auto const& activeObj : GetActiveRemoteObjects())
    {
        success = QueryObjectValue(activeObj.Id, activeObj.Addr) && success;
    }

    postMessage(new StateChangeMessage(State::GetValues));

    return success;
}

bool DeviceController::QueryObjectValue(const RemoteObject::RemObjIdent roi, const RemObjAddr& addr)
{
    auto handle = std::uint32_t(0);

    // Get the object definition
    auto objDefOpt = GetObjectDefinition(roi, addr, true);

    // Sanity checks
    jassert(objDefOpt); // Missing implementation!
    if (!objDefOpt)
        return false;
    auto& objDef = objDefOpt.value();
    if (!objDef)
        return false;

    // Send GetValue command
    bool success = m_ocp1Connection->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef->GetValueCommand(), handle).GetMemoryBlock());
    AddPendingGetValueHandle(handle, objDef->m_targetOno);
    //DBG(juce::String(__FUNCTION__) + " " + RemoteObject::GetObjectDescription(roi) + "(handle: " + NanoOcp1::HandleToString(handle) + ")");
    return success;
}

bool DeviceController::SetObjectValue(const RemoteObject& remObj)
{
    if (!m_ocp1Connection || getState() != State::Connected)
        return false;

    auto handle = std::uint32_t(0);

    auto objDefOpt = GetObjectDefinition(remObj.Id, remObj.Addr, true);
    jassert(objDefOpt); // Missing implementation for this object type!
    if (!objDefOpt)
        return false;
    auto& objDef = objDefOpt.value();
    if (!objDef)
        return false;

    bool success = m_ocp1Connection->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef->SetValueCommand(remObj.Var), handle).GetMemoryBlock());
    AddPendingSetValueHandle(handle, objDef->m_targetOno, remObj.Addr.pri);

    DBG(juce::String(__FUNCTION__) << " " << RemoteObject::GetObjectDescription(remObj.Id)
        << "(" << remObj.Addr.toNiceString() << " handle:" << NanoOcp1::HandleToString(handle) << ")");

    return success;
}

void DeviceController::AddPendingSubscriptionHandle(const std::uint32_t handle)
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    //DBG(juce::String(__FUNCTION__)
    //    << " (handle:" << NanoOcp1::HandleToString(handle) << ")");
    m_pendingSubscriptionHandles.push_back(handle);
}

bool DeviceController::PopPendingSubscriptionHandle(const std::uint32_t handle)
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    auto it = std::find(m_pendingSubscriptionHandles.begin(), m_pendingSubscriptionHandles.end(), handle);
    if (it != m_pendingSubscriptionHandles.end())
    {
        //DBG(juce::String(__FUNCTION__)
        //    << " (handle:" << NanoOcp1::HandleToString(handle) << ")");
        m_pendingSubscriptionHandles.erase(it);
        return true;
    }
    else
        return false;
}

bool DeviceController::HasPendingSubscriptions()
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    return !m_pendingSubscriptionHandles.empty();
}

void DeviceController::AddPendingGetValueHandle(const std::uint32_t handle, const std::uint32_t ONo)
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    //DBG(juce::String(__FUNCTION__)
    //    << " (handle:" << NanoOcp1::HandleToString(handle)
    //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
    m_pendingGetValueHandlesWithONo.insert(std::make_pair(handle, ONo));
}

const std::uint32_t DeviceController::PopPendingGetValueHandle(const std::uint32_t handle)
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    auto it = std::find_if(m_pendingGetValueHandlesWithONo.begin(), m_pendingGetValueHandlesWithONo.end(), [handle](const auto& val) { return val.first == handle; });
    if (it != m_pendingGetValueHandlesWithONo.end())
    {
        auto ONo = it->second;
        //DBG(juce::String(__FUNCTION__)
        //    << " (handle:" << NanoOcp1::HandleToString(handle)
        //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
        m_pendingGetValueHandlesWithONo.erase(it);
        return ONo;
    }
    else
        return 0x00;
}

bool DeviceController::HasPendingGetValues()
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    return !m_pendingGetValueHandlesWithONo.empty();
}

void DeviceController::AddPendingSetValueHandle(const std::uint32_t handle, const std::uint32_t ONo, int externalId)
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    //DBG(juce::String(__FUNCTION__)
    //    << " (handle:" << NanoOcp1::HandleToString(handle)
    //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
    m_pendingSetValueHandlesWithONo.insert(std::make_pair(handle, std::make_pair(ONo, externalId)));
}

const std::uint32_t DeviceController::PopPendingSetValueHandle(const std::uint32_t handle, int& externalId)
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    auto it = std::find_if(m_pendingSetValueHandlesWithONo.begin(), m_pendingSetValueHandlesWithONo.end(), [handle](const auto& val) { return val.first == handle; });
    if (it != m_pendingSetValueHandlesWithONo.end())
    {
        auto ONo = it->second.first;
        externalId = it->second.second;
        //DBG(juce::String(__FUNCTION__)
        //    << " (handle:" << NanoOcp1::HandleToString(handle)
        //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
        m_pendingSetValueHandlesWithONo.erase(it);
        return ONo;
    }
    else
        return 0x00;
}

bool DeviceController::HasPendingSetValues()
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    return !m_pendingGetValueHandlesWithONo.empty();
}

const std::optional<std::pair<std::uint32_t, int>> DeviceController::HasPendingSetValue(const std::uint32_t ONo)
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    auto it = std::find_if(m_pendingSetValueHandlesWithONo.begin(), m_pendingSetValueHandlesWithONo.end(), [ONo](const auto& val) { return val.second.first == ONo; });
    if (it != m_pendingSetValueHandlesWithONo.end())
    {
        //DBG(juce::String(__FUNCTION__)
        //    << " (handle:" << juce::String(NanoOcp1::HandleToString(it->first))
        //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
        return std::optional<std::pair<std::uint32_t, int>>(std::make_pair(it->first, it->second.second));
    }
    else
        return std::optional<std::pair<std::uint32_t, int>>();
}

void DeviceController::ClearPendingHandles()
{
    std::lock_guard<std::mutex> l(m_pendingHandlesMutex); // NanoOcp callback on JUCE IPC thread, safety required!

    m_pendingSubscriptionHandles.clear();
    m_pendingGetValueHandlesWithONo.clear();
    m_pendingSetValueHandlesWithONo.clear();
}

bool DeviceController::UpdateObjectValue(NanoOcp1::Ocp1Notification* notifObj)
{
    auto it = m_ONoToROIMap.find(notifObj->GetEmitterOno());
    if (it != m_ONoToROIMap.end())
    {
        auto& [roi, addr] = it->second;
        return UpdateObjectValue(roi, dynamic_cast<NanoOcp1::Ocp1Message*>(notifObj),
                                 { addr, m_ROIsToDefsMap.at(roi).at(addr) });
    }

    return false;
}

bool DeviceController::UpdateObjectValue(const std::uint32_t ONo, NanoOcp1::Ocp1Response* responseObj)
{
    auto it = m_ONoToROIMap.find(ONo);
    if (it != m_ONoToROIMap.end())
    {
        auto& [roi, addr] = it->second;
        return UpdateObjectValue(roi, dynamic_cast<NanoOcp1::Ocp1Message*>(responseObj),
                                 { addr, m_ROIsToDefsMap.at(roi).at(addr) });
    }

    return false;
}

bool DeviceController::UpdateObjectValue(const RemoteObject::RemObjIdent roi, NanoOcp1::Ocp1Message* msgObj, const std::pair<RemObjAddr, NanoOcp1::Ocp1CommandDefinition>& objectDetails)
{
    //DBG(juce::String(__FUNCTION__)
    //    << " (targetONo:0x" << juce::String::toHexString(objectDetails.second.m_targetOno) << ")");

    NanoOcp1::Ocp1DataType datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_NONE;
    switch (roi)
    {
    case RemoteObject::Fixed_GUID:
        {
            bool ok = false;
            auto guid = NanoOcp1::DataToString(msgObj->GetParameterData(), &ok);
            if (ok)
                ProcessGuidAndSubscribe(guid);

            return ok;
        }
    case RemoteObject::Error_GnrlErr:
    case RemoteObject::MatrixInput_Polarity:
    case RemoteObject::MatrixOutput_Polarity:
    case RemoteObject::MatrixInput_Mute:
    case RemoteObject::MatrixOutput_Mute:
    case RemoteObject::ReverbInputProcessing_Mute:
    case RemoteObject::SoundObjectRouting_Mute:
        datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT8;
        break;
    case RemoteObject::CoordinateMappingSettings_Flip:
    case RemoteObject::MatrixNode_Enable:
    case RemoteObject::MatrixNode_DelayEnable:
    case RemoteObject::MatrixInput_DelayEnable:
    case RemoteObject::MatrixInput_EqEnable:
    case RemoteObject::MatrixOutput_DelayEnable:
    case RemoteObject::MatrixOutput_EqEnable:
    case RemoteObject::Positioning_SourceDelayMode:
    case RemoteObject::MatrixSettings_ReverbRoomId:
    case RemoteObject::ReverbInputProcessing_EqEnable:
        datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT16;
        break;
    case RemoteObject::Status_AudioNetworkSampleStatus:
        datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_INT32;
        break;
    case RemoteObject::MatrixNode_Delay:
    case RemoteObject::MatrixInput_Delay:
    case RemoteObject::MatrixOutput_Delay:
    case RemoteObject::FunctionGroup_Delay:
    case RemoteObject::MatrixNode_Gain:
    case RemoteObject::Positioning_SourceSpread:
    case RemoteObject::MatrixInput_ReverbSendGain:
    case RemoteObject::MatrixInput_Gain:
    case RemoteObject::MatrixInput_LevelMeterPreMute:
    case RemoteObject::MatrixInput_LevelMeterPostMute:
    case RemoteObject::MatrixOutput_Gain:
    case RemoteObject::MatrixOutput_LevelMeterPreMute:
    case RemoteObject::MatrixOutput_LevelMeterPostMute:
    case RemoteObject::MatrixSettings_ReverbPredelayFactor:
    case RemoteObject::MatrixSettings_ReverbRearLevel:
    case RemoteObject::FunctionGroup_SpreadFactor:
    case RemoteObject::ReverbInput_Gain:
    case RemoteObject::ReverbInputProcessing_Gain:
    case RemoteObject::ReverbInputProcessing_LevelMeter:
    case RemoteObject::SoundObjectRouting_Gain:
        datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_FLOAT32;
        break;
    case RemoteObject::CoordinateMappingSettings_Name:
    case RemoteObject::Settings_DeviceName:
    case RemoteObject::Status_StatusText:
    case RemoteObject::Error_ErrorText:
    case RemoteObject::MatrixInput_ChannelName:
    case RemoteObject::MatrixOutput_ChannelName:
    case RemoteObject::Scene_SceneIndex:
    case RemoteObject::Scene_SceneName:
    case RemoteObject::Scene_SceneComment:
    case RemoteObject::FunctionGroup_Name:
        datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_STRING;
        break;
    case RemoteObject::CoordinateMapping_SourcePosition:
    case RemoteObject::Positioning_SpeakerPosition:
    case RemoteObject::Positioning_SourcePosition:
    case RemoteObject::CoordinateMappingSettings_P1real:
    case RemoteObject::CoordinateMappingSettings_P2real:
    case RemoteObject::CoordinateMappingSettings_P3real:
    case RemoteObject::CoordinateMappingSettings_P4real:
    case RemoteObject::CoordinateMappingSettings_P1virtual:
    case RemoteObject::CoordinateMappingSettings_P3virtual:
        datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_DB_POSITION;
        break;
    default:
        DBG(juce::String(__FUNCTION__) << " unknown: " << RemoteObject::GetObjectDescription(roi)
            << " (" << static_cast<int>(objectDetails.first.pri) << "," << static_cast<int>(objectDetails.first.sec) << ") ");
        return false;
    }

    auto val = NanoOcp1::Variant(msgObj->GetParameterData(), datatype);
    auto ro = RemoteObject(roi, objectDetails.first, val);
    postMessage(new RemoteObjectReceivedMessage(ro));

    return true;
}

const std::vector<DeviceController::RemoteObject>& DeviceController::GetActiveRemoteObjects()
{
    return m_activeRemoteObjects;
}

bool DeviceController::SetActiveRemoteObjects(const std::vector<DeviceController::RemoteObject>& remObjs)
{
    if (getState() != DeviceController::State::Disconnected)
        return false;

    m_activeRemoteObjects = remObjs;

    return true;
}

void DeviceController::ProcessGuidAndSubscribe(const juce::String newGuid)
{
    if (newGuid == m_ocp1DeviceGUID) // on new connection guid is reset so when receiving a guid again the connection/subscriptions should already be established
        return;

    if (SetOcaRevisionAndDeviceModel(newGuid)) // validate guid and determines revision and device model
        m_ocp1DeviceGUID = newGuid;
    else
        return; // close connection ?

    auto roa = RemObjAddr(RemObjAddr::sc_INV, RemObjAddr::sc_INV);
    if (m_ocp1DeviceStackIdent >= 1) // update internal map with the right definitions
    {
        for (std::uint16_t first = 1; first <= sc_MAX_OUTPUT_CHANNELS; first++)
        {
            roa.pri = first;
            m_ROIsToDefsMap[RemoteObject::Positioning_SpeakerPosition][roa] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Speaker_Position(first);
        }
    }
    else
    {
        for (std::uint16_t first = 1; first <= sc_MAX_OUTPUT_CHANNELS; first++)
        {
            roa.pri = first;
            m_ROIsToDefsMap[RemoteObject::Positioning_SpeakerPosition][roa] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Speaker_Position(first);
        }
    }
    // after guid and device is known create subscriptions
    CreateObjectSubscriptions();
    QueryObjectValues();
}

bool DeviceController::SetOcaRevisionAndDeviceModel(const juce::String& guid)
{
    if (guid.length() != 8) // d&b guids as string are 8 characters
        return false;
    if (!guid.startsWith("DB00")) // identify d&b device
        return false;

    int deviceBytes = 2;
    DbDeviceModel dbDeviceModel; // last two characters decide the model
    if (guid.getLastCharacters(deviceBytes) == "D0") // DS100
        dbDeviceModel = DbDeviceModel::DS100;
    else if (guid.getLastCharacters(deviceBytes) == "D1") // DS100D
        dbDeviceModel = DbDeviceModel::DS100D;
    else if (guid.getLastCharacters(deviceBytes) == "D2") // DS100M
        dbDeviceModel = DbDeviceModel::DS100M;
    else
        return false;

    int versionBytesIndexStart = 4; // in the Guid the version is after "DB00" and has two bytes/characters
    auto versionChars = guid.substring(versionBytesIndexStart, versionBytesIndexStart + 2); // will get two characters
    int ocp1DeviceStackIdent;
    switch (dbDeviceModel)
    {
    case DbDeviceModel::DS100:
        if (versionChars >= "0C") // DS100 added scalability with FW version "0C"
            ocp1DeviceStackIdent = 1;
        else
            ocp1DeviceStackIdent = 0;
        DBG(juce::String(__FUNCTION__) << " detected DS100 (ocp stack:" << ocp1DeviceStackIdent << ") " << guid);
        break;
    case DbDeviceModel::DS100D:
        ocp1DeviceStackIdent = 1; // this was implemented pre-release of DS100D and assuming there will be no FW-version without scalability
        DBG(juce::String(__FUNCTION__) << " detected DS100D (ocp stack:" << ocp1DeviceStackIdent << ") " << guid);
        break;
    case DbDeviceModel::DS100M:
        if (versionChars >= "02") // DS100M added scalability with FW version "02"
            ocp1DeviceStackIdent = 1;
        else
            ocp1DeviceStackIdent = 0;
        DBG(juce::String(__FUNCTION__) << " detected DS100M (ocp stack:" << ocp1DeviceStackIdent << ") " << guid);
        break;
    default:
        return false;

    }
    m_connectedDbDeviceModel = dbDeviceModel;
    m_ocp1DeviceStackIdent = ocp1DeviceStackIdent;
    return true;
}

