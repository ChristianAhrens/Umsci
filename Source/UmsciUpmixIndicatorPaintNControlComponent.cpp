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

#include "UmsciUpmixIndicatorPaintNControlComponent.h"


UmsciUpmixIndicatorPaintNControlComponent::UmsciUpmixIndicatorPaintNControlComponent()
    : juce::Component()
{
    m_hintLabel = std::make_unique<juce::Label>("UpmixIndicator hint label", "<Upmix indicator>");
    m_hintLabel->setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(m_hintLabel.get());
}

UmsciUpmixIndicatorPaintNControlComponent::~UmsciUpmixIndicatorPaintNControlComponent()
{
}

void UmsciUpmixIndicatorPaintNControlComponent::paint(juce::Graphics &g)
{
    //g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void UmsciUpmixIndicatorPaintNControlComponent::resized()
{
    m_hintLabel->setBounds(getLocalBounds());
}

void UmsciUpmixIndicatorPaintNControlComponent::setBoundsRealRef(const juce::Rectangle<float>& boundsRealRef)
{
    m_boundsRealRef = boundsRealRef;
}

void UmsciUpmixIndicatorPaintNControlComponent::setUpmixIndicatorParameters(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions)
{
}

