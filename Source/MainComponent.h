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
#include "UmsciExternalControlComponent.h"
#include "UmsciZeroconfDiscoverComboComponent.h"
#include "MidiController.h"
#include "OscController.h"
#include "DbprController.h"
#include "UmsciPaintNControlComponents/UmsciDbprProjectComponent.h"
#include "UmsciPaintNControlComponents/UmsciSnapshotComponent.h"


 /**
  * Fwd. Decls
  */
class UmsciControlComponent;
class UmsciDiscoveringHintComponent;
class UmsciConnectingComponent;
class AboutComponent;

/**
 * @class MainComponent
 * @brief Root JUCE component — the top-level UI that wires together the device
 *        connection, the visualisation, and user settings.
 *
 * ## Application architecture overview
 * ```
 * JUCEApplication
 *   └── MainWindow
 *         └── MainComponent                  ← this class
 *               ├── DeviceController (singleton)  ← OCP.1 TCP + DS100 logic
 *               ├── UmsciControlComponent          ← visualisation (3 stacked layers)
 *               ├── UmsciConnectingComponent       ← shown during Connecting/Subscribing/GetValues
 *               ├── UmsciDiscoveringHintComponent  ← shown when no device is configured
 *               └── AboutComponent                 ← info overlay
 * ```
 *
 * ## Responsibility
 * - Creates and owns `DeviceController` (singleton) and `UmsciControlComponent`.
 * - Subscribes to `DeviceController::onStateChanged` to switch between the three
 *   overlay states (hint / connecting / control).
 * - Subscribes to `DeviceController::onRemoteObjectReceived` and routes decoded
 *   `RemoteObject` values to `UmsciControlComponent`.
 * - Owns `UmsciAppConfiguration` and implements its Dumper/Watcher interfaces so
 *   that all settings are persisted to/restored from XML automatically.
 * - Presents the settings popup menu (gear button) for look-and-feel, colour,
 *   upmix format, icon size, connection settings, and fullscreen mode.
 *
 * ## Settings option enum
 * `UmsciSettingsOption` enumerates every entry in the settings menu.  The integer
 * values are used as menu-item IDs in the JUCE popup menu and must not be reordered
 * without updating the corresponding `handleSettings*` handlers.
 *
 * @note [MANUAL CONTEXT NEEDED] A screenshot or wireframe of the UI annotated with
 *       component boundaries would greatly help a new developer understand how the
 *       overlaid components interact visually.
 */
