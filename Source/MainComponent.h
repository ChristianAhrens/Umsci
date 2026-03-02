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

#include "UmsciAppConfiguration.h"


 /**
  * Fwd. Decls
  */
class UmsciControlComponent;
class UmsciDiscoveringHintComponent;
class UmsciConnectingComponent;
class AboutComponent;

class MainComponent :   public juce::Component,
                        //public juce::Timer,
                        public UmsciAppConfiguration::Dumper,
                        public UmsciAppConfiguration::Watcher
{
public:
    enum UmsciSettingsOption
    {
        LookAndFeel_First = 1,
        LookAndFeel_FollowHost = LookAndFeel_First,
        LookAndFeel_Dark,
        LookAndFeel_Light,
        LookAndFeel_Last = LookAndFeel_Light,
        ControlColour_First,
        ControlColour_Green = ControlColour_First,
        ControlColour_Red,
        ControlColour_Blue,
        ControlColour_Pink,
        ControlColour_Laser,
        ControlColour_Last = ControlColour_Laser,
        ConnectionSettings,
        FullscreenWindowMode,
        ControlFormat_First,
        ControlFormat_Stereo = ControlFormat_First,
        ControlFormat_LRS,
        ControlFormat_LCRS,
        ControlFormat_5point0,
        ControlFormat_5point1,
        ControlFormat_5point1point2,
        ControlFormat_7point0,
        ControlFormat_7point1,
        ControlFormat_7point1point4,
        ControlFormat_9point1point6,
        ControlFormat_Last = ControlFormat_9point1point6,
        UpmixSettings
    };

public:
    MainComponent();
    ~MainComponent() override;

    void applySettingsOption(const UmsciSettingsOption& option);

    //==============================================================================
    void resized() override;
    void paint(juce::Graphics& g) override;
    void lookAndFeelChanged() override;

    bool keyPressed(const juce::KeyPress& key) override;

    //==============================================================================
    void performConfigurationDump() override;
    void onConfigUpdated() override;

    //==============================================================================
    bool isFullscreenEnabled();

    //==============================================================================
    std::function<void(int, bool)> onPaletteStyleChange;

    //==============================================================================
    std::function<void(bool)> onSetFullscreenWindow;

private:
    //==============================================================================
    void handleSettingsMenuResult(int selectedId);
    void handleSettingsLookAndFeelMenuResult(int selectedId);
    void handleSettingsControlColourMenuResult(int selectedId);
    void handleSettingsControlFormatMenuResult(int selectedId);
    void handleSettingsFullscreenModeToggleResult();
    void showConnectionSettings();
    void showUpmixSettings();

    //==============================================================================
    void setControlColour(const juce::Colour& meteringColour);
    void applyControlColour();

    void toggleFullscreenMode();

    //==============================================================================
    std::unique_ptr<UmsciControlComponent>          m_controlComponent;
    std::unique_ptr<UmsciDiscoveringHintComponent>  m_discoverHintComponent;
    std::unique_ptr<UmsciConnectingComponent>       m_connectingComponent;

    std::unique_ptr<juce::DrawableButton>           m_settingsButton;
    std::map<int, std::pair<std::string, int>>      m_settingsItems;
    int                                             m_settingsHostLookAndFeelId = -1;

    std::unique_ptr<juce::DrawableButton>           m_connectionToggleButton;

    std::unique_ptr<juce::DrawableButton>           m_aboutButton;
    std::unique_ptr<AboutComponent>                 m_aboutComponent;

    std::unique_ptr<juce::AlertWindow>              m_messageBox;

    juce::Colour                                    m_controlColour = juce::Colours::forestgreen;

    std::unique_ptr<UmsciAppConfiguration>          m_config;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

