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


#include "UmsciAppConfiguration.h"

/**
 * fwd. Decls.
 */
class AboutComponent;


class UmsciComponent :   public juce::Component, juce::MessageListener
{
public:
    enum RunningStatus
    {
        Inactive,
        Standby,
        Active
    };

public:
    UmsciComponent();
    ~UmsciComponent() override;

    //void setExternalAdmOscSettings(const int ADMOSCport, const juce::IPAddress& ADMOSCremoteIP, const int ADMOSCremotePort);
    //std::tuple<int, juce::IPAddress, int> getExternalAdmOscSettings();

    //==============================================================================
    void resized() override;
    void paint(juce::Graphics& g) override;

    //==============================================================================
    void handleMessage(const Message& message) override;

    ////==============================================================================
    //std::function<void()>                           onExitClick;
    //std::function<void(const juce::MemoryBlock&)>   onMessageReadyToSend;

private:
    //==============================================================================
    RunningStatus m_runningStatus = RunningStatus::Inactive;
    static constexpr int sc_connectionTimeout = 5000; // 5s running before attempt is considered failed

    //std::pair<int, int>                                     m_currentIOCount = { 0, 0 };
    //std::map<std::uint16_t, bool>                           m_inputMuteStates = {};
    //std::map<std::uint16_t, bool>                           m_outputMuteStates = {};
    //std::map<std::uint16_t, std::map<std::uint16_t, bool>>  m_crosspointStates = {};
    //std::map<std::uint16_t, std::map<std::uint16_t, float>> m_crosspointValues = {};
    //
    //std::tuple<int, juce::IPAddress, int>   m_externalAdmOscSettings = { 4001, juce::IPAddress::local(), 4002 };

    float m_ioRatio = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciComponent)
};

