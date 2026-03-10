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


/**
 * @class DeviceController
 * @brief Singleton that owns the OCP.1 (AES70) TCP connection to a d&b audiotechnik
 *        DS100 device and exposes a high-level Remote-Object API to the rest of Umsci.
 *
 * ## Overview
 * OCP.1 is a binary protocol defined in the AES70 standard.  Every controllable
 * parameter on the device lives at a fixed 32-bit Object Number (ONo) and can be
 * subscribed to, queried, or set.  This class hides all that complexity behind the
 * RemoteObject/RemObjIdent/RemObjAddr vocabulary that the rest of the application uses.
 *
 * ## Connection lifecycle
 * ```
 * Disconnected ─connect()──► Connecting ──TCP established──► (queries GUID)
 *   ▲                                                              │
 *   └──disconnect()──────────────────── Connected ◄──── Subscribed ◄── Subscribing
 *                                               ▲
 *                                          GetValues ──all responses──┘
 * ```
 * State transitions are posted as `StateChangeMessage` objects to the JUCE message
 * thread so that `onStateChanged` is always called on the message thread.
 *
 * ## Addressing
 * Most DS100 objects are indexed by a *primary* channel (1-based) and sometimes a
 * *secondary* record (mapping area 1-4, output channel, function group, etc.).
 * These two dimensions are captured in RemObjAddr::pri and RemObjAddr::sec.
 * Objects with no channel addressing use RemObjAddr() (both fields == sc_INV == 0).
 *
 * ## Threading
 * NanoOcp1::NanoOcp1Client fires `onDataReceived` on the JUCE IPC/socket thread.
 * `ocp1MessageReceived()` runs there.  State changes and decoded RemoteObjects are
 * dispatched to the message thread via `juce::MessageListener::postMessage()`.
 *
 * ## How to add a new subscribed object
 * 1. Add an entry to `RemObjIdent`.
 * 2. Handle it in `GetObjectDescription()`.
 * 3. Add it to `CreateKnownONosMap()` with the right `dbOcaObjectDef_*` factory.
 * 4. Add it to `GetObjectDefinition()` to allow on-demand definition creation.
 * 5. Add it to the datatype switch in `UpdateObjectValue()`.
 *
 * ## DS100 hardware background
 * The DS100 is a *signal engine* by d&b audiotechnik: it combines an audio matrix,
 * a convolution reverb engine (En-Space), and a spatialization algorithm (En-Scene)
 * to drive a distributed d&b loudspeaker system.
 *
 * - **Matrix inputs/outputs** — audio channels accepted from (inputs) or sent to
 *   (outputs) the audio network (Dante or Milan).  Internal routing and processing
 *   are fully configurable via AES70/OCA/OCP.1.  The available I/O count depends on
 *   the active software license (M / L / XL).
 * - **Sound objects** — when En-Scene (the spatialization algorithm) is activated for
 *   a matrix input, it controls the matrix nodes to create a spatially specific
 *   routing of that input to the connected loudspeaker outputs.  This turns the
 *   matrix input into a positionable "sound object" with X/Y/Z coordinates, spread,
 *   and delay mode.
 * - **Function groups** — logical groups of matrix outputs, each corresponding to a
 *   distinct subset of loudspeakers in the system (e.g. main PA, subwoofers, ceiling,
 *   surround).  Grouping allows users to manage per-group delay and spread without
 *   addressing individual outputs.
 * - **En-Space zones** — the four processing zones for the convolution reverb engine's
 *   early-reflection algorithm.  Three zones (Left, Center, Right) cover the stage
 *   area; one zone (Audience) covers the rest of the venue.  The zone assignment
 *   tells the algorithm which set of impulse responses to apply for each position.
 * - **Coordinate mapping** — En-Scene can optionally transform sound-object positions
 *   through a virtual-to-real coordinate mapping (up to four independent areas,
 *   each defined by corner-point pairs) before driving the routing.
 */
