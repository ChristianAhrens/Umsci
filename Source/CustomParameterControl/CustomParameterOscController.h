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

#include "CustomParameterConfig.h"


/**
 * @class CustomParameterOscController
 * @brief Manages OSC communication for user-defined custom parameter control.
 *
 * Owns both an `juce::OSCSender` (to the remote host on the configured send port)
 * and a `juce::OSCReceiver` (listening on the configured receive port).
 *
 * ## Lifecycle
 * 1. Call `setConfig()` to apply connection parameters and OSC addresses.
 * 2. Call `connect()` to open the sender and start the receiver — returns false on error.
 * 3. Optionally call `pollAllValues()` right after connecting; this sends each configured
 *    OSC address without arguments so the remote host returns the current values.
 * 4. Call `sendParameterValue()` when the user moves a control.
 * 5. Assign `onParameterValueReceived` to receive incoming updates from the remote host.
 * 6. Call `disconnect()` when done.
 */
class CustomParameterOscController
    : public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    CustomParameterOscController();
    ~CustomParameterOscController() override;

    //==============================================================================
    /** Applies new connection and parameter address configuration. */
    void setConfig(const CustomParameterConfig& config);

    /** Returns the current configuration. */
    const CustomParameterConfig& getConfig() const { return m_config; }

    //==============================================================================
    /** Opens the OSC sender and starts the receiver.  Returns true on success. */
    bool connect();

    /** Closes both sender and receiver. */
    void disconnect();

    /** Returns true if connect() has been called successfully. */
    bool isConnected() const { return m_connected; }

    //==============================================================================
    /** Sends an OSC message with the parameter's address and the given native value. */
    void sendParameterValue(int parameterIndex, float nativeValue);

    /** Sends each parameter's address without arguments, requesting the current
     *  value from the remote host per OSC convention. */
    void pollAllValues();

    //==============================================================================
    /** Fired on the message thread when an incoming OSC message matches a
     *  configured parameter address.
     *  @param parameterIndex  Zero-based index into CustomParameterConfig::parameters.
     *  @param nativeValue     Received float value. */
    std::function<void(int parameterIndex, float nativeValue)> onParameterValueReceived;

private:
    //==============================================================================
    void oscMessageReceived(const juce::OSCMessage& message) override;

    //==============================================================================
    juce::OSCSender   m_sender;
    juce::OSCReceiver m_receiver;
    CustomParameterConfig m_config;
    bool m_connected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomParameterOscController)
};
