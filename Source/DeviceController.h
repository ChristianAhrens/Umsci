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

#include <NanoOcp1.h>
#include <Ocp1ObjectDefinitions.h>
#include <Variant.h>


class DeviceController : public juce::Timer, public juce::MessageListener
{
public:
    enum State
    {
        Disconnected,
        Connecting,
        Subscribing,
        Subscribed,
		GetValues,
		Connected
    };

	enum DbDeviceModel
	{
		InvalidDev = 0,
		DS100,
		DS100D,
		DS100M,
		InvalidDev_max
	};

	enum MappingAreaId
	{
		InvalidMapId = -1,
		First = 1,
		Second,
		Third,
		Fourth,
		InvaliMapId_max
	};

	static constexpr std::uint16_t sc_MAX_INPUTS_CHANNELS = 128;
	static constexpr std::uint16_t sc_MAX_OUTPUT_CHANNELS = 64;
	static constexpr std::uint16_t sc_MAX_FUNCTION_GROUPS = 32;
	static constexpr std::uint16_t sc_MAX_REVERB_ZONES = 4;

	struct RemObjAddr
	{
		std::int16_t	pri;
		std::int16_t	sec;

		static constexpr std::int16_t sc_INV = 0;

		RemObjAddr()
		{
			pri = sc_INV;
			sec = sc_INV;
		};
		RemObjAddr(const RemObjAddr& rhs)
		{
			*this = rhs;
		};
		RemObjAddr(std::int16_t a, std::int16_t b)
		{
			pri = a;
			sec = b;
		};
		juce::String toNiceString() const
		{
			if (sc_INV != pri && sc_INV != sec)
				return juce::String(sec) + "/" + juce::String(pri);
			else if (sc_INV != pri)
				return juce::String(pri);
			else if (sc_INV != sec)
				return juce::String(sec);
			else
				return juce::String();
		}
		juce::String toString() const
		{
			return juce::String(pri) + "," + juce::String(sec);
		}
		static juce::String toString(const std::vector<RemObjAddr>& RemObjAddrs)
		{
			auto objectListString = juce::String();

			for (auto const& objectAddressing : RemObjAddrs)
				objectListString << objectAddressing.toString() << ";";

			return objectListString;
		}
		bool fromString(const juce::String& commaseparatedStringRepresentation)
		{
			juce::StringArray sa;
			auto tokens = sa.addTokens(commaseparatedStringRepresentation, ",", "");
			if (tokens != 2 || sa.size() != 2)
				return false;

			pri = std::int16_t(sa[0].getIntValue());
			sec = std::int16_t(sa[1].getIntValue());

			return true;
		}
		static RemObjAddr createFromString(const juce::String& commaseparatedStringRepresentation)
		{
			juce::StringArray sa;
			sa.addTokens(commaseparatedStringRepresentation.trimCharactersAtEnd(","), ",", "");
			if (sa.size() != 2)
				return RemObjAddr();

			return RemObjAddr(std::int16_t(sa[0].getIntValue()), std::int16_t(sa[1].getIntValue()));
		}
		static std::vector<RemObjAddr> createFromListString(const juce::String& objectListStringRepresentation)
		{
			auto remoteObjects = std::vector<RemObjAddr>();

			juce::StringArray sa;
			sa.addTokens(objectListStringRepresentation.trimCharactersAtEnd(";"), ";", "");

			for (auto const& commaseparatedStringRepresentation : sa)
				remoteObjects.push_back(RemObjAddr::createFromString(commaseparatedStringRepresentation));

			return remoteObjects;
		}
		bool operator==(const RemObjAddr& rhs) const
		{
			return (pri == rhs.pri) && (sec == rhs.sec);
		}
		bool operator!=(const RemObjAddr& rhs) const
		{
			return !(*this == rhs);
		}
		bool operator<(const RemObjAddr& rhs) const
		{
			return (pri < rhs.pri) || ((pri == rhs.pri) && (sec < rhs.sec));
		}
		bool operator>(const RemObjAddr& rhs) const
		{
			// not less and not equal is greater
			return (!(*this < rhs) && (*this != rhs));
		}
		RemObjAddr& operator=(const RemObjAddr& rhs)
		{
			if (this != &rhs)
			{
				pri = rhs.pri;
				sec = rhs.sec;
			}

			return *this;
		}

