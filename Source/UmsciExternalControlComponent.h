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
#include <MidiLearnerComponent.h>


/**
 * @class UmsciExternalControlComponent
 * @brief Settings panel for MIDI-based external control of the six upmix transform
 *        parameters (rotation, translation, height translation, angle stretch, X/Y offset).
 *
 * The component consists of:
 *  - A MIDI input device selector (ComboBox) — selects the device to learn from.
 *  - Six MidiLearnerComponent rows, one per upmix parameter, configured for
 *    value-range + command-range assignment so that a continuous CC sweep is mapped
 *    to the full parameter range.
 *
 * Changes are applied immediately via callbacks:
 *  - `onMidiInputDeviceChanged` — fired when the device combo changes.
 *  - `onMidiAssiChanged`        — fired when any learner's assignment changes.
 *
 * Designed to be shown in a `juce::DialogWindow` (via `LaunchOptions::launchAsync()`).
 *
 * ## Parameter ID enum (`UpmixMidiParam`)
 * Used both as the `refId` passed to `MidiLearnerComponent` and as an index into
 * the internal arrays, so the integer values must remain stable.
 */
class UmsciExternalControlComponent : public juce::Component,
                                      public juce::ComboBox::Listener
{
public:
    /** @brief Identifies each controllable upmix transform parameter. */
    enum UpmixMidiParam
    {
        UpmixMidiParam_Rotation         = 0,  ///< m_upmixRot    — ring rotation in radians (−π – +π = −180°–+180°, 0 = front).
        UpmixMidiParam_Translation      = 1,  ///< m_upmixTrans  — radial scale factor.
        UpmixMidiParam_HeightTranslation= 2,  ///< m_upmixHeightTrans — height ring fraction.
        UpmixMidiParam_AngleStretch     = 3,  ///< m_upmixAngleStretch — front/rear angular spread.
        UpmixMidiParam_OffsetX          = 4,  ///< m_upmixOffsetX — ring centre X offset.
        UpmixMidiParam_OffsetY          = 5,  ///< m_upmixOffsetY — ring centre Y offset.
        UpmixMidiParam_COUNT            = 6
    };

    /**
     * @brief Natural parameter ranges used when mapping a normalised MIDI value [0,1]
     *        to the actual parameter domain.  Indexed by `UpmixMidiParam`.
     */
    static const std::array<std::pair<float, float>, UpmixMidiParam_COUNT> s_paramRanges;

public:
    UmsciExternalControlComponent();
    ~UmsciExternalControlComponent() override;

    //==============================================================================
    void resized() override;

    //==============================================================================
    /** @brief Sets the currently selected MIDI input device identifier and updates
     *         the combo selection and all learner components. */
    void setMidiInputDeviceIdentifier(const juce::String& identifier);
    /** @brief Returns the identifier of the currently selected MIDI input device. */
    const juce::String& getMidiInputDeviceIdentifier() const;

    /** @brief Restores a stored MIDI assignment into the corresponding learner. */
    void setMidiAssi(UpmixMidiParam param, const JUCEAppBasics::MidiCommandRangeAssignment& assi);
    /** @brief Returns the current MIDI assignment for the given parameter. */
    const JUCEAppBasics::MidiCommandRangeAssignment& getMidiAssi(UpmixMidiParam param) const;

    //==============================================================================
    /** @brief Fired when the MIDI input device combo changes.  Parameter: new device identifier. */
    std::function<void(const juce::String&)> onMidiInputDeviceChanged;

    /** @brief Fired when a learner's assignment changes.  Parameters: param ID, new assignment. */
    std::function<void(UpmixMidiParam, JUCEAppBasics::MidiCommandRangeAssignment)> onMidiAssiChanged;

    //==============================================================================
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

private:
    //==============================================================================
    /** @brief Populates the MIDI input device combo with currently available devices. */
    void updateAvailableMidiInputDevices();

    //==============================================================================
    std::unique_ptr<juce::Label>    m_midiDeviceLabel;
    std::unique_ptr<juce::ComboBox> m_midiDeviceCombo;
    /** @brief Maps ComboBox item IDs to MIDI device identifier strings (empty = "None"). */
    std::map<int, juce::String>     m_midiInputDeviceIdentifiers;

    static const int s_paramCount = UpmixMidiParam_COUNT;
    static const juce::String s_paramLabels[s_paramCount];

    std::unique_ptr<juce::Label>                         m_paramLabels[s_paramCount];
    std::unique_ptr<JUCEAppBasics::MidiLearnerComponent> m_learners[s_paramCount];

    juce::String m_currentDeviceIdentifier;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciExternalControlComponent)
};
