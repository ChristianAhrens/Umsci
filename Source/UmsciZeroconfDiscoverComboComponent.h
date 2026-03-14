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

#include <ZeroconfDiscoverComponent.h>


/**
 * @class UmsciZeroconfDiscoverComboComponent
 * @brief A ComboBox that continuously scans the local network for DS100 devices
 *        announced via Zeroconf/mDNS and lets the user select one to connect to.
 *
 * Used inside the connection settings dialog (`MainComponent::showConnectionSettings()`).
 * When the user picks an entry, `onServiceSelected` fires with the resolved IP and
 * port so the caller can call `DeviceController::setConnectionParameters()`.
 *
 * Backed by `ZeroconfSearcher::ZeroconfSearcher` from the JUCEAppBasics submodule,
 * which handles the mDNS browsing on a background thread.  `handleServicesChanged()`
 * is called on the message thread whenever the service list changes.
 */
class UmsciZeroconfDiscoverComboComponent
    : public juce::Component,
      public ZeroconfSearcher::ZeroconfSearcher::ZeroconfSearcherListener
{
public:
    UmsciZeroconfDiscoverComboComponent();
    ~UmsciZeroconfDiscoverComboComponent() override;

    //==============================================================================
    void resized() override;

    //==============================================================================
    /** @brief `ZeroconfSearcherListener` callback — called when devices appear or disappear. */
    void handleServicesChanged(std::string serviceName) override;

    //==============================================================================
    /**
     * @brief Fired when the user selects a device from the combo box.
     * The `ServiceInfo` contains the resolved hostname, IP address, and port.
     */
    std::function<void(const ZeroconfSearcher::ZeroconfSearcher::ServiceInfo&)> onServiceSelected;

private:
    //==============================================================================
    /** @brief Rebuilds the combo box items from the current `m_services` list. */
    void updateComboBox();

    //==============================================================================
    std::unique_ptr<ZeroconfSearcher::ZeroconfSearcher>          m_searcher; ///< mDNS browser.
    std::unique_ptr<juce::ComboBox>                              m_comboBox; ///< Device selector.
    std::vector<ZeroconfSearcher::ZeroconfSearcher::ServiceInfo> m_services; ///< Current service list.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciZeroconfDiscoverComboComponent)
};
