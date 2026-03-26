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

#include <MidiCommandRangeAssignment.h>

#include "UmsciExternalControlComponent.h"


/**
 * @class MidiController
 * @brief Owns the MIDI input device and maps learned MIDI messages to upmix
 *        parameter domain values.
 *
 * Inherits `juce::MidiInputCallback` to receive raw MIDI on the MIDI thread,
 * then dispatches normalised→domain-mapped values to `onParamValueChanged` on
 * the message thread.
 *
 * ## Usage
 * 1. Call `openDevice()` to open a MIDI input device.
 * 2. Call `setAssignment()` for each `UpmixMidiParam` to configure which MIDI
 *    command drives that parameter.
 * 3. Assign `onParamValueChanged` to receive (param, domainValue) pairs.
 */
class MidiController : public juce::MidiInputCallback
{
public:
    MidiController();
    ~MidiController() override;

    //==============================================================================
    /** Opens (or re-opens) the MIDI input device identified by @p deviceIdentifier.
     *  Closes any previously open device first.  Pass an empty string to close only. */
    void openDevice(const juce::String& deviceIdentifier);

    /** Returns the identifier of the currently open MIDI input device (empty = none). */
    const juce::String& getDeviceIdentifier() const { return m_deviceIdentifier; }

    //==============================================================================
    /** Sets the MIDI command range assignment for a given upmix parameter. */
    void setAssignment(UmsciExternalControlComponent::UpmixMidiParam param,
                       const JUCEAppBasics::MidiCommandRangeAssignment& assi);

    /** Returns the current assignment for a given upmix parameter. */
    const JUCEAppBasics::MidiCommandRangeAssignment& getAssignment(
        UmsciExternalControlComponent::UpmixMidiParam param) const;

    //==============================================================================
    /** Fired on the message thread when an incoming MIDI message matches a learned
     *  assignment.  The float is the value mapped to the parameter's natural domain
     *  using `UmsciExternalControlComponent::s_paramRanges`. */
    std::function<void(UmsciExternalControlComponent::UpmixMidiParam, float)> onParamValueChanged;

private:
    //==============================================================================
    /** @brief `juce::MidiInputCallback` — called on the MIDI thread. */
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;

    //==============================================================================
    std::unique_ptr<juce::MidiInput> m_midiInput;
    juce::String m_deviceIdentifier;
    std::array<JUCEAppBasics::MidiCommandRangeAssignment,
               UmsciExternalControlComponent::UpmixMidiParam_COUNT> m_assignments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiController)
};
