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
 * @class AboutComponent
 * @brief Small overlay panel showing the application icon, version info, and a
 *        link to the project repository.
 *
 * Embedded inside a `CustomAboutItem` and shown as a custom item inside a
 * JUCE PopupMenu, triggered by the about button in `MainComponent`.
 *
 * @param imageData     Raw bytes of the app icon image (SVG or PNG, compiled into
 *                      the binary as a JUCE BinaryData resource).
 * @param imageDataSize Byte count of `imageData`.
 */
class AboutComponent :   public juce::Component
{
public:
    AboutComponent(const char* imageData, int imageDataSize);
    ~AboutComponent() override;

    //========================================================================*
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::unique_ptr<juce::DrawableButton>   m_appIcon;      ///< App icon rendered as a drawable.
    std::unique_ptr<juce::Label>            m_appInfoLabel; ///< Version and build info text.
    std::unique_ptr<juce::HyperlinkButton>  m_appRepoLink;  ///< Clickable link to the GitHub repo.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutComponent)
};

