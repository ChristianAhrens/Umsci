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


/**
 * @class UmsciConnectingComponent
 * @brief Fullscreen progress overlay shown while the OCP.1 connection is being
 *        established, subscriptions are being set up, or initial values are being read.
 *
 * `MainComponent` switches to this component (hiding `UmsciControlComponent`) when
 * `DeviceController::onStateChanged` reports any state between Connecting and
 * GetValues inclusive.  When the device reaches the Connected state this overlay
 * is hidden and the control component is shown.
 *
 * Displays a JUCE indeterminate `ProgressBar` and a text description of the current
 * phase plus the target IP:port.
 */
class UmsciConnectingComponent :   public juce::Component
{
public:
    /**
     * @brief Mirrors the subset of `DeviceController::State` values that this
     *        component visualises.  The caller maps device states to these.
     */
    enum Status
    {
        Connecting,  ///< TCP connect in progress.
        Subscribing, ///< AddSubscription commands sent, waiting for ACKs.
        Reading      ///< GetValue responses being collected (DeviceController::GetValues state).
    };

public:
    UmsciConnectingComponent();
    ~UmsciConnectingComponent() override;

    /** @brief Updates the status text label to reflect the current connection phase. */
    void setConnectionStatus(Status status);
    /** @brief Updates the "connecting to ip:port" description line. */
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