		JUCE_LEAK_DETECTOR(RemObjAddr)
	};

	struct RemoteObject
	{
		enum RemObjIdent
		{
			HeartbeatPing = 0,			/**< Hearbeat request (OSC-exclusive) without data content. */
			HeartbeatPong,				/**< Hearbeat answer (OSC-exclusive) without data content. */
			Invalid,					/**< Invalid remote object id. This is not the first
										   * value to allow iteration over enum starting
										   * here (e.g. to not show the user the internal-only ping/pong). */
			Fixed_GUID,
			Settings_DeviceName,
			Status_StatusText,
			Status_AudioNetworkSampleStatus,
			Error_GnrlErr,
			Error_ErrorText,
			MatrixInput_Mute,
			MatrixInput_Gain,
			MatrixInput_Delay,
			MatrixInput_DelayEnable,
			MatrixInput_EqEnable,
			MatrixInput_Polarity,
			MatrixInput_ChannelName,
			MatrixInput_LevelMeterPreMute,
			MatrixInput_LevelMeterPostMute,
			MatrixInput_ReverbSendGain,				/**< reverbsendgain remote object id. */
			MatrixNode_Enable,
			MatrixNode_Gain,
			MatrixNode_DelayEnable,
			MatrixNode_Delay,
			MatrixOutput_Mute,
			MatrixOutput_Gain,
			MatrixOutput_Delay,
			MatrixOutput_DelayEnable,
			MatrixOutput_EqEnable,
			MatrixOutput_Polarity,
			MatrixOutput_ChannelName,
			MatrixOutput_LevelMeterPreMute,
			MatrixOutput_LevelMeterPostMute,
			Positioning_SourceSpread,				/**< spread remote object id. */
			Positioning_SourceDelayMode,			/**< delaymode remote object id. */
			Positioning_SourcePosition_XY,
			Positioning_SourcePosition_X,
			Positioning_SourcePosition_Y,
			Positioning_SourcePosition,
			CoordinateMapping_SourcePosition_XY,	/**< combined xy position remote object id. */
			CoordinateMapping_SourcePosition_X,		/**< x position remote object id. */
			CoordinateMapping_SourcePosition_Y,		/**< y position remote object id. */
			CoordinateMapping_SourcePosition,		/**< combined xyz position remote object id. */
			MatrixSettings_ReverbRoomId,
			MatrixSettings_ReverbPredelayFactor,
			MatrixSettings_ReverbRearLevel,
			FunctionGroup_Name,
			FunctionGroup_Delay,
			FunctionGroup_SpreadFactor,
			ReverbInput_Gain,
			ReverbInputProcessing_Mute,
			ReverbInputProcessing_Gain,
			ReverbInputProcessing_EqEnable,
			ReverbInputProcessing_LevelMeter,
			Scene_SceneIndex,
			Scene_SceneName,
			Scene_SceneComment,
			Scene_Previous,
			Scene_Next,
			Scene_Recall,
			CoordinateMappingSettings_P1real,
			CoordinateMappingSettings_P2real,
			CoordinateMappingSettings_P3real,
			CoordinateMappingSettings_P4real,
			CoordinateMappingSettings_P1virtual,
			CoordinateMappingSettings_P3virtual,
			CoordinateMappingSettings_Flip,
			CoordinateMappingSettings_Name,
			Positioning_SpeakerPosition,			// 6-float loudspeaker position (x, y, z, hor, vert, rot)
			SoundObjectRouting_Mute,
			SoundObjectRouting_Gain,
			Device_Clear,
			InvalidMAX
		};

		RemObjIdent	Id;
		RemObjAddr	Addr;
		NanoOcp1::Variant		Var;

