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

#include "MainComponent.h"

#include "DeviceController.h"

#include "CustomPopupMenuComponent.h"
#include "UmsciControlComponent.h"
#include "UmsciDiscoveringHintComponent.h"
#include "UmsciConnectingComponent.h"

#include "AboutComponent.h"
#include "UmsciExternalControlComponent.h"
#include "UmsciPaintNControlComponents/UmsciDbprProjectComponent.h"

#include <CustomLookAndFeel.h>
#include <WebUpdateDetector.h>

#include <iOS_utils.h>


MainComponent::MainComponent()
    : juce::Component()
{
    // create the configuration object (is being initialized from disk automatically)
    m_config = std::make_unique<UmsciAppConfiguration>(JUCEAppBasics::AppConfigurationBase::getDefaultConfigFilePath());
    m_config->addDumper(this);

    // check if config creation was able to read a valid config from disk...
    if (!m_config->isValid())
    {
        m_config->ResetToDefault();
    }

    // create different main components and add them
    m_controlComponent = std::make_unique<UmsciControlComponent>();
    addAndMakeVisible(m_controlComponent.get());

    m_discoverHintComponent = std::make_unique<UmsciDiscoveringHintComponent>();
    addAndMakeVisible(m_discoverHintComponent.get());

    m_connectingComponent = std::make_unique<UmsciConnectingComponent>();
    addAndMakeVisible(m_connectingComponent.get());

    // initially activate the 'offline state' reg. component configuration
    m_connectingComponent->setVisible(false);
    m_controlComponent->setVisible(false);
    m_discoverHintComponent->setVisible(true);

    m_aboutComponent = std::make_unique<AboutComponent>(BinaryData::UmsciRect_png, BinaryData::UmsciRect_pngSize);
    m_aboutButton = std::make_unique<juce::DrawableButton>("About", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_aboutButton->setTooltip(juce::String("About") + juce::JUCEApplication::getInstance()->getApplicationName());
    m_aboutButton->onClick = [this] {
        juce::PopupMenu aboutMenu;
        aboutMenu.addCustomItem(1, std::make_unique<CustomAboutItem>(m_aboutComponent.get(), juce::Rectangle<int>(250, 250)), nullptr, juce::String("Info about") + juce::JUCEApplication::getInstance()->getApplicationName());
        aboutMenu.showMenuAsync(juce::PopupMenu::Options());
    };
    m_aboutButton->setAlwaysOnTop(true);
    m_aboutButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_aboutButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(m_aboutButton.get());

    // Ctrl+F11 on Windows/Linux, Cmd+Ctrl+F on macOS
#if JUCE_WINDOWS
    auto fullscreenShortCutHint = std::string(" (Ctrl+F11)");
#elif JUCE_MAC
    auto fullscreenShortCutHint = std::string(" (Cmd+Ctrl+F)");
#endif

    // default lookandfeel is follow local, therefor none selected
    m_settingsItems[UmsciSettingsOption::LookAndFeel_FollowHost] = std::make_pair("Follow host", 0);
    m_settingsItems[UmsciSettingsOption::LookAndFeel_Dark] = std::make_pair("Dark", 1);
    m_settingsItems[UmsciSettingsOption::LookAndFeel_Light] = std::make_pair("Light", 0);
    // default output visu is 5.0
    m_settingsItems[UmsciSettingsOption::ControlFormat_Stereo] = std::make_pair(juce::AudioChannelSet::stereo().getDescription().toStdString(), 0);
    m_settingsItems[UmsciSettingsOption::ControlFormat_LRS] = std::make_pair(juce::AudioChannelSet::createLRS().getDescription().toStdString(), 0);
    m_settingsItems[UmsciSettingsOption::ControlFormat_LCRS] = std::make_pair(juce::AudioChannelSet::createLCRS().getDescription().toStdString(), 0);
    m_settingsItems[UmsciSettingsOption::ControlFormat_5point0] = std::make_pair(juce::AudioChannelSet::create5point0().getDescription().toStdString(), 1);
    m_settingsItems[UmsciSettingsOption::ControlFormat_5point1] = std::make_pair(juce::AudioChannelSet::create5point1().getDescription().toStdString(), 0);
    m_settingsItems[UmsciSettingsOption::ControlFormat_5point1point2] = std::make_pair(juce::AudioChannelSet::create5point1point2().getDescription().toStdString(), 0);
    m_settingsItems[UmsciSettingsOption::ControlFormat_7point0] = std::make_pair(juce::AudioChannelSet::create7point0().getDescription().toStdString(), 0);
    m_settingsItems[UmsciSettingsOption::ControlFormat_7point1] = std::make_pair(juce::AudioChannelSet::create7point1().getDescription().toStdString(), 0);
    m_settingsItems[UmsciSettingsOption::ControlFormat_7point1point4] = std::make_pair(juce::AudioChannelSet::create7point1point4().getDescription().toStdString(), 0);
    m_settingsItems[UmsciSettingsOption::ControlFormat_9point1point6] = std::make_pair(juce::AudioChannelSet::create9point1point6().getDescription().toStdString(), 0);
    // default panning colour is green
    m_settingsItems[UmsciSettingsOption::ControlColour_Green] = std::make_pair("Green", 1);
    m_settingsItems[UmsciSettingsOption::ControlColour_Red] = std::make_pair("Red", 0);
    m_settingsItems[UmsciSettingsOption::ControlColour_Blue] = std::make_pair("Blue", 0);
    m_settingsItems[UmsciSettingsOption::ControlColour_Pink] = std::make_pair("Anni Pink", 0);
    m_settingsItems[UmsciSettingsOption::ControlColour_Laser] = std::make_pair("Laser", 0);
    // connection settings
    m_settingsItems[UmsciSettingsOption::ConnectionSettings] = std::make_pair("Connection settings...", 0);
    // upmix settings
    m_settingsItems[UmsciSettingsOption::UpmixSettings] = std::make_pair("Upmix control settings...", 0);
    // external (MIDI) control settings
    m_settingsItems[UmsciSettingsOption::ExternalControlSettings] = std::make_pair("External control...", 0);
    // dbpr project import
    m_settingsItems[UmsciSettingsOption::DbprProjectLoad] = std::make_pair("Load dbpr project...", 0);
    // control size
    m_settingsItems[UmsciSettingsOption::ControlSize_S] = std::make_pair("S", 1);
    m_settingsItems[UmsciSettingsOption::ControlSize_M] = std::make_pair("M", 0);
    m_settingsItems[UmsciSettingsOption::ControlSize_L] = std::make_pair("L", 0);
#if JUCE_WINDOWS || JUCE_MAC
    // fullscreen toggling
    m_settingsItems[UmsciSettingsOption::FullscreenWindowMode] = std::make_pair("Toggle fullscreen mode" + fullscreenShortCutHint, 0);
#endif
    // Further components
    m_settingsButton = std::make_unique<juce::DrawableButton>("Settings", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_settingsButton->setTooltip(juce::String("Settings for ") + juce::JUCEApplication::getInstance()->getApplicationName());
    m_settingsButton->onClick = [this] {
        juce::PopupMenu lookAndFeelSubMenu;
        for (int i = UmsciSettingsOption::LookAndFeel_First; i <= UmsciSettingsOption::LookAndFeel_Last; i++)
            lookAndFeelSubMenu.addItem(i, m_settingsItems[i].first, true, m_settingsItems[i].second == 1);

        juce::PopupMenu controlColourSubMenu;
        for (int i = UmsciSettingsOption::ControlColour_First; i <= UmsciSettingsOption::ControlColour_Last; i++)
            controlColourSubMenu.addItem(i, m_settingsItems[i].first, true, m_settingsItems[i].second == 1);

        juce::PopupMenu controlSizeSubMenu;
        for (int i = UmsciSettingsOption::ControlSize_First; i <= UmsciSettingsOption::ControlSize_Last; i++)
            controlSizeSubMenu.addItem(i, m_settingsItems[i].first, true, m_settingsItems[i].second == 1);

        juce::PopupMenu settingsMenu;
        settingsMenu.addSubMenu("LookAndFeel", lookAndFeelSubMenu);
        settingsMenu.addSubMenu("Control colour", controlColourSubMenu);
        settingsMenu.addSubMenu("Control size", controlSizeSubMenu);
        settingsMenu.addSeparator();
        settingsMenu.addItem(UmsciSettingsOption::ConnectionSettings, m_settingsItems[UmsciSettingsOption::ConnectionSettings].first, true, false);
        settingsMenu.addItem(UmsciSettingsOption::UpmixSettings, m_settingsItems[UmsciSettingsOption::UpmixSettings].first, true, false);
        settingsMenu.addItem(UmsciSettingsOption::ExternalControlSettings, m_settingsItems[UmsciSettingsOption::ExternalControlSettings].first, true, false);
        settingsMenu.addSeparator();
        settingsMenu.addItem(UmsciSettingsOption::DbprProjectLoad, m_settingsItems[UmsciSettingsOption::DbprProjectLoad].first, true, false);
#if JUCE_WINDOWS || JUCE_MAC
        settingsMenu.addSeparator();
        settingsMenu.addItem(UmsciSettingsOption::FullscreenWindowMode, m_settingsItems[UmsciSettingsOption::FullscreenWindowMode].first, true, false);
#endif
        settingsMenu.showMenuAsync(juce::PopupMenu::Options(), [=](int selectedId) {
            handleSettingsMenuResult(selectedId);
            if (m_config)
                m_config->triggerConfigurationDump();
        });
    };
    m_settingsButton->setAlwaysOnTop(true);
    m_settingsButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_settingsButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(m_settingsButton.get());

    m_connectionToggleButton = std::make_unique<juce::DrawableButton>("ConnectionToggle", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_connectionToggleButton->setTooltip("Toggle connection to device.");
    m_connectionToggleButton->onClick = [this] {
        if (m_connectionToggleButton->getToggleState())
            DeviceController::getInstance()->connect();
        else
        {
            DeviceController::getInstance()->disconnect();
            m_controlComponent->resetData();
        }

        lookAndFeelChanged();

        if (m_config)
            m_config->triggerConfigurationDump();
    };
    m_connectionToggleButton->setAlwaysOnTop(true);
    m_connectionToggleButton->setClickingTogglesState(true);
    m_connectionToggleButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_connectionToggleButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(m_connectionToggleButton.get());
    
    if (juce::JUCEApplication::getInstance()->getCommandLineParameters().contains("--noconfigui"))
    {
        m_aboutButton->setVisible(false);
        m_settingsButton->setVisible(false);
        m_connectionToggleButton->setVisible(false);
    }

    m_snapshotComponent = std::make_unique<UmsciSnapshotComponent>();
    m_snapshotComponent->setHighlightColour(m_controlColour);
    addAndMakeVisible(m_snapshotComponent.get());
    m_snapshotComponent->setVisible(false);

    m_snapshotComponent->onStoreRequested = [this] {
        if (!m_controlComponent) return;
        m_snapshotComponent->setSnapshotData({
            m_controlComponent->getUpmixRot(),
            m_controlComponent->getUpmixTrans(),
            m_controlComponent->getUpmixHeightTrans(),
            m_controlComponent->getUpmixAngleStretch(),
            m_controlComponent->getUpmixOffsetX(),
            m_controlComponent->getUpmixOffsetY()
        });
        m_snapshotComponent->setRecallEnabled(true);
        if (m_config)
            m_config->triggerConfigurationDump();
    };

    m_snapshotComponent->onRecallRequested = [this] {
        if (!m_controlComponent) return;
        const auto& snapData = m_snapshotComponent->getSnapshotData();
        if (!snapData.has_value()) return;
        const auto& s = *snapData;
        m_controlComponent->setUpmixTransform(s.rot, s.scale, s.heightScale, s.angleStretch);
        m_controlComponent->setUpmixOffset(s.offsetX, s.offsetY);
        if (m_controlComponent->getUpmixLiveMode())
            m_controlComponent->triggerUpmixTransformApplied();
        else
            m_controlComponent->triggerUpmixFlashCheck();
        if (m_config)
            m_config->triggerConfigurationDump();
    };

    m_controlComponent->onUpmixTransformChanged = [this]() {
        if (m_config)
            m_config->triggerConfigurationDump();
    };

    m_controlComponent->onDatabaseComplete = [this]() {
        checkDbprDeviceSync();
    };

    m_controlComponent->onDeviceDataUpdated = [this]() {
        checkDbprDeviceSync();
    };

    DeviceController::getInstance()->onStateChanged = [=](DeviceController::State s) {
        auto noconfigui = juce::JUCEApplication::getInstance()->getCommandLineParameters().contains("--noconfigui");
        switch (s)
        {
            case DeviceController::State::Disconnected:
                m_connectionToggleButton->setToggleState(false, juce::dontSendNotification);
                m_connectingComponent->setVisible(false);
                m_connectingComponent->setConnectionStatus(UmsciConnectingComponent::Status::Connecting);
                m_controlComponent->setVisible(false);
                m_discoverHintComponent->setVisible(true);
                if (m_snapshotComponent) m_snapshotComponent->setVisible(false);
                if (m_dbprProjectComponent)
                {
                    m_dbprProjectComponent->setVisible(false);
                    m_dbprProjectComponent->setMismatchFlashing(false);
                }
                break;
            case DeviceController::State::Connecting:
                m_connectionToggleButton->setToggleState(true, juce::dontSendNotification);
                m_controlComponent->setVisible(false);
                m_discoverHintComponent->setVisible(false);
                m_connectingComponent->setVisible(true);
                m_connectingComponent->setConnectionStatus(UmsciConnectingComponent::Status::Connecting);
                if (m_snapshotComponent) m_snapshotComponent->setVisible(false);
                if (m_dbprProjectComponent) m_dbprProjectComponent->setVisible(false);
                break;
            case DeviceController::State::Subscribing:
                m_connectionToggleButton->setToggleState(true, juce::dontSendNotification);
                m_controlComponent->setVisible(false);
                m_discoverHintComponent->setVisible(false);
                m_connectingComponent->setVisible(true);
                m_connectingComponent->setConnectionStatus(UmsciConnectingComponent::Status::Subscribing);
                if (m_snapshotComponent) m_snapshotComponent->setVisible(false);
                if (m_dbprProjectComponent) m_dbprProjectComponent->setVisible(false);
                break;
            case DeviceController::State::GetValues:
                m_connectionToggleButton->setToggleState(true, juce::dontSendNotification);
                m_connectingComponent->setVisible(true);
                m_connectingComponent->setConnectionStatus(UmsciConnectingComponent::Status::Reading);
                m_discoverHintComponent->setVisible(false);
                m_controlComponent->setVisible(false);
                if (m_snapshotComponent) m_snapshotComponent->setVisible(false);
                if (m_dbprProjectComponent) m_dbprProjectComponent->setVisible(false);
                break;
            case DeviceController::State::Connected:
                m_connectionToggleButton->setToggleState(true, juce::dontSendNotification);
                m_connectingComponent->setVisible(false);
                m_discoverHintComponent->setVisible(false);
                m_controlComponent->setVisible(true);
                if (!noconfigui)
                {
                    if (m_snapshotComponent) m_snapshotComponent->setVisible(true);
                    if (m_dbprProjectComponent) m_dbprProjectComponent->setVisible(true);
                }
                break;
            default:
                break;
        }
    };

#ifdef RUN_MESSAGE_TESTS
    Mema::runTests();
#endif

    setSize(800, 600);

#if defined JUCE_IOS
    // iOS is updated via AppStore
#define IGNORE_UPDATES
#elif defined JUCE_ANDROID
    // Android as well
#define IGNORE_UPDATES
#endif

#if defined IGNORE_UPDATES
#else
    auto noUpdates = juce::JUCEApplication::getInstance()->getCommandLineParameters().contains("--noupdates");
    if (!noUpdates)
    {
        auto updater = JUCEAppBasics::WebUpdateDetector::getInstance();
        updater->SetReferenceVersion(ProjectInfo::versionString);
        updater->SetDownloadUpdateWebAddress("https://github.com/christianahrens/umsci/releases/latest");
        updater->CheckForNewVersion(true, "https://raw.githubusercontent.com/ChristianAhrens/Umsci/refs/heads/main/");
    }
#endif


    // MIDI and OSC controllers
    m_midiController = std::make_unique<MidiController>();
    m_midiController->onParamValueChanged = [this](UmsciExternalControlComponent::UpmixMidiParam param, float domainValue) {
        applyUpmixParamValue(param, domainValue);
    };

    m_oscController = std::make_unique<OscController>();
    m_oscController->onParamValueChanged = [this](UmsciExternalControlComponent::UpmixMidiParam param, float domainValue) {
        applyUpmixParamValue(param, domainValue);
    };

    // dbpr project controller and floating panel
    m_dbprController = std::make_unique<DbprController>();
    m_dbprProjectComponent = std::make_unique<UmsciDbprProjectComponent>();
    m_dbprProjectComponent->setHighlightColour(m_controlColour);
    addAndMakeVisible(m_dbprProjectComponent.get());
    m_dbprProjectComponent->setVisible(false);

    m_dbprController->onProjectLoaded = [this](const dbpr::ProjectData& data) {
        if (m_dbprProjectComponent)
        {
            m_dbprProjectComponent->setProjectData(data);
            setDbprPanelState(UmsciDbprProjectComponent::PanelState::Visible);
        }

        checkDbprDeviceSync();

        if (m_config)
            m_config->triggerConfigurationDump();

        // Warn if the En-Scene inputs don't align with the current upmix configuration
        if (m_controlComponent)
        {
            const auto startId      = m_controlComponent->getUpmixSourceStartId();
            const auto channelCount = static_cast<int>(m_controlComponent->getUpmixChannelConfiguration().size());

            // Check that every input in the range [startId, startId+channelCount-1] is En-Scene.
            // Any gap (missing or InputMode=0) means upmix will not work correctly.
            juce::StringArray missingIds;
            for (int id = startId; id < startId + channelCount; ++id)
            {
                auto it = data.matrixInputData.find(id);
                if (it == data.matrixInputData.end() || !it->second.isEnScene())
                    missingIds.add(juce::String(id));
            }

            if (!missingIds.isEmpty())
            {
                juce::String warning = "The loaded project's En-Scene inputs do not cover the full upmix range.\n"
                    "Upmix expects inputs " + juce::String(startId)
                    + " to " + juce::String(startId + channelCount - 1)
                    + " to be set to En-Scene, but the following are not:\n  "
                    + missingIds.joinIntoString(", ")
                    + "\nPlease adjust the upmix configuration or the project to avoid mismatches.";

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Upmix Configuration Mismatch",
                    warning);
            }
        }
    };

    m_dbprController->onProjectLoadFailed = [](const std::string& errorMsg) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Project Load Failed",
            juce::String(errorMsg));
    };

    m_dbprProjectComponent->onStateChangeRequested = [this](UmsciDbprProjectComponent::PanelState newState) {
        setDbprPanelState(newState);
    };

    m_snapshotComponent->onStateChangeRequested = [this](UmsciSnapshotComponent::PanelState newState) {
        setSnapshotPanelState(newState);
    };

    m_dbprProjectComponent->onDeleteRequested = [this] {
        if (m_dbprController)
            m_dbprController->clearProjectData();
        if (m_dbprProjectComponent)
            m_dbprProjectComponent->clearProjectData();
        checkDbprDeviceSync();
        if (m_config)
            m_config->triggerConfigurationDump();
    };

    // add this main component to watchers
    m_config->addWatcher(this); // without initial update - that we have to do externally after lambdas were assigned

    // we want keyboard focus for fullscreen toggle shortcut
    setWantsKeyboardFocus(true);

    lookAndFeelChanged();

    // On iOS/iPadOS, UIKit populates safeAreaInsets asynchronously after the
    // window is shown. initialise() registers a callback so resized() is re-run
    // once the insets are valid and on any subsequent geometry change
    // (Stage Manager, split-screen, etc.). No-op on all other platforms.
    JUCEAppBasics::iOS_utils::initialise([this] {
        juce::MessageManager::callAsync([this] { resized(); });
    });
}

