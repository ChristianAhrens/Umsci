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

#include "CustomParameterControlConfigDialog.h"


CustomParameterControlConfigDialog::CustomParameterControlConfigDialog()
{
    // Connection section
    m_hostLabel = std::make_unique<juce::Label>("hostLabel", "Remote host:");
    addAndMakeVisible(m_hostLabel.get());

    m_hostEditor = std::make_unique<juce::TextEditor>("hostEditor");
    m_hostEditor->setText("127.0.0.1", juce::dontSendNotification);
    addAndMakeVisible(m_hostEditor.get());

    m_sendPortLabel = std::make_unique<juce::Label>("sendPortLabel", "Send port:");
    addAndMakeVisible(m_sendPortLabel.get());

    m_sendPortEditor = std::make_unique<juce::TextEditor>("sendPortEditor");
    m_sendPortEditor->setInputRestrictions(5, "0123456789");
    m_sendPortEditor->setText("9001", juce::dontSendNotification);
    addAndMakeVisible(m_sendPortEditor.get());

    m_recvPortLabel = std::make_unique<juce::Label>("recvPortLabel", "Receive port:");
    addAndMakeVisible(m_recvPortLabel.get());

    m_recvPortEditor = std::make_unique<juce::TextEditor>("recvPortEditor");
    m_recvPortEditor->setInputRestrictions(5, "0123456789");
    m_recvPortEditor->setText("9002", juce::dontSendNotification);
    addAndMakeVisible(m_recvPortEditor.get());

    // Scrollable list
    m_listContent = std::make_unique<juce::Component>();
    m_viewport    = std::make_unique<juce::Viewport>();
    m_viewport->setViewedComponent(m_listContent.get(), false);
    m_viewport->setScrollBarsShown(true, false);
    addAndMakeVisible(m_viewport.get());

    // Add button
    m_addButton = std::make_unique<juce::TextButton>("+ Add parameter");
    m_addButton->onClick = [this]() {
        CustomParameterEntry newEntry;
        newEntry.name       = "Parameter " + juce::String(int(m_paramRows.size()) + 1);
        newEntry.oscAddress = "/param/" + juce::String(int(m_paramRows.size()));
        m_config.parameters.push_back(newEntry);
        rebuildParameterRows();
        resized();
    };
    addAndMakeVisible(m_addButton.get());

    setSize(kDialogWidth, computePreferredHeight());
}

CustomParameterControlConfigDialog::~CustomParameterControlConfigDialog() = default;

void CustomParameterControlConfigDialog::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Section labels
    auto font = juce::Font(juce::FontOptions{}.withHeight(13.0f).withStyle("Bold"));
    g.setFont(font);
    g.setColour(getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId));
    g.drawText("Connection", kMargin, kMargin, kDialogWidth - 2 * kMargin, 16,
               juce::Justification::centredLeft);
    g.drawText("Parameters", kMargin, kHeaderH + kMargin, kDialogWidth - 2 * kMargin, 16,
               juce::Justification::centredLeft);

    // Separator lines
    g.setColour(getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId)
                .withAlpha(0.2f));
    g.drawHorizontalLine(kHeaderH - 1, float(kMargin), float(kDialogWidth - kMargin));
}

void CustomParameterControlConfigDialog::resized()
{
    auto bounds = getLocalBounds().reduced(kMargin);
    bounds.removeFromTop(18); // section label "Connection"

    // Row 1: host
    auto connRow1 = bounds.removeFromTop(kRowH);
    auto hostLabelBounds = connRow1.removeFromLeft(90);
    m_hostLabel->setBounds(hostLabelBounds);
    m_hostEditor->setBounds(connRow1.removeFromLeft(180).reduced(0, 2));

    // Row 2: ports
    auto connRow2 = bounds.removeFromTop(kRowH);
    connRow2.removeFromLeft(90); // align under host editor
    auto sendPortLabelBounds = connRow2.removeFromLeft(72);
    m_sendPortLabel->setBounds(sendPortLabelBounds);
    m_sendPortEditor->setBounds(connRow2.removeFromLeft(70).reduced(0, 2));
    connRow2.removeFromLeft(8);
    auto recvPortLabelBounds = connRow2.removeFromLeft(84);
    m_recvPortLabel->setBounds(recvPortLabelBounds);
    m_recvPortEditor->setBounds(connRow2.removeFromLeft(70).reduced(0, 2));

    bounds.removeFromTop(kMargin); // gap before "Parameters" label
    bounds.removeFromTop(18);      // section label "Parameters"

    // Column header row heights (shared with content rows)
    const int kViewportH = juce::jmax(kRowH, getHeight() - kHeaderH - kAddBtnH - 3 * kMargin - 18);
    m_viewport->setBounds(bounds.removeFromTop(kViewportH));

    bounds.removeFromTop(kMargin);
    m_addButton->setBounds(bounds.removeFromTop(kAddBtnH));

    // Layout list content rows
    if (m_listContent)
    {
        int contentH = int(m_paramRows.size()) * (kRowH + 2);
        m_listContent->setSize(m_viewport->getWidth() - m_viewport->getScrollBarThickness(),
                               juce::jmax(kRowH, contentH));

        // Column widths — variable section (min+max+step or stepNames) occupies the same total
        // space so the OSC address column is always the same width regardless of type.
        const int delW        = 26;
        const int typeW       = 112;
        const int nameW       = 100;
        const int minW        = 52;
        const int maxW        = 52;
        const int stepW       = 52;
        const int gap         = 3;
        const int varSectionW = minW + maxW + stepW + 2 * gap; // Continuous: min+gap+max+gap+step

        int y = 0;
        for (int i = 0; i < int(m_paramRows.size()); ++i)
        {
            auto& row      = m_paramRows[i];
            auto rowBounds = juce::Rectangle<int>(0, y, m_listContent->getWidth(), kRowH);
            y += kRowH + 2;

            auto r = rowBounds.reduced(0, 2);
            row.deleteButton->setBounds(r.removeFromLeft(delW));  r.removeFromLeft(gap);
            row.typeCombo->setBounds(r.removeFromLeft(typeW));     r.removeFromLeft(gap);
            row.nameEditor->setBounds(r.removeFromLeft(nameW));    r.removeFromLeft(gap);

            if (row.minEditor->isVisible())
            {
                row.minEditor->setBounds(r.removeFromLeft(minW));   r.removeFromLeft(gap);
                row.maxEditor->setBounds(r.removeFromLeft(maxW));   r.removeFromLeft(gap);
                row.stepEditor->setBounds(r.removeFromLeft(stepW)); r.removeFromLeft(gap);
            }
            if (row.stepNamesEditor->isVisible())
            {
                row.stepNamesEditor->setBounds(r.removeFromLeft(varSectionW)); r.removeFromLeft(gap);
            }

            row.oscAddrEditor->setBounds(r.removeFromLeft(juce::jmax(50, r.getWidth())));
        }
    }
}