		RemoteObject() : Id(RemObjIdent::Invalid), Addr(RemObjAddr())
		{
		};
		RemoteObject(const RemoteObject& rhs)
		{
			*this = rhs;
		};
		RemoteObject(RemObjIdent id, RemObjAddr addr, NanoOcp1::Variant v)
			: Id(id), Addr(addr), Var(v)
		{
		};
		bool operator==(const RemoteObject& other) const
		{
			return (Id == other.Id && Addr == other.Addr && Var == other.Var);
		};
		bool operator!=(const RemoteObject& other) const
		{
			return !(*this == other);
		}
		bool operator<(const RemoteObject& other) const
		{
			return (!(*this > other) && (*this != other));
		}
		bool operator>(const RemoteObject& other) const
		{
			return (Id > other.Id) || ((Id == other.Id) && (Addr > other.Addr));
		}
		RemoteObject& operator=(const RemoteObject& other)
		{
			if (this != &other)
			{
				Id = other.Id;
				Addr = other.Addr;
				Var = other.Var;
			}

			return *this;
		}

		static juce::String GetObjectDescription(const RemObjIdent roi)
		{
			switch (roi)
			{
			case HeartbeatPing:
				return "PING";
			case HeartbeatPong:
				return "PONG";
			case Fixed_GUID:
				return "Fixed Guid";
			case Settings_DeviceName:
				return "Device Name";
			case Error_GnrlErr:
				return "General Error";
			case Error_ErrorText:
				return "Error Text";
			case Status_StatusText:
				return "Status Text";
			case Status_AudioNetworkSampleStatus:
				return "Status AudioNetworkSampleStatus";
			case MatrixInput_Mute:
				return "Matrix Input Mute";
			case MatrixInput_Gain:
				return "Matrix Input Gain";
			case MatrixInput_Delay:
				return "Matrix Input Delay";
			case MatrixInput_DelayEnable:
				return "Matrix Input DelayEnable";
			case MatrixInput_EqEnable:
				return "Matrix Input EqEnable";
			case MatrixInput_Polarity:
				return "Matrix Input Polarity";
			case MatrixInput_ChannelName:
				return "Matrix Input ChannelName";
			case MatrixInput_LevelMeterPreMute:
				return "Matrix Input LevelMeterPreMute";
			case MatrixInput_LevelMeterPostMute:
				return "Matrix Input LevelMeterPostMute";
			case MatrixNode_Enable:
				return "Matrix Node Enable";
			case MatrixNode_Gain:
				return "Matrix Node Gain";
			case MatrixNode_DelayEnable:
				return "Matrix Node DelayEnable";
			case MatrixNode_Delay:
				return "Matrix Node Delay";
			case MatrixOutput_Mute:
				return "Matrix Output Mute";
			case MatrixOutput_Gain:
				return "Matrix Output Gain";
			case MatrixOutput_Delay:
				return "Matrix Output Delay";
			case MatrixOutput_DelayEnable:
				return "Matrix Output DelayEnable";
			case MatrixOutput_EqEnable:
				return "Matrix Output EqEnable";
			case MatrixOutput_Polarity:
				return "Matrix Output Polarity";
			case MatrixOutput_ChannelName:
				return "Matrix Output ChannelName";
			case MatrixOutput_LevelMeterPreMute:
				return "Matrix Output LevelMeterPreMute";
			case MatrixOutput_LevelMeterPostMute:
				return "Matrix Output LevelMeterPostMute";
			case Positioning_SourceSpread:
				return "Sound Object Spread";
			case Positioning_SourceDelayMode:
				return "Sound Object Delay Mode";
			case Positioning_SourcePosition:
				return "Absolute Sound Object Position XYZ";
			case Positioning_SourcePosition_XY:
				return "Absolute Sound Object Position XY";
			case Positioning_SourcePosition_X:
				return "Absolute Sound Object Position X";
			case Positioning_SourcePosition_Y:
				return "Absolute Sound Object Position Y";
			case CoordinateMapping_SourcePosition:
				return "Mapped Sound Object Position XYZ";
			case CoordinateMapping_SourcePosition_XY:
				return "Mapped Sound Object Position XY";
			case CoordinateMapping_SourcePosition_X:
				return "Mapped Sound Object Position X";
			case CoordinateMapping_SourcePosition_Y:
				return "Mapped Sound Object Position Y";
			case MatrixSettings_ReverbRoomId:
				return "Matrix Settings ReverbRoomId";
			case MatrixSettings_ReverbPredelayFactor:
				return "Matrix Settings ReverbPredelayFactor";
			case MatrixSettings_ReverbRearLevel:
				return "Matrix Settings ReverbRearLevel";
			case MatrixInput_ReverbSendGain:
				return "Matrix Input ReverbSendGain";
			case FunctionGroup_Name:
				return "FunctionGroup Name";
			case FunctionGroup_Delay:
				return "FunctionGroup Delay";
			case FunctionGroup_SpreadFactor:
				return "FunctionGroup SpreadFactor";
			case ReverbInput_Gain:
				return "Reverb Input Gain";
			case ReverbInputProcessing_Mute:
				return "Reverb Input Processing Mute";
			case ReverbInputProcessing_Gain:
				return "Reverb Input Processing Gain";
			case ReverbInputProcessing_LevelMeter:
				return "Reverb Input Processing LevelMeter";
			case ReverbInputProcessing_EqEnable:
				return "Reverb Input Processing EqEnable";
			case Device_Clear:
				return "Device Clear";
			case Scene_Previous:
				return "Scene Previous";
			case Scene_Next:
				return "Scene Next";
			case Scene_Recall:
				return "Scene Recall";
			case Scene_SceneIndex:
				return "Scene SceneIndex";
			case Scene_SceneName:
				return "Scene SceneName";
			case Scene_SceneComment:
				return "Scene SceneComment";
			case CoordinateMappingSettings_P1real:
				return "Mapping Area P1 real";
			case CoordinateMappingSettings_P2real:
				return "Mapping Area P2 real";
			case CoordinateMappingSettings_P3real:
				return "Mapping Area P3 real";
			case CoordinateMappingSettings_P4real:
				return "Mapping Area P4 real";
			case CoordinateMappingSettings_P1virtual:
				return "Mapping Area P1 virt";
			case CoordinateMappingSettings_P3virtual:
				return "Mapping Area P3 virt";
			case CoordinateMappingSettings_Flip:
				return "Mapping Area flip";
			case CoordinateMappingSettings_Name:
				return "Mapping Area name";
			case Positioning_SpeakerPosition:
				return "Speaker Position";
			case SoundObjectRouting_Mute:
				return "Soundobject Routing Mute";
			case SoundObjectRouting_Gain:
				return "Soundobject Routing Gain";
			case Invalid:
				return "INVALID";
			default:
				jassertfalse;
				return "";
			}
		}