class MainComponent :   public juce::Component,
                        public UmsciAppConfiguration::Dumper,
                        public UmsciAppConfiguration::Watcher
{
public:
    /**
     * @brief Enumerates every user-selectable setting exposed via the settings menu.
     *
     * Values are split into logical groups with First/Last sentinels to allow
     * range-based checks in the menu-result handlers.
     */
    enum UmsciSettingsOption
    {
        LookAndFeel_First = 1,
        LookAndFeel_FollowHost = LookAndFeel_First, ///< Inherit host application L&F.
        LookAndFeel_Dark,                           ///< Force dark colour scheme.
        LookAndFeel_Light,                          ///< Force light colour scheme.
        LookAndFeel_Last = LookAndFeel_Light,
        ControlColour_First,
        ControlColour_Green = ControlColour_First,  ///< Green source icons.
        ControlColour_Red,
        ControlColour_Blue,
        ControlColour_Pink,
        ControlColour_Laser,                        ///< Bright laser-style highlight.
        ControlColour_Last = ControlColour_Laser,
        ConnectionSettings,     ///< Opens the connection settings dialog.
        FullscreenWindowMode,   ///< Toggles fullscreen / windowed.
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
        UpmixSettings,          ///< Opens the upmix settings dialog.
        ExternalControlSettings,///< Opens the external (MIDI) control settings dialog.
        ControlSize_First,
        ControlSize_S = ControlSize_First, ///< Small icons.
        ControlSize_M,                     ///< Medium icons.
        ControlSize_L,                     ///< Large icons.
        ControlSize_Last = ControlSize_L,
    };

public:
    MainComponent();
    ~MainComponent() override;

    /** @brief Applies a single settings option (called from both menu handlers and config restore). */
    void applySettingsOption(const UmsciSettingsOption& option);

    //==============================================================================
    void resized() override;
    void paint(juce::Graphics& g) override;
    void lookAndFeelChanged() override;

    /** @brief Handles the Escape key (exits fullscreen) and F key (toggles fullscreen). */
    bool keyPressed(const juce::KeyPress& key) override;

    //==============================================================================
    /** @brief `UmsciAppConfiguration::Dumper` — serialises all current settings to XML. */
    void performConfigurationDump() override;
    /** @brief `UmsciAppConfiguration::Watcher` — called when the config XML changes on disk. */
    void onConfigUpdated() override;

    //==============================================================================
    bool isFullscreenEnabled();

    //==============================================================================
    /**
     * @brief Fired when the look-and-feel palette changes, so the host application
     *        (if this is used as a plugin) can update its own colour scheme.
     * Parameters: (paletteIndex, isDark).
     */
    std::function<void(int, bool)> onPaletteStyleChange;

    //==============================================================================
    /** @brief Fired when the user requests fullscreen mode; the host window applies it. */
    std::function<void(bool)> onSetFullscreenWindow;

private:
    //==============================================================================
    void handleSettingsMenuResult(int selectedId);
    void handleSettingsLookAndFeelMenuResult(int selectedId);
    void handleSettingsControlColourMenuResult(int selectedId);
    void handleSettingsControlFormatMenuResult(int selectedId);
    void handleSettingsControlSizeMenuResult(int selectedId);
    void handleSettingsFullscreenModeToggleResult();
    void showConnectionSettings();
    void showUpmixSettings();
    void showExternalControlSettings();
    void showDbprProjectLoad();     // called from onLoadRequested on the dbpr panel
    void syncProjectToDevice();     // called from onSyncRequested on the dbpr panel

    //==============================================================================
    /** @brief Animates the dbpr panel to the given state and records the new state. */
    void setDbprPanelState(UmsciDbprProjectComponent::PanelState state);

    /** @brief Animates the snapshot panel to the given state and records the new state. */
    void setSnapshotPanelState(UmsciSnapshotComponent::PanelState state);

    //==============================================================================
    /**
     * @brief Compares the loaded dbpr project data against the live device data
     *        and updates the dbpr panel's mismatch-flash state accordingly.
     *
     * Compares four dimensions:
     *  - MatrixInput/Soundobject names (by ID).
     *  - Loudspeaker count and positions.
     *  - Function-group count and mode values.
     *
     * Calls `m_dbprProjectComponent->setMismatchFlashing(true/false)`.
     * No-op if either controller or component is null or no project is loaded.
     */
    void checkDbprDeviceSync();

    //==============================================================================
    /** @brief Applies a domain-mapped parameter value from either MIDI or OSC to
     *         `m_controlComponent`.  Must be called on the message thread. */
    void applyUpmixParamValue(UmsciExternalControlComponent::UpmixMidiParam param, float domainValue);

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

    std::unique_ptr<UmsciSnapshotComponent>         m_snapshotComponent;

    std::unique_ptr<juce::AlertWindow>              m_messageBox;
    std::unique_ptr<UmsciZeroconfDiscoverComboComponent> m_zeroconfDiscoverComboComponent;
    std::unique_ptr<UmsciExternalControlComponent>  m_externalControlComponent;

    juce::Colour                                    m_controlColour = juce::Colours::forestgreen;

    std::unique_ptr<UmsciAppConfiguration>          m_config;

    //==============================================================================
    std::unique_ptr<MidiController>                 m_midiController;
    std::unique_ptr<OscController>                  m_oscController;

    //==============================================================================
    std::unique_ptr<DbprController>                 m_dbprController;
    std::unique_ptr<UmsciDbprProjectComponent>      m_dbprProjectComponent;
    std::unique_ptr<juce::FileChooser>              m_fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

