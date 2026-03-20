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

#include "UmsciZeroconfDiscoverComboComponent.h"


UmsciZeroconfDiscoverComboComponent::UmsciZeroconfDiscoverComboComponent()
{
    m_comboBox = std::make_unique<juce::ComboBox>("ZeroconfServices");
    m_comboBox->setTextWhenNothingSelected("Searching for OCA/OCP.1 devices...");
    m_comboBox->setSelectedId(0, juce::dontSendNotification);
    m_comboBox->setEnabled(false);
    addAndMakeVisible(m_comboBox.get());

    m_comboBox->onChange = [this]() {
        auto idx = m_comboBox->getSelectedId() - 2; // id 1 = "searching" placeholder; services start at 2
        if (idx >= 0 && idx < static_cast<int>(m_services.size()) && onServiceSelected)
            onServiceSelected(m_services[static_cast<size_t>(idx)]);
    };

    m_searcher = std::make_unique<ZeroconfSearcher::ZeroconfSearcher>("OCA", "_oca._tcp");
    m_searcher->AddListener(this);
    m_searcher->StartSearching();
}

UmsciZeroconfDiscoverComboComponent::~UmsciZeroconfDiscoverComboComponent()
{
    if (m_searcher)
        m_searcher->StopSearching();
}

void UmsciZeroconfDiscoverComboComponent::resized()
{
    m_comboBox->setBounds(getLocalBounds());
}

void UmsciZeroconfDiscoverComboComponent::parentSizeChanged()
{
    if (auto* parent = getParentComponent())
        setSize(juce::roundToInt(parent->getWidth() * 0.8f), getHeight());
}

void UmsciZeroconfDiscoverComboComponent::handleServicesChanged(std::string /*serviceName*/)
{
    juce::MessageManager::callAsync([this]() { updateComboBox(); });
}

void UmsciZeroconfDiscoverComboComponent::updateComboBox()
{
    if (!m_searcher)
        return;

    m_services = m_searcher->GetServices();

    m_comboBox->clear(juce::dontSendNotification);

    if (m_services.empty())
    {
        m_comboBox->setTextWhenNothingSelected("Searching for OCA/OCP.1 devices...");
        m_comboBox->setSelectedId(0, juce::dontSendNotification);
        m_comboBox->setEnabled(false);
        return;
    }

    m_comboBox->setEnabled(true);
    m_comboBox->setTextWhenNothingSelected(juce::String(m_services.size()) + " OCA/OCP.1 devices available...");
    for (int i = 0; i < static_cast<int>(m_services.size()); ++i)
    {
        auto& s = m_services[static_cast<size_t>(i)];
        auto displayName = juce::String(s.name).upToFirstOccurrenceOf("._oca._tcp", false, true);
        auto label = displayName + "  (" + juce::String(s.ip) + ":" + juce::String(s.port) + ")";
        m_comboBox->addItem(label, i + 2);
    }
    m_comboBox->setSelectedId(0, juce::dontSendNotification);
}
