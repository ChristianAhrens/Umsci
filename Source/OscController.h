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

#include "UmsciExternalControlComponent.h"


/**
 * @class OscController
 * @brief Owns the OSC receiver and maps incoming OSC float messages to upmix
 *        parameter domain values.
 *
 * Inherits `juce::OSCReceiver::Listener<MessageLoopCallback>` so that
 * `oscMessageReceived()` is called on the message thread directly.
 * Incoming float values are clamped to the natural parameter range defined in
 * `UmsciExternalControlComponent::s_paramRanges` before being forwarded via
 * `onParamValueChanged`.
 *
 * ## Usage
 * 1. Call `openPort()` to start listening on a UDP port (0 = stop listening).
 * 2. Call `setAddress()` for each `UpmixMidiParam` to configure the OSC address
 *    pattern that drives that parameter.
 * 3. Assign `onParamValueChanged` to receive (param, domainValue) pairs.
 */
class OscController : public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    OscController();
    ~OscController() override;

    //==============================================================================
    /** Opens (or re-opens) the OSC UDP listen port.
     *  Pass 0 to stop listening without reopening. */
    void openPort(int port);

    /** Returns the currently open OSC listen port (0 = not listening). */
    int getPort() const { return m_port; }

    //==============================================================================
    /** Sets the OSC address pattern for a given upmix parameter. */
    void setAddress(UmsciExternalControlComponent::UpmixMidiParam param,
                    const juce::String& address);

    /** Returns the current OSC address for a given upmix parameter. */
    juce::String getAddress(UmsciExternalControlComponent::UpmixMidiParam param) const;

    //==============================================================================
    /** Fired on the message thread when an incoming OSC float message matches a
     *  configured address.  The float is the received value clamped to the
     *  parameter's natural domain using `UmsciExternalControlComponent::s_paramRanges`. */
    std::function<void(UmsciExternalControlComponent::UpmixMidiParam, float)> onParamValueChanged;

private:
    //==============================================================================
    /** @brief `juce::OSCReceiver::Listener` — called on the message thread. */
    void oscMessageReceived(const juce::OSCMessage& message) override;

    //==============================================================================
    juce::OSCReceiver m_oscReceiver;
    int m_port = 0;
    std::array<juce::String,
               UmsciExternalControlComponent::UpmixMidiParam_COUNT> m_addresses;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscController)
};