MainComponent::~MainComponent()
{
    JUCEAppBasics::iOS_utils::deinitialise();
}

void MainComponent::resized()
{
    auto safety = JUCEAppBasics::iOS_utils::getDeviceSafetyMargins();
    auto safeBounds = getLocalBounds();
    safeBounds.removeFromTop(safety._top);
    safeBounds.removeFromBottom(safety._bottom);
    safeBounds.removeFromLeft(safety._left);
    safeBounds.removeFromRight(safety._right);
    
    m_controlComponent->setBounds(safeBounds);
    m_connectingComponent->setBounds(safeBounds);
    m_discoverHintComponent->setBounds(safeBounds);

    const auto panelW    = UmsciDbprProjectComponent::s_panelWidth;
    const auto panelH    = UmsciDbprProjectComponent::s_panelHeight;
    const auto panelTopY = safeBounds.getBottom() - panelH - UmsciDbprProjectComponent::s_panelMargin;

    if (m_dbprProjectComponent)
    {
        const auto x = (m_dbprProjectComponent->getPanelState() == UmsciDbprProjectComponent::PanelState::Tucked)
            ? -(panelW - UmsciDbprProjectComponent::s_grabStripWidth)
            : UmsciDbprProjectComponent::s_panelMargin;
        m_dbprProjectComponent->setBounds(x, panelTopY, panelW, panelH);
    }

    if (!juce::JUCEApplication::getInstance()->getCommandLineParameters().contains("--noconfigui"))
    {
        auto leftButtons  = safeBounds.removeFromLeft(36);
        auto rightButtons = safeBounds.removeFromLeft(36);
        m_aboutButton->setBounds(leftButtons.removeFromTop(35).removeFromBottom(30));
        m_settingsButton->setBounds(leftButtons.removeFromTop(35).removeFromBottom(30));
        m_connectionToggleButton->setBounds(rightButtons.removeFromTop(35).removeFromBottom(30));

        // Snapshot panel sits directly above the dbpr panel.
        if (m_snapshotComponent)
        {
            const auto snapW = UmsciSnapshotComponent::s_panelWidth;
            const auto snapH = UmsciSnapshotComponent::s_panelHeight;
            const auto snapTopY = panelTopY - UmsciSnapshotComponent::s_panelMargin - snapH;
            const auto snapX = (m_snapshotComponent->getPanelState() == UmsciSnapshotComponent::PanelState::Tucked)
                ? -(snapW - UmsciSnapshotComponent::s_grabStripWidth)
                : UmsciSnapshotComponent::s_panelMargin;
            m_snapshotComponent->setBounds(snapX, snapTopY, snapW, snapH);
        }
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::lookAndFeelChanged()
{
    auto aboutButtonDrawable = juce::Drawable::createFromSVG(*juce::XmlDocument::parse(BinaryData::question_mark_24dp_svg).get());
    aboutButtonDrawable->replaceColour(juce::Colours::black, getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId));
    m_aboutButton->setImages(aboutButtonDrawable.get());

    auto settingsDrawable = juce::Drawable::createFromSVG(*juce::XmlDocument::parse(BinaryData::settings_24dp_svg).get());
    settingsDrawable->replaceColour(juce::Colours::black, getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId));
    m_settingsButton->setImages(settingsDrawable.get());

    auto connectedDrawable = juce::Drawable::createFromSVG(*juce::XmlDocument::parse(BinaryData::link_off_24dp_svg).get());
    connectedDrawable->replaceColour(juce::Colours::black, getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId));
    auto disconnectedDrawable = juce::Drawable::createFromSVG(*juce::XmlDocument::parse(BinaryData::link_24dp_svg).get());
    disconnectedDrawable->replaceColour(juce::Colours::black, getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOffId));
    if (m_connectionToggleButton->getToggleState())
        m_connectionToggleButton->setImages(connectedDrawable.get());
    else
        m_connectionToggleButton->setImages(disconnectedDrawable.get());

    applyControlColour();
}