class DeviceController : public juce::Timer, public juce::MessageListener
{
public:
    /**
     * @brief Represents the logical phase of the OCP.1 connection.
     *
     * Transitions always flow in order from Disconnected toward Connected; they
     * never skip steps.  The reverse direction (toward Disconnected) happens only
     * when `disconnect()` is called explicitly or the TCP connection is lost.
     */
    enum State
    {
        Disconnected, ///< No TCP connection exists; no resources are allocated.
        Connecting,   ///< TCP connect in progress; timer retries until success.
        Subscribing,  ///< AddSubscription commands sent, awaiting acknowledgements.
        Subscribed,   ///< All subscriptions confirmed; GetValue queries still pending.
        GetValues,    ///< GetValue responses being collected.
        Connected     ///< All subscriptions confirmed and all initial values received.
    };

    /**
     * @brief Identifies which DS100 hardware variant is connected.
     *
     * Detected automatically from the 8-character device GUID returned by the
     * `Fixed_GUID` OCA object immediately after TCP connection.  The model
     * affects which OCA object definitions are used for speaker positions (see
     * `SetOcaRevisionAndDeviceModel()` and `ProcessGuidAndSubscribe()`).
     *
     * Hardware variants:
     * - **DS100**  — the original Dante-based machine (legacy audio network).
     * - **DS100D** — updated Dante machine (newer OCA revision; all known firmware
     *                releases include scalability, so stack ident is always 1).
     * - **DS100M** — Milan audio-network enabled machine (newer protocol standard,
     *                scalability added from firmware version "02" onward).
     */
	enum DbDeviceModel
	{
		InvalidDev  = 0, ///< Not yet determined or unsupported device.
		DS100,           ///< Standard DS100.
		DS100D,          ///< DS100D (Dante network audio variant).
		DS100M,          ///< DS100M (Milan network audio variant).
		InvalidDev_max
	};

    /**
     * @brief Identifies a DS100 coordinate-mapping area (1–4).
     *
     * En-Scene supports up to four independent coordinate-mapping areas.  Each area
     * defines a virtual-to-real transform via corner-point pairs (P1/P2/P3/P4 real
     * and P1/P3 virtual), enabling a single sound object to be positioned in
     * different virtual coordinate spaces that are each mapped to different parts
     * of the physical room.
     *
     * Each sound object can have a position in every mapping area simultaneously.
     * Used as the `sec` (secondary/record) dimension of RemObjAddr for
     * `CoordinateMapping_SourcePosition` and `CoordinateMappingSettings_*` objects.
     */
	enum MappingAreaId
	{
		InvalidMapId = -1, ///< Sentinel for "no mapping area".
		First  = 1,        ///< Mapping area 1.
		Second,            ///< Mapping area 2.
		Third,             ///< Mapping area 3.
		Fourth,            ///< Mapping area 4.
		InvaliMapId_max
	};

	static constexpr std::uint16_t sc_MAX_INPUTS_CHANNELS = 128;
	static constexpr std::uint16_t sc_MAX_OUTPUT_CHANNELS = 64;
	static constexpr std::uint16_t sc_MAX_FUNCTION_GROUPS = 32;
	static constexpr std::uint16_t sc_MAX_REVERB_ZONES = 4;

    /**
     * @brief Two-dimensional address of a remote object on the DS100.
     *
     * Most DS100 OCA objects are indexed by one or two integers:
     * - @b pri (primary) — channel number, typically 1-based.
     *   For matrix inputs this is the sound-object/input-channel index (1–128).
     *   For matrix outputs this is the output/speaker index (1–64).
     *   For function groups / reverb zones this is the group/zone index.
     * - @b sec (secondary / record) — used only where a second dimension exists:
     *   mapping area index (1–4) for CoordinateMapping objects,
     *   output channel for MatrixNode (cross-point) objects,
     *   function-group index for SoundObjectRouting objects.
     *
     * When a dimension is unused, the field is set to `sc_INV` (== 0), which
     * is the "invalid / not applicable" sentinel.
     */
	struct RemObjAddr
	{
		std::int16_t	pri; ///< Primary index (channel, speaker, group, zone…). 0 = not used.
		std::int16_t	sec; ///< Secondary index (mapping area, output ch, group…). 0 = not used.

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