		JUCE_LEAK_DETECTOR(RemoteObject)
	};

	class StateChangeMessage : public juce::Message
	{
	public:
		StateChangeMessage(State s) { setState(s); };
		StateChangeMessage() = default;
		virtual ~StateChangeMessage() = default;

		void setState(State s) { m_state = s; };
		State getState() const { return m_state; };

	private:
		State m_state = Disconnected;
	};

	class RemoteObjectReceivedMessage : public juce::Message
	{
	public:
		RemoteObjectReceivedMessage(const RemoteObject& r) { setRemoteObject(r); };
		RemoteObjectReceivedMessage() = default;
		virtual ~RemoteObjectReceivedMessage() = default;

		void setRemoteObject(const RemoteObject& r) { m_remoteObject = r; };
		const RemoteObject& getRemoteObject() const { return m_remoteObject; };

	private:
		RemoteObject m_remoteObject;
	};

public:
    DeviceController();
    virtual ~DeviceController();

    JUCE_DECLARE_SINGLETON(DeviceController, false)

    //==============================================================================
    void timerCallback() override;

	//==============================================================================
	void handleMessage(const juce::Message& message) override;

    //==============================================================================
    bool connect();
    void disconnect();

	void setConnectionParameters(juce::IPAddress ip, int port, int timeoutMs = 150);
	const std::tuple<juce::IPAddress, int, int> getConnectionParameters();

