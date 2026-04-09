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


// ─── Shared static data ───────────────────────────────────────────────────────

const juce::String UmsciExternalControlComponent::s_paramLabels[UmsciExternalControlComponent::UpmixMidiParam_COUNT] = {
    "Rotation",
    "Translation (scale)",
    "Height translation",
    "Angle stretch",
    "Offset X",
    "Offset Y"
};

const std::array<std::pair<float, float>, UmsciExternalControlComponent::UpmixMidiParam_COUNT>
    UmsciExternalControlComponent::s_paramRanges = {{
        { -juce::MathConstants<float>::pi, juce::MathConstants<float>::pi },  // Rotation: −π – +π rad
        { 0.0f,  3.0f },  // Translation:       radial scale factor
        { 0.0f,  2.0f },  // HeightTranslation: height ring fraction
        { 0.0f,  2.0f },  // AngleStretch:      front/rear angular spread
        { -2.0f, 2.0f },  // OffsetX:           ring centre X (base-radius units)
        { -2.0f, 2.0f },  // OffsetY:           ring centre Y (base-radius units)
    }};

const juce::String UmsciExternalControlComponent::s_oscDefaultAddresses[UmsciExternalControlComponent::UpmixMidiParam_COUNT] = {
    "/umsci/indicator/rot",
    "/umsci/indicator/trans",
    "/umsci/indicator/heighttrans",
    "/umsci/indicator/anglestretch",
    "/umsci/indicator/offsetx",
    "/umsci/indicator/offsety"
};


// ─── MidiTab ─────────────────────────────────────────────────────────────────

class UmsciExternalControlComponent::MidiTab : public juce::Component,
                                               public juce::ComboBox::Listener
{
public:
    MidiTab(UmsciExternalControlComponent& owner) : m_owner(owner)
    {
        m_midiDeviceLabel = std::make_unique<juce::Label>();
        m_midiDeviceLabel->setText("MIDI input device:", juce::dontSendNotification);
        addAndMakeVisible(m_midiDeviceLabel.get());

        m_midiDeviceCombo = std::make_unique<juce::ComboBox>();
        m_midiDeviceCombo->addListener(this);
        addAndMakeVisible(m_midiDeviceCombo.get());

        updateAvailableMidiInputDevices();

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            m_paramLabels[i] = std::make_unique<juce::Label>();
            m_paramLabels[i]->setText(UmsciExternalControlComponent::s_paramLabels[i],
                                      juce::dontSendNotification);
            addAndMakeVisible(m_paramLabels[i].get());

            m_learners[i] = std::make_unique<JUCEAppBasics::MidiLearnerComponent>(
                static_cast<std::int16_t>(i),
                static_cast<JUCEAppBasics::MidiLearnerComponent::AssignmentType>(JUCEAppBasics::MidiLearnerComponent::AT_ValueRange | JUCEAppBasics::MidiLearnerComponent::AT_CommandRange));

            m_learners[i]->onMidiAssiSet = [this, i](juce::Component*, const JUCEAppBasics::MidiCommandRangeAssignment& assi) {
                if (m_owner.onMidiAssiChanged)
                    m_owner.onMidiAssiChanged(static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i), assi);
            };

            addAndMakeVisible(m_learners[i].get());
        }
    }

    void resized() override
    {
        constexpr int margin     = 10;
        constexpr int rowHeight  = 28;
        constexpr int rowGap     = 4;
        constexpr int labelWidth = 140;

        auto bounds = getLocalBounds().reduced(margin);

        auto deviceRow = bounds.removeFromTop(rowHeight);
        m_midiDeviceLabel->setBounds(deviceRow.removeFromLeft(labelWidth));
        m_midiDeviceCombo->setBounds(deviceRow);

        bounds.removeFromTop(rowGap + 4);

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            if (i > 0) bounds.removeFromTop(rowGap);
            auto row = bounds.removeFromTop(rowHeight);
            m_paramLabels[i]->setBounds(row.removeFromLeft(labelWidth));
            m_learners[i]->setBounds(row);
        }
    }

    void comboBoxChanged(juce::ComboBox* cb) override
    {
        if (cb != m_midiDeviceCombo.get())
            return;

        auto it = m_midiInputDeviceIdentifiers.find(m_midiDeviceCombo->getSelectedId());
        if (it == m_midiInputDeviceIdentifiers.end())
            return;

        m_currentDeviceIdentifier = it->second;

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
            m_learners[i]->setSelectedDeviceIdentifier(m_currentDeviceIdentifier);

        if (m_owner.onMidiInputDeviceChanged)
            m_owner.onMidiInputDeviceChanged(m_currentDeviceIdentifier);
    }

    void setMidiInputDeviceIdentifier(const juce::String& identifier)
    {
        m_currentDeviceIdentifier = identifier;

        for (auto& [id, devId] : m_midiInputDeviceIdentifiers)
        {
            if (devId == identifier)
            {
                m_midiDeviceCombo->setSelectedId(id, juce::dontSendNotification);
                break;
            }
        }

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
            m_learners[i]->setSelectedDeviceIdentifier(identifier);
    }

    const juce::String& getMidiInputDeviceIdentifier() const
    {
        return m_currentDeviceIdentifier;
    }

    void setMidiAssi(int param, const JUCEAppBasics::MidiCommandRangeAssignment& assi)
    {
        m_learners[param]->setCurrentMidiAssi(assi);
    }

    const JUCEAppBasics::MidiCommandRangeAssignment& getMidiAssi(int param) const
    {
        return m_learners[param]->getCurrentMidiAssi();
    }

