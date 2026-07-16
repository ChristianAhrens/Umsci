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

#include "UmsciUpmixSettingsComponent.h"

UmsciUpmixSettingsComponent::UmsciUpmixSettingsComponent()
{
    addAndMakeVisible(m_controlFormatLabel);
    addAndMakeVisible(m_controlFormatCombo);

    addAndMakeVisible(m_liveModeLabel);
    m_liveModeCombo.addItem("Manual (double-click to apply)", 1);
    m_liveModeCombo.addItem("Live (apply changes immediately)", 2);
    addAndMakeVisible(m_liveModeCombo);

    addAndMakeVisible(m_shapeLabel);
    m_shapeCombo.addItem("Circle", 1);
    m_shapeCombo.addItem("Rectangle", 2);
    addAndMakeVisible(m_shapeCombo);

    addAndMakeVisible(m_visualizationLabel);
    m_visualizationCombo.addItem("Dots & Line", 1);
    m_visualizationCombo.addItem("Solid Bar", 2);
    addAndMakeVisible(m_visualizationCombo);

    addAndMakeVisible(m_startIdLabel);
    addAndMakeVisible(m_startIdEditor);

    addAndMakeVisible(m_showSourcesLabel);
    m_showSourcesCombo.addItem("All", 1);
    m_showSourcesCombo.addItem("Upmix controlled only", 2);
    addAndMakeVisible(m_showSourcesCombo);

    addAndMakeVisible(m_lfeLabel);
    m_lfeCombo.addItem("Disregard", 1);
    m_lfeCombo.addItem("Position with bed", 2);
    addAndMakeVisible(m_lfeCombo);

    addAndMakeVisible(m_levelMeterLabel);
    m_levelMeterCombo.addItem("Off", 1);
    m_levelMeterCombo.addItem("On", 2);
    addAndMakeVisible(m_levelMeterCombo);

    addAndMakeVisible(m_channelLabelsLabel);
    m_channelLabelsCombo.addItem("Off", 1);
    m_channelLabelsCombo.addItem("On", 2);
    addAndMakeVisible(m_channelLabelsCombo);

    setSize(kDialogWidth, getPreferredHeight());
}

UmsciUpmixSettingsComponent::~UmsciUpmixSettingsComponent() = default;

int UmsciUpmixSettingsComponent::getPreferredHeight() const
{
    return kMargin * 2 + kRowCount * kRowH + (kRowCount - 1) * kRowGap;
}

void UmsciUpmixSettingsComponent::resized()
{
    auto bounds = getLocalBounds().reduced(kMargin);

    auto layoutRow = [&](juce::Label& label, juce::Component& control)
    {
        auto row = bounds.removeFromTop(kRowH);
        label.setBounds(row.removeFromLeft(kLabelW));
        control.setBounds(row);
        bounds.removeFromTop(kRowGap);
    };

    layoutRow(m_controlFormatLabel, m_controlFormatCombo);
    layoutRow(m_liveModeLabel, m_liveModeCombo);
    layoutRow(m_shapeLabel, m_shapeCombo);
    layoutRow(m_visualizationLabel, m_visualizationCombo);
    layoutRow(m_startIdLabel, m_startIdEditor);
    layoutRow(m_showSourcesLabel, m_showSourcesCombo);
    layoutRow(m_lfeLabel, m_lfeCombo);
    layoutRow(m_levelMeterLabel, m_levelMeterCombo);
    layoutRow(m_channelLabelsLabel, m_channelLabelsCombo);
}

void UmsciUpmixSettingsComponent::setControlFormatItems(const juce::StringArray& items, int selectedIndex)
{
    m_controlFormatCombo.clear(juce::dontSendNotification);
    for (int i = 0; i < items.size(); ++i)
        m_controlFormatCombo.addItem(items[i], i + 1);
    m_controlFormatCombo.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
}

int UmsciUpmixSettingsComponent::getControlFormatIndex() const
{
    return m_controlFormatCombo.getSelectedItemIndex();
}

void UmsciUpmixSettingsComponent::setLiveMode(bool liveMode)
{
    m_liveModeCombo.setSelectedItemIndex(liveMode ? 1 : 0, juce::dontSendNotification);
}

bool UmsciUpmixSettingsComponent::getLiveMode() const
{
    return m_liveModeCombo.getSelectedItemIndex() == 1;
}

void UmsciUpmixSettingsComponent::setShapeIsRectangle(bool isRectangle)
{
    m_shapeCombo.setSelectedItemIndex(isRectangle ? 1 : 0, juce::dontSendNotification);
}

bool UmsciUpmixSettingsComponent::getShapeIsRectangle() const
{
    return m_shapeCombo.getSelectedItemIndex() == 1;
}

void UmsciUpmixSettingsComponent::setVisualizationIsSolidBar(bool isSolidBar)
{
    m_visualizationCombo.setSelectedItemIndex(isSolidBar ? 1 : 0, juce::dontSendNotification);
}

bool UmsciUpmixSettingsComponent::getVisualizationIsSolidBar() const
{
    return m_visualizationCombo.getSelectedItemIndex() == 1;
}

void UmsciUpmixSettingsComponent::setStartSoundobjectId(int startId)
{
    m_startIdEditor.setText(juce::String(startId), juce::dontSendNotification);
}

int UmsciUpmixSettingsComponent::getStartSoundobjectId() const
{
    return m_startIdEditor.getText().getIntValue();
}

void UmsciUpmixSettingsComponent::setShowAllSources(bool showAll)
{
    m_showSourcesCombo.setSelectedItemIndex(showAll ? 0 : 1, juce::dontSendNotification);
}

bool UmsciUpmixSettingsComponent::getShowAllSources() const
{
    return m_showSourcesCombo.getSelectedItemIndex() == 0;
}

void UmsciUpmixSettingsComponent::setShowLfeChannel(bool show)
{
    m_lfeCombo.setSelectedItemIndex(show ? 1 : 0, juce::dontSendNotification);
}

bool UmsciUpmixSettingsComponent::getShowLfeChannel() const
{
    return m_lfeCombo.getSelectedItemIndex() == 1;
}

void UmsciUpmixSettingsComponent::setShowLevelMeter(bool show)
{
    m_levelMeterCombo.setSelectedItemIndex(show ? 1 : 0, juce::dontSendNotification);
}

bool UmsciUpmixSettingsComponent::getShowLevelMeter() const
{
    return m_levelMeterCombo.getSelectedItemIndex() == 1;
}

void UmsciUpmixSettingsComponent::setShowChannelLabels(bool show)
{
    m_channelLabelsCombo.setSelectedItemIndex(show ? 1 : 0, juce::dontSendNotification);
}

bool UmsciUpmixSettingsComponent::getShowChannelLabels() const
{
    return m_channelLabelsCombo.getSelectedItemIndex() == 1;
}