void MainComponent::applySettingsOption(const UmsciSettingsOption& option)
{
    // use the settings menu item call infrastructure to set the option
    handleSettingsMenuResult(option);
}

void MainComponent::handleSettingsMenuResult(int selectedId)
{
    if (0 == selectedId)
        return; // nothing selected, dismiss
    else if (UmsciSettingsOption::LookAndFeel_First <= selectedId && UmsciSettingsOption::LookAndFeel_Last >= selectedId)
        handleSettingsLookAndFeelMenuResult(selectedId);
    else if (UmsciSettingsOption::ControlColour_First <= selectedId && UmsciSettingsOption::ControlColour_Last >= selectedId)
        handleSettingsControlColourMenuResult(selectedId);
    else if (UmsciSettingsOption::ConnectionSettings == selectedId)
        showConnectionSettings();
    else if (UmsciSettingsOption::FullscreenWindowMode == selectedId)
        handleSettingsFullscreenModeToggleResult();
    else if (UmsciSettingsOption::ControlFormat_First <= selectedId && UmsciSettingsOption::ControlFormat_Last >= selectedId)
        handleSettingsControlFormatMenuResult(selectedId);
    else if (UmsciSettingsOption::UpmixSettings == selectedId)
        showUpmixSettings();
    else if (UmsciSettingsOption::ExternalControlSettings == selectedId)
        showExternalControlSettings();
    else if (UmsciSettingsOption::ControlSize_First <= selectedId && UmsciSettingsOption::ControlSize_Last >= selectedId)
        handleSettingsControlSizeMenuResult(selectedId);
    else if (UmsciSettingsOption::DbprProjectLoad == selectedId)
        showDbprProjectLoad();
    else
        jassertfalse; // unhandled menu entry!?
}

