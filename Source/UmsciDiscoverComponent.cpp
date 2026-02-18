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

#include "UmsciDiscoverComponent.h"


#include <CustomLookAndFeel.h>


UmsciDiscoverComponent::UmsciDiscoverComponent()
    : juce::Component()
{
    //m_discoveredTopologyLabel = std::make_unique<juce::Label>("TopologyLabel", "Available Mema sessions:");
    //addAndMakeVisible(m_discoveredTopologyLabel.get());
    //
    //m_discoveredTopologyTreeView = std::make_unique<JUCEAppBasics::ServiceTopologyTreeView>(true);
    //m_discoveredTopologyTreeView->setDefaultOpenness(true);
    //addAndMakeVisible(m_discoveredTopologyTreeView.get());
    //
    //m_selectServiceButton = std::make_unique<juce::TextButton>("Join session", "Join the selected Mema session.");
    //m_selectServiceButton->onClick = [=]() {
    //    if (onServiceSelected && m_discoveredTopologyTreeView)
    //    {
    //        auto item = dynamic_cast<JUCEAppBasics::MasterServiceTreeViewItem*>(m_discoveredTopologyTreeView->getSelectedItem(0));
    //        if (nullptr != item)
    //            onServiceSelected(item->getServiceInfo());
    //    }
    //};
    //addAndMakeVisible(m_selectServiceButton.get());
}

UmsciDiscoverComponent::~UmsciDiscoverComponent()
{
}

void UmsciDiscoverComponent::paint(Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void UmsciDiscoverComponent::resized()
{
    auto labelHeight = 35;
    auto buttonHeight = 35;
    auto margin = 4;
    auto maxDiscoveryElmsWidth = 450;
    auto maxDiscoveryElmsHeight = 350;

    auto bounds = getLocalBounds().reduced(2*margin);
    if (bounds.getWidth() > maxDiscoveryElmsWidth)
    {
        auto hmargin = int((bounds.getWidth() - maxDiscoveryElmsWidth) * 0.5f);
        bounds.removeFromLeft(hmargin);
        bounds.removeFromRight(hmargin);
    }
    if (bounds.getHeight() > maxDiscoveryElmsHeight)
    {
        auto vmargin = int((bounds.getHeight() - maxDiscoveryElmsHeight) * 0.5f);
        bounds.removeFromTop(vmargin);
        bounds.removeFromBottom(vmargin);
    }

    //if (m_selectServiceButton)
    //    m_selectServiceButton->setBounds(bounds.removeFromBottom(buttonHeight));
    //bounds.removeFromBottom(margin);
    //if (m_discoveredTopologyLabel)
    //    m_discoveredTopologyLabel->setBounds(bounds.removeFromTop(labelHeight));
    //if (m_discoveredTopologyTreeView)
    //    m_discoveredTopologyTreeView->setBounds(bounds);
}

void UmsciDiscoverComponent::lookAndFeelChanged()
{
    getLookAndFeel().setColour(
        TreeView::ColourIds::selectedItemBackgroundColourId, 
        getLookAndFeel().findColour(JUCEAppBasics::CustomLookAndFeel::MeteringRmsColourId));
}

