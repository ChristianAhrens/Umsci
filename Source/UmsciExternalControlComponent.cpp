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

#include "UmsciExternalControlComponent.h"


// Human-readable labels shown beside each learner row.
const juce::String UmsciExternalControlComponent::s_paramLabels[UmsciExternalControlComponent::s_paramCount] = {
    "Rotation",
    "Translation (scale)",
    "Height translation",
    "Angle stretch",
    "Offset X",
    "Offset Y"
};

// Minimum and maximum values used when mapping a normalised MIDI value [0,1]
// to the actual parameter domain.  These match the interactive drag ranges
// observed in UmsciUpmixIndicatorPaintNControlComponent.
const std::array<std::pair<float, float>, UmsciExternalControlComponent::UpmixMidiParam_COUNT>
    UmsciExternalControlComponent::s_paramRanges = {{
        { -juce::MathConstants<float>::pi, juce::MathConstants<float>::pi },  // Rotation: radians [-π, π] (−180°–+180°, centre = front)
        { 0.0f,  3.0f },  // Translation:        radial scale factor
        { 0.0f,  2.0f },  // HeightTranslation:  height ring fraction
        { 0.0f,  2.0f },  // AngleStretch:       front/rear angular spread
        { -2.0f, 2.0f },  // OffsetX:            ring centre X in base-radius units
        { -2.0f, 2.0f },  // OffsetY:            ring centre Y in base-radius units
    }};


UmsciExternalControlComponent::UmsciExternalControlComponent()
{
    m_midiDeviceLabel = std::make_unique<juce::Label>();
    m_midiDeviceLabel->setText("MIDI input device:", juce::dontSendNotification);
    addAndMakeVisible(m_midiDeviceLabel.get());

    m_midiDeviceCombo = std::make_unique<juce::ComboBox>();
    m_midiDeviceCombo->addListener(this);
    addAndMakeVisible(m_midiDeviceCombo.get());

    updateAvailableMidiInputDevices();

    for (int i = 0; i < s_paramCount; ++i)
    {
        m_paramLabels[i] = std::make_unique<juce::Label>();
        m_paramLabels[i]->setText(s_paramLabels[i], juce::dontSendNotification);
        addAndMakeVisible(m_paramLabels[i].get());

        m_learners[i] = std::make_unique<JUCEAppBasics::MidiLearnerComponent>(
            static_cast<std::int16_t>(i),
            JUCEAppBasics::MidiLearnerComponent::AT_ValueRange | JUCEAppBasics::MidiLearnerComponent::AT_CommandRange);

        m_learners[i]->onMidiAssiSet = [this, i](juce::Component*, const JUCEAppBasics::MidiCommandRangeAssignment& assi) {
            if (onMidiAssiChanged)
                onMidiAssiChanged(static_cast<UpmixMidiParam>(i), assi);
        };

        addAndMakeVisible(m_learners[i].get());
    }

    setSize(480, 250);
}

UmsciExternalControlComponent::~UmsciExternalControlComponent()
{
}

void UmsciExternalControlComponent::resized()
{
    constexpr int margin     = 10;
    constexpr int rowHeight  = 28;
    constexpr int rowGap     = 4;
    constexpr int labelWidth = 140;

    auto bounds = getLocalBounds().reduced(margin);

    // MIDI device selector row
    auto deviceRow = bounds.removeFromTop(rowHeight);
    m_midiDeviceLabel->setBounds(deviceRow.removeFromLeft(labelWidth));
    m_midiDeviceCombo->setBounds(deviceRow);

    // Spacer between device row and parameter rows
    bounds.removeFromTop(rowGap + 4);

    // Parameter rows
    for (int i = 0; i < s_paramCount; ++i)
    {
        if (i > 0)
            bounds.removeFromTop(rowGap);

        auto row = bounds.removeFromTop(rowHeight);
        m_paramLabels[i]->setBounds(row.removeFromLeft(labelWidth));
        m_learners[i]->setBounds(row);
    }
}

void UmsciExternalControlComponent::updateAvailableMidiInputDevices()
{
    m_midiInputDeviceIdentifiers.clear();
    m_midiDeviceCombo->clear(juce::dontSendNotification);

    int itemId = 1;
    m_midiDeviceCombo->addItem("None", itemId);
    m_midiInputDeviceIdentifiers[itemId] = {};
    ++itemId;

    for (auto& device : juce::MidiInput::getAvailableDevices())
    {
        m_midiDeviceCombo->addItem(device.name, itemId);
        m_midiInputDeviceIdentifiers[itemId] = device.identifier;
        ++itemId;
    }

    m_midiDeviceCombo->setSelectedId(1, juce::dontSendNotification);
}

void UmsciExternalControlComponent::setMidiInputDeviceIdentifier(const juce::String& identifier)
{
    m_currentDeviceIdentifier = identifier;

    // Restore combo selection to match the given identifier
    for (auto& [id, devId] : m_midiInputDeviceIdentifiers)
    {
        if (devId == identifier)
        {
            m_midiDeviceCombo->setSelectedId(id, juce::dontSendNotification);
            break;
        }
    }

    for (int i = 0; i < s_paramCount; ++i)
        m_learners[i]->setSelectedDeviceIdentifier(identifier);
}

const juce::String& UmsciExternalControlComponent::getMidiInputDeviceIdentifier() const
{
    return m_currentDeviceIdentifier;
}

void UmsciExternalControlComponent::setMidiAssi(UpmixMidiParam param,
                                                 const JUCEAppBasics::MidiCommandRangeAssignment& assi)
{
    jassert(param >= 0 && param < UpmixMidiParam_COUNT);
    m_learners[param]->setCurrentMidiAssi(assi);
}

const JUCEAppBasics::MidiCommandRangeAssignment& UmsciExternalControlComponent::getMidiAssi(UpmixMidiParam param) const
{
    jassert(param >= 0 && param < UpmixMidiParam_COUNT);
    return m_learners[param]->getCurrentMidiAssi();
}

void UmsciExternalControlComponent::comboBoxChanged(juce::ComboBox* cb)
{
    if (cb != m_midiDeviceCombo.get())
        return;

    auto selectedId = m_midiDeviceCombo->getSelectedId();
    auto it = m_midiInputDeviceIdentifiers.find(selectedId);
    if (it == m_midiInputDeviceIdentifiers.end())
        return;

    m_currentDeviceIdentifier = it->second;

    for (int i = 0; i < s_paramCount; ++i)
        m_learners[i]->setSelectedDeviceIdentifier(m_currentDeviceIdentifier);

    if (onMidiInputDeviceChanged)
        onMidiInputDeviceChanged(m_currentDeviceIdentifier);
}