void CustomParameterControlConfigDialog::setConfig(const CustomParameterConfig& config)
{
    m_config = config;

    m_hostEditor->setText(config.oscRemoteHost, juce::dontSendNotification);
    m_sendPortEditor->setText(juce::String(config.oscSendPort), juce::dontSendNotification);
    m_recvPortEditor->setText(juce::String(config.oscReceivePort), juce::dontSendNotification);

    rebuildParameterRows();
    setSize(kDialogWidth, computePreferredHeight());
    resized();
}

CustomParameterConfig CustomParameterControlConfigDialog::getConfig() const
{
    CustomParameterConfig result;
    result.oscRemoteHost  = m_hostEditor->getText().trim();
    result.oscSendPort    = m_sendPortEditor->getText().getIntValue();
    result.oscReceivePort = m_recvPortEditor->getText().getIntValue();

    for (int i = 0; i < int(m_paramRows.size()); ++i)
    {
        auto& row = m_paramRows[i];
        CustomParameterEntry entry;

        int typeIdx    = row.typeCombo->getSelectedId() - 1;
        if (typeIdx == 0)      entry.controlType = JUCEAppBasics::ParameterControlType::Continuous;
        else if (typeIdx == 1) entry.controlType = JUCEAppBasics::ParameterControlType::Discrete;
        else                   entry.controlType = JUCEAppBasics::ParameterControlType::Toggle;

        entry.name       = row.nameEditor->getText().trim();
        entry.oscAddress = row.oscAddrEditor->getText().trim();

        switch (entry.controlType)
        {
        case JUCEAppBasics::ParameterControlType::Continuous:
            entry.minValue = row.minEditor->getText().getFloatValue();
            entry.maxValue = row.maxEditor->getText().getFloatValue();
            entry.stepSize = row.stepEditor->getText().getFloatValue();
            break;
        case JUCEAppBasics::ParameterControlType::Discrete:
        {
            juce::StringArray names;
            names.addTokens(row.stepNamesEditor->getText(), ",", "");
            for (auto& n : names)
                entry.stepNames.push_back(n.trim().toStdString());
            entry.stepCount = int(entry.stepNames.size());
            entry.minValue  = 0.0f;
            entry.maxValue  = entry.stepCount > 1 ? float(entry.stepCount - 1) : 0.0f;
            entry.stepSize  = 1.0f;
            break;
        }
        case JUCEAppBasics::ParameterControlType::Toggle:
            entry.minValue = 0.0f;
            entry.maxValue = 1.0f;
            entry.stepSize = 1.0f;
            break;
        }

        result.parameters.push_back(entry);
    }

    return result;
}

void CustomParameterControlConfigDialog::rebuildParameterRows()
{
    // Remove all existing row widgets from the list content
    for (auto& row : m_paramRows)
    {
        if (m_listContent) m_listContent->removeChildComponent(row.typeCombo.get());
        if (m_listContent) m_listContent->removeChildComponent(row.nameEditor.get());
        if (m_listContent) m_listContent->removeChildComponent(row.minEditor.get());
        if (m_listContent) m_listContent->removeChildComponent(row.maxEditor.get());
        if (m_listContent) m_listContent->removeChildComponent(row.stepEditor.get());
        if (m_listContent) m_listContent->removeChildComponent(row.stepNamesEditor.get());
        if (m_listContent) m_listContent->removeChildComponent(row.oscAddrEditor.get());
        if (m_listContent) m_listContent->removeChildComponent(row.deleteButton.get());
    }
    m_paramRows.clear();

    for (int i = 0; i < int(m_config.parameters.size()); ++i)
        addParameterRow(i);
}