void MainComponent::handleSettingsControlFormatMenuResult(int selectedId)
{
    // helper internal function to avoid code clones
    std::function<void(int, int, int, int, int, int, int, int, int, int)> setSettingsItemsCheckState = [=](int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
        m_settingsItems[UmsciSettingsOption::ControlFormat_Stereo].second = a;
        m_settingsItems[UmsciSettingsOption::ControlFormat_LRS].second = b;
        m_settingsItems[UmsciSettingsOption::ControlFormat_LCRS].second = c;
        m_settingsItems[UmsciSettingsOption::ControlFormat_5point0].second = d;
        m_settingsItems[UmsciSettingsOption::ControlFormat_5point1].second = e;
        m_settingsItems[UmsciSettingsOption::ControlFormat_5point1point2].second = f;
        m_settingsItems[UmsciSettingsOption::ControlFormat_7point0].second = g;
        m_settingsItems[UmsciSettingsOption::ControlFormat_7point1].second = h;
        m_settingsItems[UmsciSettingsOption::ControlFormat_7point1point4].second = i;
        m_settingsItems[UmsciSettingsOption::ControlFormat_9point1point6].second = j;
        };

    switch (selectedId)
    {
    case UmsciSettingsOption::ControlFormat_Stereo:
        setSettingsItemsCheckState(1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::stereo());
        break;
    case UmsciSettingsOption::ControlFormat_LRS:
        setSettingsItemsCheckState(0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::createLRS());
        break;
    case UmsciSettingsOption::ControlFormat_LCRS:
        setSettingsItemsCheckState(0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::createLCRS());
        break;
    case UmsciSettingsOption::ControlFormat_5point0:
        setSettingsItemsCheckState(0, 0, 0, 1, 0, 0, 0, 0, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::create5point0());
        break;
    case UmsciSettingsOption::ControlFormat_5point1:
        setSettingsItemsCheckState(0, 0, 0, 0, 1, 0, 0, 0, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::create5point1());
        break;
    case UmsciSettingsOption::ControlFormat_5point1point2:
        setSettingsItemsCheckState(0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::create5point1point2());
        break;
    case UmsciSettingsOption::ControlFormat_7point0:
        setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::create7point0());
        break;
    case UmsciSettingsOption::ControlFormat_7point1:
        setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 0, 1, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::create7point1());
        break;
    case UmsciSettingsOption::ControlFormat_7point1point4:
        setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::create7point1point4());
        break;
    case UmsciSettingsOption::ControlFormat_9point1point6:
        setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
        if (m_controlComponent)
            m_controlComponent->setUpmixChannelConfiguration(juce::AudioChannelSet::create9point1point6());
        break;
    default:
        jassertfalse; // unknown id fed in unintentionally ?!
        break;
    }

    resized();
}

void MainComponent::handleSettingsControlSizeMenuResult(int selectedId)
{
    std::function<void(int, int, int)> setSettingsItemsCheckState = [=](int s, int m, int l) {
        m_settingsItems[UmsciSettingsOption::ControlSize_S].second = s;
        m_settingsItems[UmsciSettingsOption::ControlSize_M].second = m;
        m_settingsItems[UmsciSettingsOption::ControlSize_L].second = l;
    };

    switch (selectedId)
    {
    case UmsciSettingsOption::ControlSize_S:
        setSettingsItemsCheckState(1, 0, 0);
        if (m_controlComponent)
            m_controlComponent->setControlsSize(UmsciPaintNControlComponentBase::ControlsSize::S);
        break;
    case UmsciSettingsOption::ControlSize_M:
        setSettingsItemsCheckState(0, 1, 0);
        if (m_controlComponent)
            m_controlComponent->setControlsSize(UmsciPaintNControlComponentBase::ControlsSize::M);
        break;
    case UmsciSettingsOption::ControlSize_L:
        setSettingsItemsCheckState(0, 0, 1);
        if (m_controlComponent)
            m_controlComponent->setControlsSize(UmsciPaintNControlComponentBase::ControlsSize::L);
        break;
    default:
        jassertfalse;
        break;
    }
}

void MainComponent::handleSettingsLookAndFeelMenuResult(int selectedId)
{
    // helper internal function to avoid code clones
    std::function<void(int, int, int)> setSettingsItemsCheckState = [=](int a, int b, int c) {
        m_settingsItems[UmsciSettingsOption::LookAndFeel_FollowHost].second = a;
        m_settingsItems[UmsciSettingsOption::LookAndFeel_Dark].second = b;
        m_settingsItems[UmsciSettingsOption::LookAndFeel_Light].second = c;
    };

    switch (selectedId)
    {
    case UmsciSettingsOption::LookAndFeel_FollowHost:
        setSettingsItemsCheckState(1, 0, 0);
        if (onPaletteStyleChange && m_settingsHostLookAndFeelId != -1)
            onPaletteStyleChange(m_settingsHostLookAndFeelId, false);
        break;
    case UmsciSettingsOption::LookAndFeel_Dark:
        setSettingsItemsCheckState(0, 1, 0);
        if (onPaletteStyleChange)
            onPaletteStyleChange(JUCEAppBasics::CustomLookAndFeel::PS_Dark, false);
        break;
    case UmsciSettingsOption::LookAndFeel_Light:
        setSettingsItemsCheckState(0, 0, 1);
        if (onPaletteStyleChange)
            onPaletteStyleChange(JUCEAppBasics::CustomLookAndFeel::PS_Light, false);
        break;
    default:
        jassertfalse; // unknown id fed in unintentionally ?!
        break;
    }
}

void MainComponent::handleSettingsControlColourMenuResult(int selectedId)
{
    // helper internal function to avoid code clones
    std::function<void(int, int, int, int, int)> setSettingsItemsCheckState = [=](int green, int red, int blue, int pink, int laser) {
        m_settingsItems[UmsciSettingsOption::ControlColour_Green].second = green;
        m_settingsItems[UmsciSettingsOption::ControlColour_Red].second = red;
        m_settingsItems[UmsciSettingsOption::ControlColour_Blue].second = blue;
        m_settingsItems[UmsciSettingsOption::ControlColour_Pink].second = pink;
        m_settingsItems[UmsciSettingsOption::ControlColour_Laser].second = laser;
    };

    switch (selectedId)
    {
    case UmsciSettingsOption::ControlColour_Green:
        setSettingsItemsCheckState(1, 0, 0, 0, 0);
        setControlColour(juce::Colours::forestgreen);
        break;
    case UmsciSettingsOption::ControlColour_Red:
        setSettingsItemsCheckState(0, 1, 0, 0, 0);
        setControlColour(juce::Colours::orangered);
        break;
    case UmsciSettingsOption::ControlColour_Blue:
        setSettingsItemsCheckState(0, 0, 1, 0, 0);
        setControlColour(juce::Colours::dodgerblue);
        break;
    case UmsciSettingsOption::ControlColour_Pink:
        setSettingsItemsCheckState(0, 0, 0, 1, 0);
        setControlColour(juce::Colours::deeppink);
        break;
    case UmsciSettingsOption::ControlColour_Laser:
        setSettingsItemsCheckState(0, 0, 0, 0, 1);
        setControlColour(juce::Colour(0xd1, 0xff, 0x4f));
        break;
    default:
        jassertfalse; // unknown id fed in unintentionally ?!
        break;
    }
}

void MainComponent::handleSettingsFullscreenModeToggleResult()
{
    toggleFullscreenMode();
}

void MainComponent::toggleFullscreenMode()
{
    auto enabled = isFullscreenEnabled();
    if (onSetFullscreenWindow)
        onSetFullscreenWindow(!enabled);
}

