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

#include "MidiController.h"


MidiController::MidiController() = default;

MidiController::~MidiController()
{
    if (m_midiInput)
    {
        m_midiInput->stop();
        m_midiInput.reset();
    }
}

void MidiController::openDevice(const juce::String& deviceIdentifier)
{
    if (m_deviceIdentifier == deviceIdentifier && m_midiInput != nullptr)
        return;

    if (m_midiInput)
    {
        m_midiInput->stop();
        m_midiInput.reset();
    }

    m_deviceIdentifier = deviceIdentifier;

    if (deviceIdentifier.isNotEmpty())
    {
        m_midiInput = juce::MidiInput::openDevice(deviceIdentifier, this);
        if (m_midiInput)
            m_midiInput->start();
    }
}

void MidiController::setAssignment(UmsciExternalControlComponent::UpmixMidiParam param,
                                    const JUCEAppBasics::MidiCommandRangeAssignment& assi)
{
    m_assignments[static_cast<int>(param)] = assi;
}

const JUCEAppBasics::MidiCommandRangeAssignment& MidiController::getAssignment(
    UmsciExternalControlComponent::UpmixMidiParam param) const
{
    return m_assignments[static_cast<int>(param)];
}

void MidiController::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    // Called on the MIDI thread — copy the message and dispatch to the message thread.
    auto msgCopy = message;
    juce::MessageManager::callAsync([this, msgCopy]() {
        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            auto& assi = m_assignments[i];
            if (assi.getCommandType() == JUCEAppBasics::MidiCommandRangeAssignment::CT_Invalid)
                continue;
            if (!assi.isMatchingCommand(msgCopy) && !assi.isMatchingCommandRange(msgCopy))
                continue;
            if (!assi.isMatchingValueRange(msgCopy) && !assi.isMatchingCommandRange(msgCopy))
                continue;

            float normalised = 0.5f;
            if (assi.isValueRangeAssignment())
            {
                auto range = assi.getValueRange();
                auto val = JUCEAppBasics::MidiCommandRangeAssignment::getValue(msgCopy);
                if (range.getLength() > 0)
                    normalised = juce::jlimit(0.0f, 1.0f,
                                              float(val - range.getStart()) / float(range.getLength()));
            }

            auto [minVal, maxVal] = UmsciExternalControlComponent::s_paramRanges[i];
            auto domainValue = minVal + normalised * (maxVal - minVal);

            if (onParamValueChanged)
                onParamValueChanged(static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i),
                                    domainValue);
        }
    });
}
