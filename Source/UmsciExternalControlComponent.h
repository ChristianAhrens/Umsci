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
 * @brief Settings panel for MIDI- and OSC-based external control of the six upmix
 *        transform parameters (rotation, translation, height translation, angle stretch,
 *        X/Y offset).
 *
 * The component presents two tabs:
 *  - **MIDI** — MIDI input device selector and six `MidiLearnerComponent` rows.
 *  - **OSC**  — UDP listen-port field and six OSC-address text fields.
 *
 * Changes are communicated via callbacks:
 *  - `onMidiInputDeviceChanged` / `onMidiAssiChanged` for MIDI.
 *  - `onOscInputPortChanged` / `onOscAddrChanged` for OSC.
 *
 * ## Parameter ID enum (`UpmixMidiParam`)
 * Shared by both MIDI and OSC tabs; used as index into internal arrays.
 * Integer values must remain stable.
 */
class UmsciExternalControlComponent : public juce::Component
{
public:
    /** @brief Identifies each controllable upmix transform parameter. */
    enum UpmixMidiParam
    {
        UpmixMidiParam_Rotation         = 0,  ///< m_upmixRot    — ring rotation (−π – +π rad).
        UpmixMidiParam_Translation      = 1,  ///< m_upmixTrans  — radial scale factor.
        UpmixMidiParam_HeightTranslation= 2,  ///< m_upmixHeightTrans — height ring fraction.
        UpmixMidiParam_AngleStretch     = 3,  ///< m_upmixAngleStretch — angular spread.
        UpmixMidiParam_OffsetX          = 4,  ///< m_upmixOffsetX — ring centre X offset.
        UpmixMidiParam_OffsetY          = 5,  ///< m_upmixOffsetY — ring centre Y offset.
        UpmixMidiParam_COUNT            = 6
    };

    /** @brief Natural parameter ranges for normalised MIDI→domain mapping. Indexed by `UpmixMidiParam`. */
    static const std::array<std::pair<float, float>, UpmixMidiParam_COUNT> s_paramRanges;

    /** @brief Human-readable parameter labels shared by both tabs. */
    static const juce::String s_paramLabels[UpmixMidiParam_COUNT];

    /** @brief Default OSC address for each parameter, indexed by `UpmixMidiParam`. */
    static const juce::String s_oscDefaultAddresses[UpmixMidiParam_COUNT];

public:
    UmsciExternalControlComponent();
    ~UmsciExternalControlComponent() override;

    //==============================================================================
    void resized() override;

    //==============================================================================
    // MIDI API
    void setMidiInputDeviceIdentifier(const juce::String& identifier);
    const juce::String& getMidiInputDeviceIdentifier() const;
    void setMidiAssi(UpmixMidiParam param, const JUCEAppBasics::MidiCommandRangeAssignment& assi);
    const JUCEAppBasics::MidiCommandRangeAssignment& getMidiAssi(UpmixMidiParam param) const;

    std::function<void(const juce::String&)> onMidiInputDeviceChanged;
    std::function<void(UpmixMidiParam, JUCEAppBasics::MidiCommandRangeAssignment)> onMidiAssiChanged;

    //==============================================================================
    // OSC API
    void setOscInputPort(int port);
    int  getOscInputPort() const;
    void setOscAddr(UpmixMidiParam param, const juce::String& address);
    juce::String getOscAddr(UpmixMidiParam param) const;

    std::function<void(int)> onOscInputPortChanged;
    std::function<void(UpmixMidiParam, juce::String)> onOscAddrChanged;

private:
    class MidiTab;
    class OscTab;

    std::unique_ptr<juce::TabbedComponent> m_tabs;
    MidiTab* m_midiTab = nullptr;  ///< Non-owning pointer; lifetime managed by m_tabs.
    OscTab*  m_oscTab  = nullptr;  ///< Non-owning pointer; lifetime managed by m_tabs.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciExternalControlComponent)
};
