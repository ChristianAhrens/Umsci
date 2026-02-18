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

#include "UmsciComponent.h"


UmsciComponent::UmsciComponent()
    : juce::Component()
{
}

UmsciComponent::~UmsciComponent()
{
}

//void UmsciComponent::setExternalAdmOscSettings(const int ADMOSCport, const juce::IPAddress& ADMOSCremoteIP, const int ADMOSCremotePort)
//{
//    std::get<0>(m_externalAdmOscSettings) = ADMOSCport;
//    std::get<1>(m_externalAdmOscSettings) = ADMOSCremoteIP;
//    std::get<2>(m_externalAdmOscSettings) = ADMOSCremotePort;
//
//    m_panningCtrlComponent->setExternalControlSettings(ADMOSCport, ADMOSCremoteIP, ADMOSCremotePort);
//}
//
//std::tuple<int, juce::IPAddress, int> UmsciComponent::getExternalAdmOscSettings()
//{
//    return m_externalAdmOscSettings;
//}

void UmsciComponent::paint(Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::LookAndFeel_V4::ColourScheme::widgetBackground));
}

void UmsciComponent::resized()
{
    //if (m_faderbankCtrlComponent && m_faderbankCtrlComponent->isVisible())
    //    m_faderbankCtrlComponent->setBounds(getLocalBounds());
    //if (m_panningCtrlComponent && m_panningCtrlComponent->isVisible())
    //    m_panningCtrlComponent->setBounds(getLocalBounds());
    //if (m_pluginCtrlComponent && m_pluginCtrlComponent->isVisible())
    //    m_pluginCtrlComponent->setBounds(getLocalBounds());
}

void UmsciComponent::handleMessage(const Message& message)
{
    if (RunningStatus::Active != m_runningStatus)
    {
        m_runningStatus = RunningStatus::Active;
        resized();
    }
    
    //if ()
    //{
    //    DBG(juce::String(__FUNCTION__) + " handling PluginParameterValueMessage (" + juce::String(ppvm->getParameterIndex()) + "; " + ppvm->getParameterId() + "; " + juce::String(ppvm->getCurrentValue()) + ") ...");
    //
    //    if (m_pluginCtrlComponent && m_pluginCtrlComponent->isVisible())
    //        m_pluginCtrlComponent->setParameterValue(ppvm->getParameterIndex(), ppvm->getParameterId().toStdString(), ppvm->getCurrentValue());
    //
    //    resized();
    //}
}