    /**
     * @brief A fully-qualified remote parameter including its type, address, and current value.
     *
     * `RemoteObject` is the currency that flows between DeviceController and the rest of
     * Umsci.  It bundles together:
     * - `Id`  — *what* parameter (RemObjIdent enum),
     * - `Addr` — *which* channel/record it belongs to (RemObjAddr),
     * - `Var`  — the current value as a type-erased NanoOcp1::Variant.
     *
     * Callers receive RemoteObjects via the `onRemoteObjectReceived` callback and
     * send them via `SetObjectValue()`.
     */
	struct RemoteObject
	{
        /**
         * @brief Enumerates every controllable or monitorable parameter on the DS100.
         *
         * The identifiers roughly mirror the OCA object tree on the device.
         * Objects in the `Matrix*` family map to the DS100's internal audio matrix.
         * Objects in the `Positioning_*` family control/report spatial attributes of
         * sound objects and loudspeakers.
         * Objects in the `CoordinateMapping_*` family deal with virtual-to-real mapping.
         *
         * Special IDs:
         * - `HeartbeatPing/Pong` — used by OSC-based protocols, not by OCP.1.
         * - `Fixed_GUID` — read-only string; queried first on every connection to
         *   determine the device model and firmware OCA revision before subscribing
         *   to any other objects (see `ProcessGuidAndSubscribe()`).
         *
         * Parameter families at a glance:
         * - `MatrixInput_*` — properties of a DS100 matrix input channel (sound object
         *   channel): mute, gain, delay, EQ enable, polarity, name, level meters,
         *   reverb send gain.
         * - `MatrixOutput_*` — same properties on the output side (loudspeaker channel).
         * - `MatrixNode_*` — cross-point properties of the routing matrix (input × output):
         *   enable, gain, delay, delay enable.
         * - `Positioning_Source*` — En-Scene sound-object spatial attributes: absolute
         *   XYZ position, spread (0 = point source, 1 = full spread), delay mode
         *   (off / compensate / reflect).
         * - `Positioning_SpeakerPosition` — fixed loudspeaker 6-DOF position reported
         *   by the DS100 (X, Y, Z + orientation angles); read-only from the device.
         * - `CoordinateMapping_*` — En-Scene sound-object positions in *mapped* (virtual)
         *   coordinates for each of the four mapping areas.
         * - `CoordinateMappingSettings_*` — the corner-point pairs (P1–P4 real, P1/P3
         *   virtual) that define each mapping area's transform, plus its name and flip flag.
         * - `MatrixSettings_*` — global En-Space reverb settings: room ID, pre-delay
         *   factor, rear level.
         * - `ReverbInput_Gain` — per-sound-object send gain into an En-Space zone.
         * - `ReverbInputProcessing_*` — per En-Space zone properties: mute, gain,
         *   EQ enable, level meter.
         * - `FunctionGroup_*` — properties of a function group (logical loudspeaker set):
         *   name, delay, spread factor.
         * - `SoundObjectRouting_*` — per-sound-object routing into a function group: mute,
         *   gain.
         * - `Scene_*` — scene management: current index, name, comment, and the
         *   previous/next/recall action commands.
         * - `Settings_DeviceName` / `Status_*` / `Error_*` — device-level metadata.
         *
         * @note [MANUAL CONTEXT NEEDED] A table mapping each identifier to its exact
         *       OCA class name, ONo, and value units/range would help when debugging
         *       raw OCA traffic.  This information is in Ocp1DS100ObjectDefinitions.h.
         */
		enum RemObjIdent
		{
			HeartbeatPing = 0,			/**< Hearbeat request (OSC-exclusive) without data content. */
			HeartbeatPong,				/**< Hearbeat answer (OSC-exclusive) without data content. */
			Invalid,					/**< Invalid remote object id. This is not the first
										   * value to allow iteration over enum starting
										   * here (e.g. to not show the user the internal-only ping/pong). */
			Fixed_GUID, ///< Read-only 8-char device GUID; queried before any subscriptions.
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

