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

#include "UmsciLoudspeakersPaintComponent.h"


UmsciLoudspeakersPaintComponent::UmsciLoudspeakersPaintComponent()
    : juce::Component()
{
    m_hintLabel = std::make_unique<juce::Label>("Loudspeakers hint label", "<Loudspeakers>");
    m_hintLabel->setJustificationType(juce::Justification::centredTop);
    addAndMakeVisible(m_hintLabel.get());
}

UmsciLoudspeakersPaintComponent::~UmsciLoudspeakersPaintComponent()
{
}

void UmsciLoudspeakersPaintComponent::paint(juce::Graphics &g)
{
    //g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void UmsciLoudspeakersPaintComponent::resized()
{
    m_hintLabel->setBounds(getLocalBounds());
}