private:
    void updateAvailableMidiInputDevices()
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

    UmsciExternalControlComponent& m_owner;

    std::unique_ptr<juce::Label>    m_midiDeviceLabel;
    std::unique_ptr<juce::ComboBox> m_midiDeviceCombo;
    std::map<int, juce::String>     m_midiInputDeviceIdentifiers;

    std::unique_ptr<juce::Label>                         m_paramLabels[UmsciExternalControlComponent::UpmixMidiParam_COUNT];
    std::unique_ptr<JUCEAppBasics::MidiLearnerComponent> m_learners[UmsciExternalControlComponent::UpmixMidiParam_COUNT];

    juce::String m_currentDeviceIdentifier;
};


// ─── OscTab ──────────────────────────────────────────────────────────────────

class UmsciExternalControlComponent::OscTab : public juce::Component
{
public:
    OscTab(UmsciExternalControlComponent& owner) : m_owner(owner)
    {
        m_oscPortLabel = std::make_unique<juce::Label>();
        m_oscPortLabel->setText("OSC listen port:", juce::dontSendNotification);
        addAndMakeVisible(m_oscPortLabel.get());

        m_oscPortEditor = std::make_unique<juce::TextEditor>();
        m_oscPortEditor->setText("0", juce::dontSendNotification);
        m_oscPortEditor->setInputRestrictions(5, "0123456789");
        m_oscPortEditor->onReturnKey = [this] { notifyPortChanged(); };
        m_oscPortEditor->onFocusLost = [this] { notifyPortChanged(); };
        addAndMakeVisible(m_oscPortEditor.get());

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            m_oscParamLabels[i] = std::make_unique<juce::Label>();
            m_oscParamLabels[i]->setText(UmsciExternalControlComponent::s_paramLabels[i],
                                         juce::dontSendNotification);
            addAndMakeVisible(m_oscParamLabels[i].get());

            m_oscAddrEditors[i] = std::make_unique<juce::TextEditor>();
            m_oscAddrEditors[i]->setText(UmsciExternalControlComponent::s_oscDefaultAddresses[i],
                                         juce::dontSendNotification);
            const int idx = i;
            m_oscAddrEditors[i]->onReturnKey = [this, idx] { notifyAddrChanged(idx); };
            m_oscAddrEditors[i]->onFocusLost = [this, idx] { notifyAddrChanged(idx); };
            addAndMakeVisible(m_oscAddrEditors[i].get());
        }
    }

    void resized() override
    {
        constexpr int margin     = 10;
        constexpr int rowHeight  = 28;
        constexpr int rowGap     = 4;
        constexpr int labelWidth = 140;

        auto bounds = getLocalBounds().reduced(margin);

        auto portRow = bounds.removeFromTop(rowHeight);
        m_oscPortLabel->setBounds(portRow.removeFromLeft(labelWidth));
        m_oscPortEditor->setBounds(portRow);

        bounds.removeFromTop(rowGap + 4);

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            if (i > 0) bounds.removeFromTop(rowGap);
            auto row = bounds.removeFromTop(rowHeight);
            m_oscParamLabels[i]->setBounds(row.removeFromLeft(labelWidth));
            m_oscAddrEditors[i]->setBounds(row);
        }
    }

    void setOscInputPort(int port)
    {
        m_oscPortEditor->setText(juce::String(port), juce::dontSendNotification);
    }

    int getOscInputPort() const
    {
        return m_oscPortEditor->getText().getIntValue();
    }

    void setOscAddr(int param, const juce::String& address)
    {
        m_oscAddrEditors[param]->setText(address, juce::dontSendNotification);
    }

    juce::String getOscAddr(int param) const
    {
        return m_oscAddrEditors[param]->getText();
    }

