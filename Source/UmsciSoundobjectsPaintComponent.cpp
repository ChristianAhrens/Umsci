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

#include "UmsciSoundobjectsPaintComponent.h"


UmsciSoundobjectsPaintComponent::UmsciSoundobjectsPaintComponent()
    : juce::Component()
{
    m_hintLabel = std::make_unique<juce::Label>("Soundobjects hint label", "<Soundobjects>");
    m_hintLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_hintLabel.get());
}

UmsciSoundobjectsPaintComponent::~UmsciSoundobjectsPaintComponent()
{
}

void UmsciSoundobjectsPaintComponent::paint(juce::Graphics &g)
{
    //g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void UmsciSoundobjectsPaintComponent::resized()
{
    m_hintLabel->setBounds(getLocalBounds());
}

void UmsciSoundobjectsPaintComponent::setBoundsRealRef(const juce::Rectangle<float>& boundsRealRef)
{
    m_boundsRealRef = boundsRealRef;
}

void UmsciSoundobjectsPaintComponent::setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions)
{
}

