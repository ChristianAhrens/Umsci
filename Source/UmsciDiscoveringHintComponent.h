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
 * @class UmsciDiscoveringHintComponent
 * @brief Fullscreen overlay shown when no DS100 connection has been configured yet.
 *
 * Displayed by `MainComponent` in place of `UmsciControlComponent` while the
 * application is in `DeviceController::State::Disconnected` and no target IP has
 * been set.  Contains a simple label prompting the user to open the connection
 * settings dialog.
 */
class UmsciDiscoveringHintComponent :   public juce::Component
{
public:
    UmsciDiscoveringHintComponent();
    ~UmsciDiscoveringHintComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    std::unique_ptr<juce::Label>    m_hintLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciDiscoveringHintComponent)
};