private:
    void notifyPortChanged()
    {
        if (m_owner.onOscInputPortChanged)
            m_owner.onOscInputPortChanged(m_oscPortEditor->getText().getIntValue());
    }

    void notifyAddrChanged(int idx)
    {
        if (m_owner.onOscAddrChanged)
            m_owner.onOscAddrChanged(
                static_cast<UmsciExternalControlComponent::UpmixMidiParam>(idx),
                m_oscAddrEditors[idx]->getText());
    }

    UmsciExternalControlComponent& m_owner;

    std::unique_ptr<juce::Label>      m_oscPortLabel;
    std::unique_ptr<juce::TextEditor> m_oscPortEditor;

    std::unique_ptr<juce::Label>      m_oscParamLabels[UmsciExternalControlComponent::UpmixMidiParam_COUNT];
    std::unique_ptr<juce::TextEditor> m_oscAddrEditors[UmsciExternalControlComponent::UpmixMidiParam_COUNT];
};


// ─── UmsciExternalControlComponent ───────────────────────────────────────────

UmsciExternalControlComponent::UmsciExternalControlComponent()
{
    m_tabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);

    auto midiTabOwned = std::make_unique<MidiTab>(*this);
    m_midiTab = midiTabOwned.get();

    auto oscTabOwned = std::make_unique<OscTab>(*this);
    m_oscTab = oscTabOwned.get();

    auto bgColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    m_tabs->addTab("MIDI", bgColour, midiTabOwned.release(), true);
    m_tabs->addTab("OSC",  bgColour, oscTabOwned.release(),  true);

    addAndMakeVisible(m_tabs.get());

    setSize(480, 280);
}

UmsciExternalControlComponent::~UmsciExternalControlComponent()
{
}

void UmsciExternalControlComponent::resized()
{
    m_tabs->setBounds(getLocalBounds());
}

// ── MIDI delegation ──────────────────────────────────────────────────────────

void UmsciExternalControlComponent::setMidiInputDeviceIdentifier(const juce::String& identifier)
{
    m_midiTab->setMidiInputDeviceIdentifier(identifier);
}

const juce::String& UmsciExternalControlComponent::getMidiInputDeviceIdentifier() const
{
    return m_midiTab->getMidiInputDeviceIdentifier();
}

void UmsciExternalControlComponent::setMidiAssi(UpmixMidiParam param,
                                                 const JUCEAppBasics::MidiCommandRangeAssignment& assi)
{
    m_midiTab->setMidiAssi(static_cast<int>(param), assi);
}

const JUCEAppBasics::MidiCommandRangeAssignment& UmsciExternalControlComponent::getMidiAssi(UpmixMidiParam param) const
{
    return m_midiTab->getMidiAssi(static_cast<int>(param));
}

// ── OSC delegation ───────────────────────────────────────────────────────────

void UmsciExternalControlComponent::setOscInputPort(int port)
{
    m_oscTab->setOscInputPort(port);
}

int UmsciExternalControlComponent::getOscInputPort() const
{
    return m_oscTab->getOscInputPort();
}

void UmsciExternalControlComponent::setOscAddr(UpmixMidiParam param, const juce::String& address)
{
    m_oscTab->setOscAddr(static_cast<int>(param), address);
}

juce::String UmsciExternalControlComponent::getOscAddr(UpmixMidiParam param) const
{
    return m_oscTab->getOscAddr(static_cast<int>(param));
}
