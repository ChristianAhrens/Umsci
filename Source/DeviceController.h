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

#include <SoundscapeController.h>
#include <Variant.h>
#include <internal/NanoTimerScheduler.h>

#include <memory>


/**
 * @class DeviceController
 * @brief JUCE-aware singleton that wraps NanoOcp1::SoundscapeController.
 *
 * Derives from NanoOcp1::SoundscapeController for all OCP.1 protocol logic and
 * adds:
 * - JUCE singleton lifecycle (JUCE_DECLARE_SINGLETON).
 * - Thread-safe state dispatch via juce::MessageListener / postMessage()
 *   so that onStateChanged and onRemoteObjectReceived are always called on
 *   the JUCE message thread.
 * - JUCE-specific address API (juce::IPAddress, string helpers on RemObjAddr).
 * - JUCE leak-detector annotations on nested types.
 *
 * ## Connection lifecycle
 * Identical to NanoOcp1::SoundscapeController: Disconnected → Connecting →
 * Subscribing → Subscribed → GetValues → Connected.  Reconnect on loss is
 * handled automatically by the base class.
 *
 * ## Threading
 * NanoOcp1::SoundscapeController fires all of its callbacks (message-driven ones as
 * well as the GetValues response-timeout) on the shared NanoTimerScheduler thread;
 * NanoOcp no longer has a separate dispatcher thread for the message-driven ones.
 * DeviceController intercepts both callbacks in the constructor, wraps
 * the payload in a juce::Message, and posts it to the JUCE message thread.
 * handleMessage() then calls the public onRemoteObjectReceived /
 * onStateChanged on the message thread.
 *
 * The NanoTimerScheduler backing the base class' timers is created by
 * DeviceController's constructor and owned by the base class; NanoOcp itself
 * provides no default instance.
 */