void MainComponent::showConnectionSettings()
{
    m_messageBox = std::make_unique<juce::AlertWindow>(
        "Control connection settings",
        "Info: This machine uses IP " + juce::IPAddress::getLocalAddress().toString(),
        juce::MessageBoxIconType::NoIcon);

    auto currentOCP1connPar = DeviceController::getInstance()->getConnectionParameters();

    m_messageBox->addTextBlock("\nOCA/OCP.1 connection parameters:");
    
    m_zeroconfDiscoverComboComponent = std::make_unique<UmsciZeroconfDiscoverComboComponent>();
    m_zeroconfDiscoverComboComponent->setSize(10, 26); // width is set dynamically via parentSizeChanged()
    m_zeroconfDiscoverComboComponent->onServiceSelected = [this](const ZeroconfSearcher::ZeroconfSearcher::ServiceInfo& service) {
        if (auto* ed = m_messageBox->getTextEditor("Device IP"))
            ed->setText(juce::String(service.ip), juce::sendNotification);
        if (auto* ed = m_messageBox->getTextEditor("Device port"))
            ed->setText(juce::String(service.port), juce::sendNotification);
        auto matrixSizeIt = service.txtRecords.find("db_matrixSize");
        if (matrixSizeIt != service.txtRecords.end())
            if (auto* ed = m_messageBox->getTextEditor("Device IO size"))
                ed->setText(juce::String(matrixSizeIt->second), juce::sendNotification);
    };
    m_messageBox->addCustomComponent(m_zeroconfDiscoverComboComponent.get());

    m_messageBox->addTextEditor("Device IP", std::get<0>(currentOCP1connPar).toString(), "OCP.1 IP");
    m_messageBox->addTextEditor("Device port", juce::String(std::get<1>(currentOCP1connPar)), "OCP.1 port");
    m_messageBox->addTextEditor("Device IO size", juce::String(m_controlComponent->getOcp1IOSize().first) + "x" + juce::String(m_controlComponent->getOcp1IOSize().second), "OCP.1 IOSize");

    m_messageBox->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    m_messageBox->addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
    m_messageBox->enterModalState(true, juce::ModalCallbackFunction::create([=](int returnValue) {
        if (returnValue == 1)
        {
            auto ocp1IP = juce::IPAddress(m_messageBox->getTextEditorContents("Device IP"));
            auto ocp1Port = m_messageBox->getTextEditorContents("Device port").getIntValue();
            auto ocp1IOsize = m_messageBox->getTextEditorContents("Device IO size");

            if (m_connectingComponent)
                m_connectingComponent->setConnectionParameters(ocp1IP, ocp1Port);

            m_controlComponent->setOcp1IOSize({ ocp1IOsize.upToFirstOccurrenceOf("x", false, true).getIntValue(), ocp1IOsize.fromLastOccurrenceOf("x", false, true).getIntValue() });
            DeviceController::getInstance()->setConnectionParameters(ocp1IP, ocp1Port);

            if (m_config)
                m_config->triggerConfigurationDump();
        }

        m_zeroconfDiscoverComboComponent.reset();
        m_messageBox.reset();
    }));
}

void MainComponent::showUpmixSettings()
{
    m_messageBox = std::make_unique<juce::AlertWindow>(
        "Upmix control settings",
        "Configure the upmix overlay control settings.",
        juce::MessageBoxIconType::NoIcon);

    juce::StringArray formatItems;
    int currentFormatIndex = 0;
    for (int i = UmsciSettingsOption::ControlFormat_First; i <= UmsciSettingsOption::ControlFormat_Last; i++)
    {
        formatItems.add(m_settingsItems[i].first);
        if (m_settingsItems[i].second == 1)
            currentFormatIndex = i - UmsciSettingsOption::ControlFormat_First;
    }
    m_messageBox->addComboBox("Control format", formatItems, "Channel format");
    if (auto* combo = m_messageBox->getComboBoxComponent("Control format"))
        combo->setSelectedItemIndex(currentFormatIndex, juce::dontSendNotification);

    juce::StringArray liveModeItems;
    liveModeItems.add("Manual (double-click to apply)");
    liveModeItems.add("Live (apply changes immediately)");
    m_messageBox->addComboBox("Live mode", liveModeItems, "Control mode");
    if (auto* combo = m_messageBox->getComboBoxComponent("Live mode"))
        combo->setSelectedItemIndex(m_controlComponent->getUpmixLiveMode() ? 1 : 0,
                                    juce::dontSendNotification);

    juce::StringArray shapeItems;
    shapeItems.add("Circle");
    shapeItems.add("Rectangle");
    m_messageBox->addComboBox("Shape", shapeItems, "Indicator shape");
    if (auto* combo = m_messageBox->getComboBoxComponent("Shape"))
        combo->setSelectedItemIndex(m_controlComponent->getUpmixShape() == UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape::Rectangle ? 1 : 0,
                                    juce::dontSendNotification);

    m_messageBox->addTextEditor("Start soundobject ID",
        juce::String(m_controlComponent->getUpmixSourceStartId()),
        "First soundobject");

    juce::StringArray showSourcesItems;
    showSourcesItems.add("All");
    showSourcesItems.add("Upmix controlled only");
    m_messageBox->addComboBox("Show sources", showSourcesItems, "Visible soundobjects");
    if (auto* combo = m_messageBox->getComboBoxComponent("Show sources"))
        combo->setSelectedItemIndex(m_controlComponent->getShowAllSources() ? 0 : 1,
                                    juce::dontSendNotification);

    m_messageBox->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    m_messageBox->addButton("Ok",     1, juce::KeyPress(juce::KeyPress::returnKey));
    m_messageBox->enterModalState(true, juce::ModalCallbackFunction::create([=](int returnValue) {
        if (returnValue == 1)
        {
            if (auto* combo = m_messageBox->getComboBoxComponent("Control format"))
                handleSettingsControlFormatMenuResult(UmsciSettingsOption::ControlFormat_First + combo->getSelectedItemIndex());
            if (auto* combo = m_messageBox->getComboBoxComponent("Live mode"))
                m_controlComponent->setUpmixLiveMode(combo->getSelectedItemIndex() == 1);
            if (auto* combo = m_messageBox->getComboBoxComponent("Shape"))
                m_controlComponent->setUpmixShape(combo->getSelectedItemIndex() == 1
                    ? UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape::Rectangle
                    : UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape::Circle);
            auto startId = m_messageBox->getTextEditorContents("Start soundobject ID").getIntValue();
            m_controlComponent->setUpmixSourceStartId(startId);
            if (auto* combo = m_messageBox->getComboBoxComponent("Show sources"))
                m_controlComponent->setShowAllSources(combo->getSelectedItemIndex() == 0);
            if (m_config)
                m_config->triggerConfigurationDump();
        }
        m_messageBox.reset();
    }));
}

void MainComponent::setControlColour(const juce::Colour& controlColour)
{
    m_controlColour = controlColour;

    lookAndFeelChanged();

    if (m_connectingComponent)
        m_connectingComponent->lookAndFeelChanged();
}

void MainComponent::applyControlColour()
{
    auto customLookAndFeel = dynamic_cast<JUCEAppBasics::CustomLookAndFeel*>(&getLookAndFeel());
    if (customLookAndFeel)
    {
        switch (customLookAndFeel->getPaletteStyle())
        {
        case JUCEAppBasics::CustomLookAndFeel::PS_Light:
            getLookAndFeel().setColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringPeakColourId, m_controlColour.brighter());
            getLookAndFeel().setColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringRmsColourId, m_controlColour);
            break;
        case JUCEAppBasics::CustomLookAndFeel::PS_Dark:
        default:
            getLookAndFeel().setColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringPeakColourId, m_controlColour.darker());
            getLookAndFeel().setColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringRmsColourId, m_controlColour);
            break;
        }
    }

    if (m_dbprProjectComponent)
        m_dbprProjectComponent->setHighlightColour(m_controlColour);

    if (m_snapshotComponent)
        m_snapshotComponent->setHighlightColour(m_controlColour);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Ctrl+F11 on Windows/Linux, Cmd+Ctrl+F on macOS
    if (key == juce::KeyPress(juce::KeyPress::F11Key, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('f', juce::ModifierKeys::commandModifier | juce::ModifierKeys::ctrlModifier, 0))
    {
        toggleFullscreenMode();
        return true;
    }
    return false;
}

