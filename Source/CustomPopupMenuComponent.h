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
 * @class CustomAboutItem
 * @brief A `juce::PopupMenu::CustomComponent` wrapper that embeds an arbitrary
 *        `juce::Component` as a full-size item inside a JUCE PopupMenu.
 *
 * Used to show `AboutComponent` inside the settings popup menu without requiring
 * a separate window.  `getIdealSize()` sizes the item to 75 % of the main display
 * dimension (square), clamped to a caller-supplied minimum.
 *
 * Ownership of `componentToHold` is *not* transferred — the caller must keep the
 * component alive for at least as long as this item exists.
 */
class CustomAboutItem : public juce::PopupMenu::CustomComponent
{
public:
	CustomAboutItem(juce::Component* componentToHold, juce::Rectangle<int> minIdealSize)
	{
		m_component = componentToHold;
		addAndMakeVisible(m_component);

		m_minIdealSize = minIdealSize;
	}
    ~CustomAboutItem()
	{
		if (m_component)
			m_component->setVisible(false);
	}

    void getIdealSize(int& idealWidth, int& idealHeight) override
	{
		auto resultingIdealSize = juce::Rectangle<int>(idealWidth, idealHeight);
		auto mc = juce::Desktop::getInstance().getComponent(0);
		if (mc)
		{
			auto fBounds = mc->getBounds().toFloat();
			auto h = fBounds.getHeight();
			auto w = fBounds.getWidth();
			if (h > 0.0f && w > 0.0f)
			{
				if (h > w)
				{
					w = 0.75f * w;
					h = w;
				}
				else
				{
					h = 0.75f * h;
					w = h;
				}

				resultingIdealSize = juce::Rectangle<float>(w, h).toNearestInt();
			}
		}

		if (resultingIdealSize.getWidth() < m_minIdealSize.getWidth() && resultingIdealSize.getHeight() < m_minIdealSize.getHeight())
		{
			idealWidth = m_minIdealSize.getWidth();
			idealHeight = m_minIdealSize.getHeight();
		}
		else
		{
			idealWidth = resultingIdealSize.getWidth();
			idealHeight = resultingIdealSize.getHeight();
		}
	}

    void resized() override
	{
		if (m_component)
			m_component->setBounds(getLocalBounds());
	}

private:
    juce::Component*        m_component = nullptr;
    juce::Rectangle<int>    m_minIdealSize;
};