class DeviceController : public NanoOcp1::SoundscapeController,
                         public juce::MessageListener
{
public:
    // ── Connection state (mirrors NanoOcp1::Ocp1Controller::State) ──────────

    /**
     * @brief Connection phase — values are identical to
     * NanoOcp1::Ocp1Controller::State so static_cast is lossless.
     */
    enum State
    {
        Disconnected, ///< No TCP connection.
        Connecting,   ///< TCP connect in progress; base class retries automatically.
        Subscribing,  ///< AddSubscription commands sent.
        Subscribed,   ///< All subscriptions confirmed; GetValue queries outstanding.
        GetValues,    ///< GetValue responses being collected.
        Connected     ///< All subscriptions and initial values confirmed.
    };

    // ── Hardware variant ─────────────────────────────────────────────────────

    enum DbDeviceModel
    {
        InvalidDev  = 0,
        DS100,
        DS110,
        DS100M,
        vCore,
        InvalidDev_max
    };

    // ── Coordinate-mapping area ───────────────────────────────────────────────

    enum MappingAreaId
    {
        InvalidMapId = -1,
        First  = 1,
        Second,
        Third,
        Fourth,
        InvaliMapId_max
    };

    static constexpr std::uint16_t sc_MAX_INPUTS_CHANNELS = NanoOcp1::SoundscapeController::sc_MAX_INPUT_CHANNELS;
    static constexpr std::uint16_t sc_MAX_OUTPUT_CHANNELS = NanoOcp1::SoundscapeController::sc_MAX_OUTPUT_CHANNELS;
    static constexpr std::uint16_t sc_MAX_FUNCTION_GROUPS = NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS;
    static constexpr std::uint16_t sc_MAX_REVERB_ZONES    = NanoOcp1::SoundscapeController::sc_MAX_REVERB_ZONES;

    // ── Two-dimensional address (JUCE-enriched) ───────────────────────────────

    /**
     * @brief Object address — same two-field layout as
     * NanoOcp1::SoundscapeController::RemObjAddr, plus JUCE string helpers.
     */
    struct RemObjAddr
    {
        std::int16_t pri{0};
        std::int16_t sec{0};

        static constexpr std::int16_t sc_INV = 0;

        RemObjAddr() = default;
        RemObjAddr(const RemObjAddr&) = default;
        RemObjAddr(std::int16_t a, std::int16_t b) : pri(a), sec(b) {}

        juce::String toNiceString() const
        {
            if (sc_INV != pri && sc_INV != sec)
                return juce::String(sec) + "/" + juce::String(pri);
            else if (sc_INV != pri)
                return juce::String(pri);
            else if (sc_INV != sec)
                return juce::String(sec);
            else
                return {};
        }
        juce::String toString() const
        {
            return juce::String(pri) + "," + juce::String(sec);
        }
        static juce::String toString(const std::vector<RemObjAddr>& addrs)
        {
            juce::String s;
            for (auto const& a : addrs)
                s << a.toString() << ";";
            return s;
        }
        bool fromString(const juce::String& csv)
        {
            juce::StringArray sa;
            if (sa.addTokens(csv, ",", "") != 2 || sa.size() != 2)
                return false;
            pri = std::int16_t(sa[0].getIntValue());
            sec = std::int16_t(sa[1].getIntValue());
            return true;
        }
        static RemObjAddr createFromString(const juce::String& csv)
        {
            juce::StringArray sa;
            sa.addTokens(csv.trimCharactersAtEnd(","), ",", "");
            if (sa.size() != 2)
                return {};
            return { std::int16_t(sa[0].getIntValue()), std::int16_t(sa[1].getIntValue()) };
        }
        static std::vector<RemObjAddr> createFromListString(const juce::String& list)
        {
            std::vector<RemObjAddr> result;
            juce::StringArray sa;
            sa.addTokens(list.trimCharactersAtEnd(";"), ";", "");
            for (auto const& csv : sa)
                result.push_back(createFromString(csv));
            return result;
        }

        bool operator==(const RemObjAddr& r) const { return pri == r.pri && sec == r.sec; }
        bool operator!=(const RemObjAddr& r) const { return !(*this == r); }
        bool operator<(const RemObjAddr& r) const
        {
            return pri < r.pri || (pri == r.pri && sec < r.sec);
        }
        bool operator>(const RemObjAddr& r) const { return !(*this < r) && (*this != r); }
        RemObjAddr& operator=(const RemObjAddr&) = default;

        JUCE_LEAK_DETECTOR(RemObjAddr)
    };

    // ── Remote parameter ──────────────────────────────────────────────────────

    /**
     * @brief A fully-qualified DS100 remote parameter: type, address, value.
     *
     * RemObjIdent is the actual NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent
     * enum (aliased, not duplicated) so the two can never drift out of numeric sync with
     * each other - a hand-maintained duplicate enum silently went out of alignment here
     * once before, when NanoOcp inserted a new entry mid-enum without a matching change
     * on this side, and the resulting cross-wiring of OCA properties is hard to spot since
     * most mismatched pairs don't share an assert-catchable data type.
     * Addr stays a DeviceController::RemObjAddr (adds JUCE helpers) rather than aliasing too.
     */
    struct RemoteObject
    {
        using RemObjIdent = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;

        RemObjIdent    Id  {RemObjIdent::Invalid};
        RemObjAddr     Addr;
        NanoOcp1::Variant Var;

        RemoteObject() = default;
        RemoteObject(const RemoteObject&) = default;
        RemoteObject(RemObjIdent id, RemObjAddr addr, NanoOcp1::Variant v = NanoOcp1::Variant{})
            : Id(id), Addr(addr), Var(std::move(v)) {}

        bool operator==(const RemoteObject& o) const
        {
            return Id == o.Id && Addr == o.Addr && Var == o.Var;
        }
        bool operator!=(const RemoteObject& o) const { return !(*this == o); }
        bool operator<(const RemoteObject& o) const
        {
            return (!(*this > o) && (*this != o));
        }
        bool operator>(const RemoteObject& o) const
        {
            return (Id > o.Id) || ((Id == o.Id) && (Addr > o.Addr));
        }
        RemoteObject& operator=(const RemoteObject&) = default;

        static juce::String GetObjectDescription(RemObjIdent roi)
        {
            return juce::String(NanoOcp1::SoundscapeController::RemoteObject::GetObjectDescription(roi));
        }

        static bool IsFlickering(RemObjIdent roi)
        {
            return NanoOcp1::SoundscapeController::RemoteObject::IsFlickering(roi);
        }

        JUCE_LEAK_DETECTOR(RemoteObject)
    };

    // ── Thread-dispatch messages ──────────────────────────────────────────────

    class StateChangeMessage : public juce::Message
    {
    public:
        StateChangeMessage() = default;
        explicit StateChangeMessage(State s) : m_state(s) {}
        State getState() const { return m_state; }
    private:
        State m_state{Disconnected};
    };

    class RemoteObjectReceivedMessage : public juce::Message
    {
    public:
        RemoteObjectReceivedMessage() = default;
        explicit RemoteObjectReceivedMessage(const RemoteObject& r) : m_obj(r) {}
        const RemoteObject& getRemoteObject() const { return m_obj; }
    private:
        RemoteObject m_obj;
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    DeviceController();
    ~DeviceController() override;

    JUCE_DECLARE_SINGLETON(DeviceController, false)

    // ── juce::MessageListener ─────────────────────────────────────────────────

    void handleMessage(const juce::Message& message) override;

    // ── Connection API ────────────────────────────────────────────────────────

    bool connect();
    void disconnect();

    void setConnectionParameters(juce::IPAddress ip, int port, int timeoutMs = 150);
    const std::tuple<juce::IPAddress, int, int> getConnectionParameters();

    void setDeviceIOSize(std::uint16_t inputs, std::uint16_t outputs);

    const State getState() const;

    // ── Remote-object API ─────────────────────────────────────────────────────

    bool SetActiveRemoteObjects(const std::vector<RemoteObject>& remObjs);
    const std::vector<RemoteObject>& GetActiveRemoteObjects();

    bool SetObjectValue(const RemoteObject& remObj);

    // ── Callbacks (called on JUCE message thread) ─────────────────────────────

    std::function<bool(const RemoteObject&)>  onRemoteObjectReceived;
    std::function<void(const State state)>    onStateChanged;

private:
    // ── Type conversions ──────────────────────────────────────────────────────

    static NanoOcp1::SoundscapeController::RemObjAddr  toCtrlAddr(const RemObjAddr& a);
    static RemObjAddr                              fromCtrlAddr(const NanoOcp1::SoundscapeController::RemObjAddr& a);
    static NanoOcp1::SoundscapeController::RemoteObject toCtrlObj(const RemoteObject& o);
    static RemoteObject                            fromCtrlObj(const NanoOcp1::SoundscapeController::RemoteObject& o);

    // ── State ─────────────────────────────────────────────────────────────────

    juce::IPAddress m_ocp1IPAddress;
    int             m_ocp1Port    {50014};
    int             m_ocp1Timeout {150};

    std::vector<RemoteObject> m_activeRemoteObjects;
};