void MainComponent::performConfigurationDump()
{
    if (m_config)
    {
        // control config
        if (m_controlComponent)
        {
            auto controlConfigXmlElement = m_controlComponent->createStateXml();
            if (controlConfigXmlElement)
                m_config->setConfigState(std::move(controlConfigXmlElement), UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLCONFIG));
        }

        // connection config
        auto connectionConfigXmlElement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONNECTIONCONFIG));
        auto params = DeviceController::getInstance()->getConnectionParameters();
        connectionConfigXmlElement->setAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::ENABLED), (m_connectionToggleButton ? (m_connectionToggleButton->getToggleState() ? 1 : 0) : 0));
        connectionConfigXmlElement->setAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::IP), std::get<0>(params).toString());
        connectionConfigXmlElement->setAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::PORT), std::get<1>(params));
        m_config->setConfigState(std::move(connectionConfigXmlElement), UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONNECTIONCONFIG));

        // visu config
        auto visuConfigXmlElement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::VISUCONFIG));
        
        auto lookAndFeelXmlElmement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::LOOKANDFEEL));
        for (int i = UmsciSettingsOption::LookAndFeel_First; i <= UmsciSettingsOption::LookAndFeel_Last; i++)
        {
            if (m_settingsItems[i].second == 1)
                lookAndFeelXmlElmement->addTextElement(juce::String(i));
        }
        visuConfigXmlElement->addChildElement(lookAndFeelXmlElmement.release());
        
        auto controlColourXmlElmement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLCOLOUR));
        for (int i = UmsciSettingsOption::ControlColour_First; i <= UmsciSettingsOption::ControlColour_Last; i++)
        {
            if (m_settingsItems[i].second == 1)
                controlColourXmlElmement->addTextElement(juce::String(i));
        }
        visuConfigXmlElement->addChildElement(controlColourXmlElmement.release());

        auto controlFormatXmlElmement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLFORMAT));
        for (int i = UmsciSettingsOption::ControlFormat_First; i <= UmsciSettingsOption::ControlFormat_Last; i++)
        {
            if (m_settingsItems[i].second == 1)
                controlFormatXmlElmement->addTextElement(juce::String(i));
        }
        visuConfigXmlElement->addChildElement(controlFormatXmlElmement.release());

        auto controlSizeXmlElement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLSIZE));
        for (int i = UmsciSettingsOption::ControlSize_First; i <= UmsciSettingsOption::ControlSize_Last; i++)
        {
            if (m_settingsItems[i].second == 1)
                controlSizeXmlElement->addTextElement(juce::String(i));
        }
        visuConfigXmlElement->addChildElement(controlSizeXmlElement.release());

        m_config->setConfigState(std::move(visuConfigXmlElement), UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::VISUCONFIG));

        // upmix config
        auto upmixConfigXmlElement = std::make_unique<juce::XmlElement>(
            UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::UPMIXCONFIG));
        upmixConfigXmlElement->setAttribute(
            UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::UPMIXSOURCESTARTID),
            (m_controlComponent ? m_controlComponent->getUpmixSourceStartId() : 1));
        upmixConfigXmlElement->setAttribute(
            UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::UPMIXLIVEMODE),
            (m_controlComponent ? (m_controlComponent->getUpmixLiveMode() ? 1 : 0) : 0));
        upmixConfigXmlElement->setAttribute(
            UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::UPMIXSHAPE),
            UmsciUpmixIndicatorPaintNControlComponent::getShapeName(
                m_controlComponent ? m_controlComponent->getUpmixShape()
                                   : UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape::Circle));
        upmixConfigXmlElement->setAttribute(
            UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::UPMIXSHOWALLSOURCES),
            (m_controlComponent ? (m_controlComponent->getShowAllSources() ? 1 : 0) : 1));
        upmixConfigXmlElement->addTextElement(UmsciSnapshotComponent::UpmixSnapshot{
            m_controlComponent ? m_controlComponent->getUpmixRot()          : 0.0f,
            m_controlComponent ? m_controlComponent->getUpmixTrans()        : 1.0f,
            m_controlComponent ? m_controlComponent->getUpmixHeightTrans()  : 0.6f,
            m_controlComponent ? m_controlComponent->getUpmixAngleStretch() : 1.0f,
            m_controlComponent ? m_controlComponent->getUpmixOffsetX()      : 0.0f,
            m_controlComponent ? m_controlComponent->getUpmixOffsetY()      : 0.0f
        }.toString());

        m_config->setConfigState(std::move(upmixConfigXmlElement),
            UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::UPMIXCONFIG));

        // upmix snapshot (optional — only written when a snapshot has been stored)
        if (m_snapshotComponent && m_snapshotComponent->getSnapshotData().has_value())
        {
            auto snapshotXmlElement = std::make_unique<juce::XmlElement>(
                UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::UPMIXSNAPSHOTCONFIG));
            snapshotXmlElement->addTextElement(m_snapshotComponent->getSnapshotData()->toString());
            m_config->setConfigState(std::move(snapshotXmlElement),
                UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::UPMIXSNAPSHOTCONFIG));
        }

        // dbpr project config — always written so clearing removes any previously persisted data
        if (m_dbprController)
        {
            auto dbprXmlElement = std::make_unique<juce::XmlElement>(
                UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::DBPRPROJECTCONFIG));
            if (m_dbprController->hasProjectLoaded())
                dbprXmlElement->addTextElement(juce::String(m_dbprController->getProjectData().toString()));
            m_config->setConfigState(std::move(dbprXmlElement),
                UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::DBPRPROJECTCONFIG));
        }

        // external control (MIDI) config
        static const UmsciAppConfiguration::TagID midiParamTagIds[] = {
            UmsciAppConfiguration::TagID::MIDI_UPMIXROT,
            UmsciAppConfiguration::TagID::MIDI_UPMIXSCALE,
            UmsciAppConfiguration::TagID::MIDI_UPMIXHEIGHTSCALE,
            UmsciAppConfiguration::TagID::MIDI_UPMIXANGLESTRETCH,
            UmsciAppConfiguration::TagID::MIDI_UPMIXOFFSETX,
            UmsciAppConfiguration::TagID::MIDI_UPMIXOFFSETY
        };

        auto extCtrlXmlElement = std::make_unique<juce::XmlElement>(
            UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::EXTERNALCONTROLCONFIG));

        auto midiDeviceXmlElement = std::make_unique<juce::XmlElement>(
            UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::MIDIINPUTDEVICE));
        midiDeviceXmlElement->addTextElement(m_midiController->getDeviceIdentifier());
        extCtrlXmlElement->addChildElement(midiDeviceXmlElement.release());

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            auto assiXmlElement = std::make_unique<juce::XmlElement>(
                UmsciAppConfiguration::getTagName(midiParamTagIds[i]));
            assiXmlElement->addTextElement(m_midiController->getAssignment(
                static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i)).serializeToHexString());
            extCtrlXmlElement->addChildElement(assiXmlElement.release());
        }

        auto oscPortXmlElement = std::make_unique<juce::XmlElement>(
            UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::OSCINPUTPORT));
        oscPortXmlElement->addTextElement(juce::String(m_oscController->getPort()));
        extCtrlXmlElement->addChildElement(oscPortXmlElement.release());

        static const UmsciAppConfiguration::TagID oscParamTagIds[] = {
            UmsciAppConfiguration::TagID::OSC_UPMIXROT,
            UmsciAppConfiguration::TagID::OSC_UPMIXSCALE,
            UmsciAppConfiguration::TagID::OSC_UPMIXHEIGHTSCALE,
            UmsciAppConfiguration::TagID::OSC_UPMIXANGLESTRETCH,
            UmsciAppConfiguration::TagID::OSC_UPMIXOFFSETX,
            UmsciAppConfiguration::TagID::OSC_UPMIXOFFSETY
        };

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            auto addrXmlElement = std::make_unique<juce::XmlElement>(
                UmsciAppConfiguration::getTagName(oscParamTagIds[i]));
            addrXmlElement->addTextElement(m_oscController->getAddress(
                static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i)));
            extCtrlXmlElement->addChildElement(addrXmlElement.release());
        }

        m_config->setConfigState(std::move(extCtrlXmlElement),
            UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::EXTERNALCONTROLCONFIG));
    }
}