    /**
     * @brief Carries a State value across the thread boundary via JUCE's message queue.
     *
     * `ocp1MessageReceived()` runs on the JUCE IPC/socket thread.  Calling
     * `setState()` directly from there would invoke `onStateChanged` on the wrong
     * thread.  Instead, a `StateChangeMessage` is posted to the JUCE message thread
     * via `postMessage()`, so `handleMessage()` invokes `setState()` — and therefore
     * `onStateChanged` — safely on the message thread.
     */
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

    /**
     * @brief Carries a decoded RemoteObject across the thread boundary via JUCE's message queue.
     *
     * Analogous to `StateChangeMessage`: a fully decoded `RemoteObject` is wrapped
     * in this message and posted to the message thread so that `onRemoteObjectReceived`
     * is always invoked on the JUCE message thread, regardless of which thread the
     * OCP.1 data callback fired on.
     */
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
    /** @brief juce::Timer callback — retries `connectToSocket()` while in Connecting state. */
    void timerCallback() override;

	//==============================================================================
    /**
     * @brief juce::MessageListener callback — dispatches messages posted from the socket thread.
     *
     * Handles `StateChangeMessage` (calls `setState()`) and
     * `RemoteObjectReceivedMessage` (calls `onRemoteObjectReceived`).
     * Always invoked on the JUCE message thread.
     */
	void handleMessage(const juce::Message& message) override;

    //==============================================================================
    /**
     * @brief Initiates the TCP connection.  No-op if not in Disconnected state.
     * @return true if the connection attempt was started.
     */
    bool connect();

    /**
     * @brief Closes the TCP connection and resets state to Disconnected.
     * Safe to call from any state, including already-Disconnected.
     */
    void disconnect();

    /**
     * @brief Updates connection parameters and reconnects if currently connected.
     * @param ip        Target device IP address.
     * @param port      OCP.1 TCP port (DS100 default: 50014).
     * @param timeoutMs Socket connect/send timeout in milliseconds.
     */
	void setConnectionParameters(juce::IPAddress ip, int port, int timeoutMs = 150);

    /**
     * @brief Returns the current connection parameters as {ip, port, timeoutMs}.
     */
	const std::tuple<juce::IPAddress, int, int> getConnectionParameters();

    //==============================================================================
    /** @brief Returns the current connection/subscription state. */
    const State getState() const;

	//==============================================================================
    /**
     * @brief Sets the list of remote objects to subscribe to on the next connection.
     *
     * Must be called while in Disconnected state (returns false otherwise).
     * The list is used by `CreateObjectSubscriptions()` and `QueryObjectValues()`
     * once the GUID handshake completes.  Objects with `RemObjIdent::Fixed_GUID`
     * are handled specially and do not need to be included here.
     *
     * @param remObjs  Desired subscriptions (id + addr pairs; Var field is ignored).
     * @return true on success; false if not currently Disconnected.
     */
	bool SetActiveRemoteObjects(const std::vector<RemoteObject>& remObjs);

    /** @brief Returns the active subscription list last set by SetActiveRemoteObjects(). */
	const std::vector<RemoteObject>& GetActiveRemoteObjects();

	//==============================================================================
    /**
     * @brief Sends a SetValue command to the device for the given remote object.
     *
     * Builds the appropriate OCA SetValue command from `remObj.Id`, `remObj.Addr`,
     * and `remObj.Var`, sends it over TCP, and records the pending handle so the
     * response can be matched.  Only valid in Connected state.
     *
     * @param remObj  Object to set, with `Var` populated with the desired value.
     * @return true if the command was sent successfully.
     */
	bool SetObjectValue(const RemoteObject& remObj);

