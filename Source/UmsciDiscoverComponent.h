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


class UmsciDiscoverComponent :   public juce::Component
{
public:
    UmsciDiscoverComponent();
    ~UmsciDiscoverComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;
    void lookAndFeelChanged() override;

    //void setupServiceDiscovery(const juce::String& serviceTypeUIDBase, const juce::String& serviceTypeUID);
    //void resetServices();

    //void setMasterServiceDescription(const juce::String& masterServiceDescription);

    //==============================================================================
    //std::function<void(const JUCEAppBasics::SessionMasterAwareService&)> onServiceSelected;

private:
    //==============================================================================
    //void setDiscoveredServiceTopology(const JUCEAppBasics::SessionServiceTopology& topology);
    
    //==============================================================================
    //std::unique_ptr<juce::Label>                            m_discoveredTopologyLabel;
    //std::unique_ptr<JUCEAppBasics::ServiceTopologyTreeView> m_discoveredTopologyTreeView;
    //std::unique_ptr<juce::TextButton>                       m_selectServiceButton;
    //
    //std::unique_ptr<JUCEAppBasics::ServiceTopologyManager>  m_serviceTopologyManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciDiscoverComponent)
};