void MainComponent::onConfigUpdated()
{
    // control config
    auto controlConfigState = m_config->getConfigState(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLCONFIG));
    if (controlConfigState && m_controlComponent)
        m_controlComponent->setStateXml(controlConfigState.get());

    // connection config
    auto connectionConfigState = m_config->getConfigState(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONNECTIONCONFIG));
    if (connectionConfigState)
    {
        auto ocp1ConnectionEnabled = 1 == connectionConfigState->getIntAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::ENABLED));
        auto ocp1IP = juce::IPAddress(connectionConfigState->getStringAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::IP)));
        auto ocp1Port = connectionConfigState->getIntAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::PORT));

        if (m_connectingComponent)
            m_connectingComponent->setConnectionParameters(ocp1IP, ocp1Port);

        auto ocp1ConParams = DeviceController::getInstance()->getConnectionParameters();
        if (std::get<0>(ocp1ConParams) != ocp1IP || std::get<1>(ocp1ConParams) != ocp1Port)
        {
            DeviceController::getInstance()->setConnectionParameters(ocp1IP, ocp1Port);
        }

        if (ocp1ConnectionEnabled != m_connectionToggleButton->getToggleState())
        {
            m_connectionToggleButton->setToggleState(ocp1ConnectionEnabled, juce::dontSendNotification);
            if (ocp1ConnectionEnabled)
                DeviceController::getInstance()->connect();
            else
                DeviceController::getInstance()->disconnect();
        }
    }

    // visu config
    auto visuConfigState = m_config->getConfigState(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::VISUCONFIG));
    if (visuConfigState)
    {
        auto lookAndFeelXmlElement = visuConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::LOOKANDFEEL));
        if (lookAndFeelXmlElement)
        {
            auto lookAndFeelSettingsOptionId = lookAndFeelXmlElement->getAllSubText().getIntValue();
            handleSettingsLookAndFeelMenuResult(lookAndFeelSettingsOptionId);
        }

        auto controlColourXmlElement = visuConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLCOLOUR));
        if (controlColourXmlElement)
        {
            auto controlColourSettingsOptionId = controlColourXmlElement->getAllSubText().getIntValue();
            handleSettingsControlColourMenuResult(controlColourSettingsOptionId);
        }

        auto controlFormatXmlElement = visuConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLFORMAT));
        if (controlFormatXmlElement)
        {
            auto controlFormatSettingsOptionId = controlFormatXmlElement->getAllSubText().getIntValue();
            handleSettingsControlFormatMenuResult(controlFormatSettingsOptionId);
        }

        auto controlSizeXmlElement = visuConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLSIZE));
        if (controlSizeXmlElement)
        {
            auto controlSizeSettingsOptionId = controlSizeXmlElement->getAllSubText().getIntValue();
            handleSettingsControlSizeMenuResult(controlSizeSettingsOptionId);
        }
    }

    // upmix config
    auto upmixConfigState = m_config->getConfigState(
        UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::UPMIXCONFIG));
    if (upmixConfigState && m_controlComponent)
    {
        auto startId = upmixConfigState->getIntAttribute(
            UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::UPMIXSOURCESTARTID), 1);
        m_controlComponent->setUpmixSourceStartId(startId);
        auto showAllSources = upmixConfigState->getIntAttribute(
            UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::UPMIXSHOWALLSOURCES), 1) == 1;
        m_controlComponent->setShowAllSources(showAllSources);
        auto liveMode = upmixConfigState->getIntAttribute(
            UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::UPMIXLIVEMODE), 0) == 1;
        m_controlComponent->setUpmixLiveMode(liveMode);
        auto upmixShape = UmsciUpmixIndicatorPaintNControlComponent::getShapeForName(
            upmixConfigState->getStringAttribute(
                UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::UPMIXSHAPE)));
        m_controlComponent->setUpmixShape(upmixShape);
        auto upmixParams = UmsciSnapshotComponent::UpmixSnapshot::fromString(upmixConfigState->getAllSubText());
        m_controlComponent->setUpmixTransform(upmixParams.rot, upmixParams.scale,
                                              upmixParams.heightScale, upmixParams.angleStretch);
        m_controlComponent->setUpmixOffset(upmixParams.offsetX, upmixParams.offsetY);
    }

    // upmix snapshot (optional — absent in config means no snapshot stored)
    auto upmixSnapshotState = m_config->getConfigState(
        UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::UPMIXSNAPSHOTCONFIG));
    if (upmixSnapshotState && m_snapshotComponent)
    {
        m_snapshotComponent->setSnapshotData(
            UmsciSnapshotComponent::UpmixSnapshot::fromString(upmixSnapshotState->getAllSubText()));
        m_snapshotComponent->setRecallEnabled(true);
    }

    // dbpr project config (optional — absent in config means no project was previously loaded)
    auto dbprProjectState = m_config->getConfigState(
        UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::DBPRPROJECTCONFIG));
    if (dbprProjectState && m_dbprController)
    {
        auto projectData = dbpr::ProjectData::fromString(dbprProjectState->getAllSubText().toStdString());
        if (!projectData.isEmpty())
        {
            m_dbprController->setProjectData(projectData);
            if (m_dbprProjectComponent)
                m_dbprProjectComponent->setProjectData(projectData);
        }
    }

    // external control (MIDI) config
    auto extCtrlState = m_config->getConfigState(
        UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::EXTERNALCONTROLCONFIG));
    if (extCtrlState)
    {
        if (auto* midiDevXml = extCtrlState->getChildByName(
                UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::MIDIINPUTDEVICE)))
            m_midiController->openDevice(midiDevXml->getAllSubText());

        static const UmsciAppConfiguration::TagID midiParamTagIds[] = {
            UmsciAppConfiguration::TagID::MIDI_UPMIXROT,
            UmsciAppConfiguration::TagID::MIDI_UPMIXSCALE,
            UmsciAppConfiguration::TagID::MIDI_UPMIXHEIGHTSCALE,
            UmsciAppConfiguration::TagID::MIDI_UPMIXANGLESTRETCH,
            UmsciAppConfiguration::TagID::MIDI_UPMIXOFFSETX,
            UmsciAppConfiguration::TagID::MIDI_UPMIXOFFSETY
        };

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            if (auto* assiXml = extCtrlState->getChildByName(
                    UmsciAppConfiguration::getTagName(midiParamTagIds[i])))
            {
                auto hexStr = assiXml->getAllSubText();
                if (hexStr.isNotEmpty())
                {
                    JUCEAppBasics::MidiCommandRangeAssignment assi;
                    assi.deserializeFromHexString(hexStr);
                    m_midiController->setAssignment(
                        static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i), assi);
                }
            }
        }

        if (auto* portXml = extCtrlState->getChildByName(
                UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::OSCINPUTPORT)))
            m_oscController->openPort(portXml->getAllSubText().getIntValue());

        static const UmsciAppConfiguration::TagID oscParamTagIds[] = {
            UmsciAppConfiguration::TagID::OSC_UPMIXROT,
            UmsciAppConfiguration::TagID::OSC_UPMIXSCALE,
            UmsciAppConfiguration::TagID::OSC_UPMIXHEIGHTSCALE,
            UmsciAppConfiguration::TagID::OSC_UPMIXANGLESTRETCH,
            UmsciAppConfiguration::TagID::OSC_UPMIXOFFSETX,
            UmsciAppConfiguration::TagID::OSC_UPMIXOFFSETY
        };

        for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        {
            if (auto* addrXml = extCtrlState->getChildByName(
                    UmsciAppConfiguration::getTagName(oscParamTagIds[i])))
            {
                auto addr = addrXml->getAllSubText();
                if (addr.isNotEmpty())
                    m_oscController->setAddress(
                        static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i), addr);
            }
        }
    }
}

bool MainComponent::isFullscreenEnabled()
{
#if JUCE_WINDOWS
    return juce::Desktop::getInstance().getKioskModeComponent() != nullptr;
#elif JUCE_MAC
    if (auto* topLevel = getTopLevelComponent())
        if (auto* peer = topLevel->getPeer())
            return peer->isFullScreen();

    jassertfalse;
    return false;
#endif
}

void MainComponent::showExternalControlSettings()
{
    m_messageBox = std::make_unique<juce::AlertWindow>(
        "External control...",
        "Configure MIDI and OSC assignments for upmix transform parameters.",
        juce::MessageBoxIconType::NoIcon);

    m_externalControlComponent = std::make_unique<UmsciExternalControlComponent>();
    m_externalControlComponent->setMidiInputDeviceIdentifier(m_midiController->getDeviceIdentifier());
    for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        m_externalControlComponent->setMidiAssi(
            static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i),
            m_midiController->getAssignment(static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i)));
    m_externalControlComponent->setOscInputPort(m_oscController->getPort());
    for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
        m_externalControlComponent->setOscAddr(
            static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i),
            m_oscController->getAddress(static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i)));
    m_messageBox->addCustomComponent(m_externalControlComponent.get());

    m_messageBox->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    m_messageBox->addButton("Ok",     1, juce::KeyPress(juce::KeyPress::returnKey));
    m_messageBox->enterModalState(true, juce::ModalCallbackFunction::create([=](int returnValue) {
        if (returnValue == 1)
        {
            m_midiController->openDevice(m_externalControlComponent->getMidiInputDeviceIdentifier());
            for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
                m_midiController->setAssignment(
                    static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i),
                    m_externalControlComponent->getMidiAssi(
                        static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i)));
            m_oscController->openPort(m_externalControlComponent->getOscInputPort());
            for (int i = 0; i < UmsciExternalControlComponent::UpmixMidiParam_COUNT; ++i)
                m_oscController->setAddress(
                    static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i),
                    m_externalControlComponent->getOscAddr(
                        static_cast<UmsciExternalControlComponent::UpmixMidiParam>(i)));
            if (m_config)
                m_config->triggerConfigurationDump();
        }
        m_externalControlComponent.reset();
        m_messageBox.reset();
    }));
}

void MainComponent::applyUpmixParamValue(UmsciExternalControlComponent::UpmixMidiParam param,
                                         float domainValue)
{
    if (!m_controlComponent)
        return;

    switch (param)
    {
    case UmsciExternalControlComponent::UpmixMidiParam_Rotation:
        m_controlComponent->setUpmixTransform(domainValue,
                                              m_controlComponent->getUpmixTrans(),
                                              m_controlComponent->getUpmixHeightTrans(),
                                              m_controlComponent->getUpmixAngleStretch());
        break;
    case UmsciExternalControlComponent::UpmixMidiParam_Translation:
        m_controlComponent->setUpmixTransform(m_controlComponent->getUpmixRot(),
                                              domainValue,
                                              m_controlComponent->getUpmixHeightTrans(),
                                              m_controlComponent->getUpmixAngleStretch());
        break;
    case UmsciExternalControlComponent::UpmixMidiParam_HeightTranslation:
        m_controlComponent->setUpmixTransform(m_controlComponent->getUpmixRot(),
                                              m_controlComponent->getUpmixTrans(),
                                              domainValue,
                                              m_controlComponent->getUpmixAngleStretch());
        break;
    case UmsciExternalControlComponent::UpmixMidiParam_AngleStretch:
        m_controlComponent->setUpmixTransform(m_controlComponent->getUpmixRot(),
                                              m_controlComponent->getUpmixTrans(),
                                              m_controlComponent->getUpmixHeightTrans(),
                                              domainValue);
        break;
    case UmsciExternalControlComponent::UpmixMidiParam_OffsetX:
        m_controlComponent->setUpmixOffset(domainValue, m_controlComponent->getUpmixOffsetY());
        break;
    case UmsciExternalControlComponent::UpmixMidiParam_OffsetY:
        m_controlComponent->setUpmixOffset(m_controlComponent->getUpmixOffsetX(), domainValue);
        break;
    default:
        break;
    }

    // Fire live-mode position updates and trigger config persistence via the
    // callback chain: notifyTransformChanged → onTransformChanged → onUpmixTransformChanged
    // → triggerConfigurationDump.  This mirrors what the interactive drag handlers do.
    m_controlComponent->triggerUpmixTransformApplied();
}


