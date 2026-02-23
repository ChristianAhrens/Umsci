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


class UmsciConnectingComponent :   public juce::Component
{
public:
    enum Status
    {
        Connecting,
        Subscribing
    };

public:
    UmsciConnectingComponent();
    ~UmsciConnectingComponent() override;

    void setConnectionStatus(Status status);
    void setConnectionParameters(const juce::IPAddress& ip, int port);

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

private:
    double                                  m_progress = -1.0;
    std::unique_ptr<juce::ProgressBar>      m_startupProgressIndicator;
    juce::String                            m_connectionStatusDescription;
    juce::String                            m_connectionParametersDescription;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciConnectingComponent)
};

