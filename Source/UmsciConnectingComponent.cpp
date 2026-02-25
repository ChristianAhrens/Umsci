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

#include "UmsciConnectingComponent.h"

#include <CustomLookAndFeel.h>

UmsciConnectingComponent::UmsciConnectingComponent()
    : juce::Component()
{
    m_startupProgressIndicator = std::make_unique<juce::ProgressBar>(m_progress, juce::ProgressBar::Style::circular);
    addAndMakeVisible(m_startupProgressIndicator.get());
}

UmsciConnectingComponent::~UmsciConnectingComponent()
{
}

void UmsciConnectingComponent::setConnectionStatus(Status status)
{
    juce::String statusDescription;
    switch (status)
    {
    case Status::Subscribing:
        statusDescription = "Subscribing to";
        break;
    case Status::Reading:
        statusDescription = "Reading from";
        break;
    case Status::Connecting:
    default:
        statusDescription = "Connecting to";
        break;
    }

    m_connectionStatusDescription = statusDescription;
}

void UmsciConnectingComponent::setConnectionParameters(const juce::IPAddress& ip, int port)
{
    m_connectionParametersDescription = ip.toString() + ":" + juce::String(port);
    repaint();
}

void UmsciConnectingComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));

    g.setColour(getLookAndFeel().findColour(juce::TextEditor::ColourIds::textColourId));
    g.drawFittedText(m_connectionStatusDescription + "\n" + (m_connectionParametersDescription.isNotEmpty() ? m_connectionParametersDescription : "UNKNOWN"), getLocalBounds().reduced(35), juce::Justification::centred, 2);
}

void UmsciConnectingComponent::resized()
{
    auto bounds = getLocalBounds();
    if (bounds.getWidth() > bounds.getHeight())
        bounds.reduce((bounds.getWidth() - bounds.getHeight()) / 2, 0);
    else
        bounds.reduce(0, (bounds.getHeight() - bounds.getWidth()) / 2);

    auto progressIndicatorBounds = bounds;
    progressIndicatorBounds.reduce(35, 35);
    m_startupProgressIndicator->setBounds(progressIndicatorBounds);

}

void UmsciConnectingComponent::lookAndFeelChanged()
{
    if (m_startupProgressIndicator)
        m_startupProgressIndicator->setColour(juce::ProgressBar::ColourIds::foregroundColourId, getLookAndFeel().findColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringRmsColourId));
}