    //==============================================================================
    /**
     * @brief Called on the JUCE message thread when a notification or get-value
     *        response is decoded for a subscribed remote object.
     *
     * Set this callback before calling `connect()`.  The RemoteObject's `Var` field
     * contains the decoded value.
     */
    std::function<bool(const RemoteObject&)>	onRemoteObjectReceived;

    /**
     * @brief Called on the JUCE message thread whenever the connection state changes.
     *
     * Useful for updating UI elements (e.g. enable/disable controls when Connected,
     * show a progress indicator when Subscribing, etc.).
     */
    std::function<void(const State state)>		onStateChanged;

private:
    //==============================================================================
    /**
     * @brief Updates `m_currentState` and (if sendNotification) fires `onStateChanged`.
     *
     * No-op if the new state equals the current state.  The `dontSendNotification`
     * overload is used internally when building subscriptions so that intermediate
     * transitions don't confuse callers.
     */
    void setState(const State& s, juce::NotificationType notificationType = juce::sendNotification);

	//==============================================================================
    /**
     * @brief Pre-builds the full ROI→address→definition map for every DS100 parameter.
     *
     * Called once in the constructor.  Populates `m_ROIsToDefsMap` by iterating
     * over every possible channel/record combination for all parameter types, and
     * then builds the reverse-lookup `m_ONoToROIMap` (ONo → {ROI, addr}) to allow
     * O(1) dispatch when incoming OCA messages are received.
     *
     * Speaker-position definitions may be overwritten later by
     * `ProcessGuidAndSubscribe()` once the device's OCA revision is known.
     */
	void CreateKnownONosMap();

    /**
     * @brief Returns a freshly allocated `Ocp1CommandDefinition` for the given object.
     *
     * @param roi                   Which parameter type.
     * @param addr                  Channel/record address.
     * @param useDefinitionRemapping  When true, X/Y/XY position variants are remapped
     *                              to their parent XYZ definition (the DS100 only
     *                              exposes a combined XYZ object; the split variants
     *                              are a Umsci-level convenience that maps to the same
     *                              OCA object).  Scene action commands (Previous/Next/
     *                              Recall) are similarly remapped to the SceneAgent.
     *                              When false these return an empty optional.
     * @return Optional owning pointer; empty if `roi` is not yet implemented.
     */
	std::optional<std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>> GetObjectDefinition(const RemoteObject::RemObjIdent& roi, const RemObjAddr& addr, bool useDefinitionRemapping = false);

	//==============================================================================
    /**
     * @brief Entry point for all incoming OCP.1 data (called on the socket thread).
     *
     * Unmarshals the raw bytes into an `Ocp1Message` and dispatches to:
     * - Notification → `UpdateObjectValue(Ocp1Notification*)`
     * - Response     → subscription ACK / get-value response / set-value ACK
     * - KeepAlive    → currently unhandled (jassertfalse placeholder)
     *
     * All state changes and object updates are posted to the JUCE message thread
     * via `postMessage()`.
     */
	bool ocp1MessageReceived(const juce::MemoryBlock& data);

    /**
     * @brief Sends AddSubscription commands for every object in `m_activeRemoteObjects`.
     * Posts `State::Subscribing` to the message thread and records the pending handles.
     */
	bool CreateObjectSubscriptions();

    /**
     * @brief Stub — subscription removal is not yet implemented.
     * @todo Implement DeleteObjectSubscriptions if graceful unsubscribe is needed.
     */
	bool DeleteObjectSubscriptions();

    /**
     * @brief Sends GetValue commands for every object in `m_activeRemoteObjects`.
     * Posts `State::GetValues` to the message thread and records the pending handles.
     */
	bool QueryObjectValues();