    //==============================================================================
    const State getState() const;

	//==============================================================================
	bool SetActiveRemoteObjects(const std::vector<RemoteObject>& remObjs);
	const std::vector<RemoteObject>& GetActiveRemoteObjects();

    //==============================================================================
    std::function<bool(const RemoteObject&)>	onRemoteObjectReceived;
    std::function<void(const State state)>		onStateChanged;

private:
    //==============================================================================
    void setState(const State& s, juce::NotificationType notificationType = juce::sendNotification);

	//==============================================================================
	void CreateKnownONosMap();
	std::optional<std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>> GetObjectDefinition(const RemoteObject::RemObjIdent& roi, const RemObjAddr& addr, bool useDefinitionRemapping = false);

	//==============================================================================
	bool ocp1MessageReceived(const juce::MemoryBlock& data);
	bool CreateObjectSubscriptions();
	bool DeleteObjectSubscriptions();
	bool QueryObjectValues();
	bool QueryObjectValue(const RemoteObject::RemObjIdent roi, const RemObjAddr& addr);

	//==============================================================================
	void AddPendingSubscriptionHandle(const std::uint32_t handle);
	bool PopPendingSubscriptionHandle(const std::uint32_t handle);
	bool HasPendingSubscriptions();

	//==============================================================================
	void AddPendingGetValueHandle(const std::uint32_t handle, const std::uint32_t ONo);
	const std::uint32_t PopPendingGetValueHandle(const std::uint32_t handle);
	bool HasPendingGetValues();

	//==============================================================================
	void AddPendingSetValueHandle(const std::uint32_t handle, const std::uint32_t ONo, const int externalId);
	const std::uint32_t PopPendingSetValueHandle(const std::uint32_t handle, int& externalId);
	bool HasPendingSetValues();
	const std::optional<std::pair<std::uint32_t, int>> HasPendingSetValue(const std::uint32_t ONo);

	//==============================================================================
	void ClearPendingHandles();

	//==============================================================================
	bool UpdateObjectValue(NanoOcp1::Ocp1Notification* notifObj);
	bool UpdateObjectValue(const std::uint32_t ONo, NanoOcp1::Ocp1Response* responseObj);
	bool UpdateObjectValue(const RemoteObject::RemObjIdent roi, NanoOcp1::Ocp1Message* msgObj,
		const std::pair<RemObjAddr, NanoOcp1::Ocp1CommandDefinition>& objectDetails);

	//==============================================================================
	void ProcessGuidAndSubscribe(const juce::String newGuid);
	bool SetOcaRevisionAndDeviceModel(const juce::String& guid);

	//==============================================================================
	std::mutex                                              m_pendingHandlesMutex;
	std::vector<std::uint32_t>								m_pendingSubscriptionHandles;
	std::map<std::uint32_t, std::uint32_t>					m_pendingGetValueHandlesWithONo;
	std::map<std::uint32_t, std::pair<std::uint32_t, int>>	m_pendingSetValueHandlesWithONo;

	//==============================================================================
	std::map<RemoteObject::RemObjIdent, std::map<RemObjAddr, NanoOcp1::Ocp1CommandDefinition>>	m_ROIsToDefsMap;

	std::vector<RemoteObject>					m_activeRemoteObjects;

    //==============================================================================
    std::unique_ptr<NanoOcp1::NanoOcp1Client>   m_ocp1Connection;
	juce::IPAddress								m_ocp1IPAddress;
	int											m_ocp1Port;
	int											m_ocp1Timeout;

	juce::String								m_ocp1DeviceGUID;
	int											m_ocp1DeviceStackIdent = -1;
	DbDeviceModel								m_connectedDbDeviceModel = DbDeviceModel::InvalidDev;

    State                                       m_currentState = State::Disconnected;


};

