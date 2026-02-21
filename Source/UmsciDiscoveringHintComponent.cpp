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

#include "UmsciDiscoveringHintComponent.h"


UmsciDiscoveringHintComponent::UmsciDiscoveringHintComponent()
    : juce::Component()
{
}

UmsciDiscoveringHintComponent::~UmsciDiscoveringHintComponent()
{
}

void UmsciDiscoveringHintComponent::paint(Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));

    g.setColour(getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId));
    g.drawText("Toggle on the connection with the parameters\nspecified in settings to start\ncontrolling the signal engine.", juce::Rectangle<float>(250.0f, 250.0f).withCentre(getLocalBounds().getCentre().toFloat()), juce::Justification::centred);
}