    /**
     * @brief Sends a single GetValue command for the specified object.
     * Used for the initial `Fixed_GUID` query before subscription.
     */
	bool QueryObjectValue(const RemoteObject::RemObjIdent roi, const RemObjAddr& addr);

	//==============================================================================
    /**
     * @name Pending-handle bookkeeping
     * @{
     *
     * OCP.1 uses a 32-bit handle to correlate commands with their responses.
     * Three separate collections track outstanding handles for subscriptions,
     * get-value queries, and set-value commands respectively.  All methods are
     * mutex-protected because `ocp1MessageReceived()` pops handles on the socket
     * thread while `Connect/Query/SetObjectValue` push them on the message thread.
     */
	void AddPendingSubscriptionHandle(const std::uint32_t handle);
	bool PopPendingSubscriptionHandle(const std::uint32_t handle);
	bool HasPendingSubscriptions();

	void AddPendingGetValueHandle(const std::uint32_t handle, const std::uint32_t ONo);
	const std::uint32_t PopPendingGetValueHandle(const std::uint32_t handle);
	bool HasPendingGetValues();

	void AddPendingSetValueHandle(const std::uint32_t handle, const std::uint32_t ONo, const int externalId);
	const std::uint32_t PopPendingSetValueHandle(const std::uint32_t handle, int& externalId);
	bool HasPendingSetValues();
    /** @brief Returns {handle, externalId} for a pending set-value command targeting ONo, if any. */
	const std::optional<std::pair<std::uint32_t, int>> HasPendingSetValue(const std::uint32_t ONo);

    /** @brief Clears all pending-handle collections (called on connection loss). */
	void ClearPendingHandles();
    /** @} */

	//==============================================================================
    /**
     * @brief Decodes an incoming OCA Notification and posts a RemoteObjectReceivedMessage.
     * Looks up the emitter ONo in `m_ONoToROIMap` to find the ROI and address.
     */
	bool UpdateObjectValue(NanoOcp1::Ocp1Notification* notifObj);

    /**
     * @brief Decodes a GetValue Response and posts a RemoteObjectReceivedMessage.
     * Looks up `ONo` in `m_ONoToROIMap` to find the ROI and address.
     */
	bool UpdateObjectValue(const std::uint32_t ONo, NanoOcp1::Ocp1Response* responseObj);

    /**
     * @brief Core decoder: determines the expected data type for `roi`, builds a
     *        `NanoOcp1::Variant`, wraps it in a `RemoteObject`, and posts it.
     *
     * The `Fixed_GUID` case is handled specially here: the GUID string is extracted
     * and forwarded to `ProcessGuidAndSubscribe()` rather than posted as a value.
     */
	bool UpdateObjectValue(const RemoteObject::RemObjIdent roi, NanoOcp1::Ocp1Message* msgObj,
		const std::pair<RemObjAddr, NanoOcp1::Ocp1CommandDefinition>& objectDetails);

	//==============================================================================
    /**
     * @brief Called when a new GUID response is received; validates it, detects the
     *        device model and OCA revision, updates speaker-position definitions in
     *        `m_ROIsToDefsMap`, and then triggers `CreateObjectSubscriptions()` +
     *        `QueryObjectValues()`.
     *
     * The GUID is only acted upon once per connection: if the same GUID arrives
     * again (e.g. from a periodic notification) it is silently ignored.
     *
     * @note [MANUAL CONTEXT NEEDED] Document the exact GUID format (byte layout,
     *       what each nibble encodes) so the parsing logic in
     *       `SetOcaRevisionAndDeviceModel()` is self-explanatory.
     */
	void ProcessGuidAndSubscribe(const juce::String newGuid);