void MainComponent::showDbprProjectLoad()
{
    m_fileChooser = std::make_unique<juce::FileChooser>(
        "Open d\u0026b dbpr project file...",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.dbpr");

    m_fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto results = chooser.getResults();
            if (!results.isEmpty() && m_dbprController)
                m_dbprController->loadProjectFromFile(results.getFirst().getFullPathName().toStdString());

            m_fileChooser.reset();
        });
}

void MainComponent::checkDbprDeviceSync()
{
    if (!m_dbprController || !m_dbprProjectComponent || !m_controlComponent)
        return;
    if (!m_dbprController->hasProjectLoaded())
    {
        m_dbprProjectComponent->setMismatchFlashing(false);
        return;
    }

    const auto& projectData      = m_dbprController->getProjectData();
    const auto& deviceNames      = m_controlComponent->getSourceNames();
    const auto& devicePositions  = m_controlComponent->getSpeakerPositions();
    const auto& deviceFgData     = m_controlComponent->getFunctionGroupData();

    bool mismatch = false;

    // ── MatrixInput / Soundobject names ──────────────────────────────────────
    // Empty names on both sides are skipped; only one-sided or differing
    // non-empty names count as a mismatch.
    for (auto const& kv : projectData.matrixInputData)
    {
        const auto& projectName = kv.second.name;
        auto it = deviceNames.find(static_cast<std::int16_t>(kv.first));
        const auto deviceName = (it != deviceNames.end()) ? it->second : std::string{};

        if (!projectName.empty() || !deviceName.empty())
            if (projectName != deviceName) { mismatch = true; break; }
    }

    // ── Loudspeaker count and positions ──────────────────────────────────────
    // A speaker is considered active only when at least one of its 6DOF values
    // is non-zero; all-zero means not present.
    // Device array layout from ToAimingAndPosition(): {hor, vrt, rot, x, y, z}
    if (!mismatch)
    {
        constexpr float eps = 1e-4f;

        auto isDevicePositionNull = [eps](const std::array<std::float_t, 6>& dp) {
            return std::abs(dp[0]) <= eps && std::abs(dp[1]) <= eps && std::abs(dp[2]) <= eps &&
                   std::abs(dp[3]) <= eps && std::abs(dp[4]) <= eps && std::abs(dp[5]) <= eps;
        };

        int projectActiveCount = 0;
        for (auto const& kv : projectData.speakerPositionData)
            if (!kv.second.isNull()) ++projectActiveCount;
        int deviceActiveCount = 0;
        for (auto const& kv : devicePositions)
            if (!isDevicePositionNull(kv.second)) ++deviceActiveCount;

        if (projectActiveCount != deviceActiveCount)
        {
            mismatch = true;
        }
        else
        {
            for (auto const& kv : projectData.speakerPositionData)
            {
                if (kv.second.isNull())
                    continue;

                auto it = devicePositions.find(static_cast<std::int16_t>(kv.first));
                if (it == devicePositions.end() || isDevicePositionNull(it->second))
                    { mismatch = true; break; }

                const auto& dp = it->second;
                const auto& pp = kv.second;
                if (std::abs(dp[0] - static_cast<float>(pp.hor)) > eps ||
                    std::abs(dp[1] - static_cast<float>(pp.vrt)) > eps ||
                    std::abs(dp[2] - static_cast<float>(pp.rot)) > eps ||
                    std::abs(dp[3] - static_cast<float>(pp.x))   > eps ||
                    std::abs(dp[4] - static_cast<float>(pp.y))   > eps ||
                    std::abs(dp[5] - static_cast<float>(pp.z))   > eps)
                {
                    mismatch = true; break;
                }
            }
        }
    }

    // ── Function-group count, names, and mode values ─────────────────────────
    // Mode 0 (None) is equivalent to the FG not being present; only active
    // (mode != 0) groups are counted and compared.
    if (!mismatch)
    {
        int projectActiveCount = 0;
        for (auto const& kv : projectData.functionGroupData)
            if (kv.second.mode != 0) ++projectActiveCount;
        int deviceActiveCount = 0;
        for (auto const& kv : deviceFgData)
            if (kv.second.mode != 0) ++deviceActiveCount;

        if (projectActiveCount != deviceActiveCount)
        {
            mismatch = true;
        }
        else
        {
            for (auto const& kv : projectData.functionGroupData)
            {
                if (kv.second.mode == 0)
                    continue;

                auto it = deviceFgData.find(static_cast<std::int16_t>(kv.first));
                const auto deviceMode = (it != deviceFgData.end()) ? it->second.mode : std::uint16_t(0);

                if (deviceMode != static_cast<std::uint16_t>(kv.second.mode))
                {
                    mismatch = true; break;
                }

                // Empty names count as not present; only compare when both sides
                // have a non-empty name.
                if (!kv.second.name.empty() && !it->second.name.empty()
                    && it->second.name != kv.second.name)
                {
                    mismatch = true; break;
                }
            }
        }
    }

    m_dbprProjectComponent->setMismatchFlashing(mismatch);
}

void MainComponent::setDbprPanelState(UmsciDbprProjectComponent::PanelState newState)
{
    if (!m_dbprProjectComponent)
        return;

    m_dbprProjectComponent->setPanelState(newState);

    const auto safety = JUCEAppBasics::iOS_utils::getDeviceSafetyMargins();
    auto safeBounds = getLocalBounds();
    safeBounds.removeFromTop(safety._top);
    safeBounds.removeFromBottom(safety._bottom);
    safeBounds.removeFromLeft(safety._left);
    safeBounds.removeFromRight(safety._right);

    const auto panelW    = UmsciDbprProjectComponent::s_panelWidth;
    const auto panelH    = UmsciDbprProjectComponent::s_panelHeight;
    const auto panelTopY = safeBounds.getBottom() - panelH - UmsciDbprProjectComponent::s_panelMargin;
    const auto x         = (newState == UmsciDbprProjectComponent::PanelState::Tucked)
        ? -(panelW - UmsciDbprProjectComponent::s_grabStripWidth)
        : UmsciDbprProjectComponent::s_panelMargin;

    juce::Desktop::getInstance().getAnimator().animateComponent(
        m_dbprProjectComponent.get(),
        juce::Rectangle<int>(x, panelTopY, panelW, panelH),
        1.0f, 220, false, 1.0, 1.0);
}

void MainComponent::setSnapshotPanelState(UmsciSnapshotComponent::PanelState newState)
{
    if (!m_snapshotComponent)
        return;

    m_snapshotComponent->setPanelState(newState);

    const auto safety = JUCEAppBasics::iOS_utils::getDeviceSafetyMargins();
    auto safeBounds = getLocalBounds();
    safeBounds.removeFromTop(safety._top);
    safeBounds.removeFromBottom(safety._bottom);
    safeBounds.removeFromLeft(safety._left);
    safeBounds.removeFromRight(safety._right);

    const auto dbprPanelH  = UmsciDbprProjectComponent::s_panelHeight;
    const auto dbprTopY    = safeBounds.getBottom() - dbprPanelH - UmsciDbprProjectComponent::s_panelMargin;
    const auto snapW       = UmsciSnapshotComponent::s_panelWidth;
    const auto snapH       = UmsciSnapshotComponent::s_panelHeight;
    const auto snapTopY    = dbprTopY - UmsciSnapshotComponent::s_panelMargin - snapH;
    const auto x           = (newState == UmsciSnapshotComponent::PanelState::Tucked)
        ? -(snapW - UmsciSnapshotComponent::s_grabStripWidth)
        : UmsciSnapshotComponent::s_panelMargin;

    juce::Desktop::getInstance().getAnimator().animateComponent(
        m_snapshotComponent.get(),
        juce::Rectangle<int>(x, snapTopY, snapW, snapH),
        1.0f, 220, false, 1.0, 1.0);
}