void CustomParameterControlConfigDialog::addParameterRow(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= int(m_config.parameters.size()))
        return;

    const auto& entry = m_config.parameters[rowIndex];
    ParamRow row;

    row.typeCombo = std::make_unique<juce::ComboBox>();
    row.typeCombo->addItem("Continuous", 1);
    row.typeCombo->addItem("Discrete",   2);
    row.typeCombo->addItem("Toggle",     3);
    int typeId = 1;
    if (entry.controlType == JUCEAppBasics::ParameterControlType::Discrete) typeId = 2;
    else if (entry.controlType == JUCEAppBasics::ParameterControlType::Toggle)  typeId = 3;
    row.typeCombo->setSelectedId(typeId, juce::dontSendNotification);
    row.typeCombo->onChange = [this, rowIndex]() { updateRowVisibility(rowIndex); };

    row.nameEditor = std::make_unique<juce::TextEditor>();
    row.nameEditor->setText(entry.name, juce::dontSendNotification);
    row.nameEditor->setTextToShowWhenEmpty("Name", juce::Colours::grey);

    row.minEditor = std::make_unique<juce::TextEditor>();
    row.minEditor->setText(juce::String(entry.minValue), juce::dontSendNotification);
    row.minEditor->setTextToShowWhenEmpty("min", juce::Colours::grey);

    row.maxEditor = std::make_unique<juce::TextEditor>();
    row.maxEditor->setText(juce::String(entry.maxValue), juce::dontSendNotification);
    row.maxEditor->setTextToShowWhenEmpty("max", juce::Colours::grey);

    row.stepEditor = std::make_unique<juce::TextEditor>();
    row.stepEditor->setText(entry.stepSize > 0.0f ? juce::String(entry.stepSize) : "",
                             juce::dontSendNotification);
    row.stepEditor->setTextToShowWhenEmpty("step", juce::Colours::grey);

    row.stepNamesEditor = std::make_unique<juce::TextEditor>();
    juce::StringArray namesArr;
    for (auto& n : entry.stepNames) namesArr.add(n);
    row.stepNamesEditor->setText(namesArr.joinIntoString(","), juce::dontSendNotification);
    row.stepNamesEditor->setTextToShowWhenEmpty("step names (csv)", juce::Colours::grey);

    row.oscAddrEditor = std::make_unique<juce::TextEditor>();
    row.oscAddrEditor->setText(entry.oscAddress, juce::dontSendNotification);
    row.oscAddrEditor->setTextToShowWhenEmpty("/osc/address", juce::Colours::grey);

    const int capturedRow = rowIndex;
    row.deleteButton = std::make_unique<juce::TextButton>("X");
    row.deleteButton->onClick = [this, capturedRow]() {
        removeParameterRow(capturedRow);
    };

    m_listContent->addAndMakeVisible(row.typeCombo.get());
    m_listContent->addAndMakeVisible(row.nameEditor.get());
    m_listContent->addAndMakeVisible(row.minEditor.get());
    m_listContent->addAndMakeVisible(row.maxEditor.get());
    m_listContent->addAndMakeVisible(row.stepEditor.get());
    m_listContent->addAndMakeVisible(row.stepNamesEditor.get());
    m_listContent->addAndMakeVisible(row.oscAddrEditor.get());
    m_listContent->addAndMakeVisible(row.deleteButton.get());

    m_paramRows.push_back(std::move(row));
    updateRowVisibility(rowIndex);
}

void CustomParameterControlConfigDialog::updateRowVisibility(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= int(m_paramRows.size()))
        return;

    auto& row = m_paramRows[rowIndex];
    const int typeId        = row.typeCombo->getSelectedId();
    const bool isContinuous = (typeId == 1);
    const bool isDiscrete   = (typeId == 2);

    row.minEditor->setVisible(isContinuous);
    row.maxEditor->setVisible(isContinuous);
    row.stepEditor->setVisible(isContinuous);
    row.stepNamesEditor->setVisible(isDiscrete);

    resized();
}

void CustomParameterControlConfigDialog::removeParameterRow(int rowIndex)
{
    // Sync config from current UI state first, then remove entry
    auto currentConfig = getConfig();
    if (rowIndex < 0 || rowIndex >= int(currentConfig.parameters.size()))
        return;
    currentConfig.parameters.erase(currentConfig.parameters.begin() + rowIndex);

    m_config = currentConfig;
    rebuildParameterRows();
    resized();
}

int CustomParameterControlConfigDialog::computePreferredHeight() const
{
    int listH = juce::jmax(kRowH, int(m_paramRows.size()) * (kRowH + 2));
    listH     = juce::jmin(listH, kMaxViewportH);
    return kHeaderH + 18 + listH + kMargin + kAddBtnH + 2 * kMargin;
}