    /**
     * @brief Parses the 8-char GUID string and sets `m_ocp1DeviceStackIdent` and
     *        `m_connectedDbDeviceModel`.
     *
     * GUID format (all characters are ASCII hex digits):
     * - Characters 0–3 : "DB00" — manufacturer prefix, identifies a d&b device.
     * - Characters 4–5 : firmware version code (hex) — compared against known
     *                    thresholds to determine the OCA revision level.
     * - Characters 6–7 : device model — "D0" = DS100, "D1" = DS100D, "D2" = DS100M.
     *
     * `m_ocp1DeviceStackIdent` is set to 0 (legacy OCA object definitions) or 1+
     * (extended/scalable object definitions).  The threshold depends on the model:
     * - DS100 : >= "0C" → stack 1 (scalability added in that firmware generation).
     * - DS100D: always stack 1 (no pre-scalability firmware was publicly released).
     * - DS100M: >= "02" → stack 1.
     *
     * @note [MANUAL CONTEXT NEEDED] Add the exact firmware version *numbers*
     *       (e.g. "firmware version 3.x corresponds to version code 0C") and a
     *       brief description of what the "scalability" OCA revision means at the
     *       OCA object level (which objects changed and how).
     *
     * @return true if the GUID is a recognised d&b device; false otherwise.
     */
	bool SetOcaRevisionAndDeviceModel(const juce::String& guid);

	//==============================================================================
    /** Mutex protecting all three pending-handle containers (socket vs. message thread). */
	std::mutex                                              m_pendingHandlesMutex;
    /** Handles of outstanding AddSubscription commands, awaiting ACK responses. */
	std::vector<std::uint32_t>								m_pendingSubscriptionHandles;
    /** Maps response handle → ONo for outstanding GetValue commands. */
	std::map<std::uint32_t, std::uint32_t>					m_pendingGetValueHandlesWithONo;
    /** Maps response handle → {ONo, externalId} for outstanding SetValue commands. */
	std::map<std::uint32_t, std::pair<std::uint32_t, int>>	m_pendingSetValueHandlesWithONo;

	//==============================================================================
    /**
     * @brief Full pre-built map: RemObjIdent → (RemObjAddr → Ocp1CommandDefinition).
     *
     * Built once by `CreateKnownONosMap()`, and partially updated in
     * `ProcessGuidAndSubscribe()` when the OCA revision for speaker positions is
     * determined.  Used by `UpdateObjectValue()` for O(1) definition lookup.
     */
	std::map<RemoteObject::RemObjIdent, std::map<RemObjAddr, NanoOcp1::Ocp1CommandDefinition>>	m_ROIsToDefsMap;

    /**
     * @brief Reverse lookup: ONo → {RemObjIdent, RemObjAddr}.
     *
     * Built from `m_ROIsToDefsMap` at the end of `CreateKnownONosMap()`.
     * Allows O(1) dispatch when an OCA Notification or Response arrives with a
     * known ONo (rather than a linear search through `m_ROIsToDefsMap`).
     */
	std::unordered_map<std::uint32_t, std::pair<RemoteObject::RemObjIdent, RemObjAddr>>			m_ONoToROIMap;

    /** Objects currently subscribed to; populated by `SetActiveRemoteObjects()`. */
	std::vector<RemoteObject>					m_activeRemoteObjects;

    //==============================================================================
    /** Underlying NanoOcp1 TCP client. */
    std::unique_ptr<NanoOcp1::NanoOcp1Client>   m_ocp1Connection;
	juce::IPAddress								m_ocp1IPAddress;  ///< Target device IP.
	int											m_ocp1Port;       ///< Target port (default 50014).
	int											m_ocp1Timeout;    ///< Socket timeout in ms.

    /** GUID string received from the device; reset to "" on each new TCP connection. */
	juce::String								m_ocp1DeviceGUID;
    /**
     * @brief OCA revision level: 0 = legacy object definitions, 1 = extended/scalable.
     * -1 means not yet determined (before GUID is received).
     */
	int											m_ocp1DeviceStackIdent = -1;
    /** Hardware model detected from the GUID; InvalidDev until the GUID is received. */
	DbDeviceModel								m_connectedDbDeviceModel = DbDeviceModel::InvalidDev;

    State                                       m_currentState = State::Disconnected;


};

