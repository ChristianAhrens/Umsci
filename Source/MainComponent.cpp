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

#include "CustomPopupMenuComponent.h"
#include "UmsciComponent.h"
#include "UmsciDiscoverComponent.h"
#include "UmsciConnectingComponent.h"

#include "AboutComponent.h"

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

    m_ocp1Connection = std::make_unique<InterprocessConnectionImpl>();
    m_ocp1Connection->onConnectionMade = [=]() {
        DBG(__FUNCTION__);
    
    //    std::vector<Mema::SerializableMessage::SerializableMessageType> desiredTrafficTypes = {
    //        Mema::SerializableMessage::EnvironmentParameters, 
    //        Mema::SerializableMessage::ReinitIOCount, 
    //        Mema::SerializableMessage::ControlParameters,
    //        Mema::SerializableMessage::PluginParameterInfos,
    //        Mema::SerializableMessage::PluginParameterValue };
    //    m_networkConnection->sendMessage(std::make_unique<Mema::DataTrafficTypeSelectionMessage>(desiredTrafficTypes)->getSerializedMessage());
    
        setStatus(Status::Running);
    };
    m_ocp1Connection->onConnectionLost = [=]() {
        DBG(__FUNCTION__);
    
    //    if (m_remoteComponent)
    //        m_remoteComponent->resetCtrl();
        
        connectToMema();
    
        setStatus(Status::Connecting);
    };
    m_ocp1Connection->onMessageReceived = [=](const juce::MemoryBlock& message) {
    //    auto knownMessage = Mema::SerializableMessage::initFromMemoryBlock(message);
    //    if (auto const epm = dynamic_cast<const Mema::EnvironmentParametersMessage*>(knownMessage))
    //    {
    //        m_settingsHostLookAndFeelId = epm->getPaletteStyle();
    //        jassert(m_settingsHostLookAndFeelId >= JUCEAppBasics::CustomLookAndFeel::PS_Dark && m_settingsHostLookAndFeelId <= JUCEAppBasics::CustomLookAndFeel::PS_Light);
    //
    //        if (onPaletteStyleChange && !m_settingsItems[2].second && !m_settingsItems[3].second) // callback must be set and neither 2 nor 3 setting set (manual dark or light)
    //        {
    //            m_settingsItems[1].second = 1; // set ticked for setting 1 (follow host)
    //            onPaletteStyleChange(m_settingsHostLookAndFeelId, false/*do not follow local style any more if a message was received via net once*/);
    //        }
    //    }
    //    else if (m_remoteComponent && nullptr != knownMessage && Status::Running == m_currentStatus)
    //    {
    //        m_remoteComponent->handleMessage(*knownMessage);
    //    }
    //    Mema::SerializableMessage::freeMessageData(knownMessage);
    };

    //m_remoteComponent = std::make_unique<UmsciComponent>();
    //m_remoteComponent->onExitClick = [=]() {
    //    setStatus(Status::Discovering);
    //};
    //m_remoteComponent->onMessageReadyToSend = [=](const juce::MemoryBlock& message) {
    //    if (m_networkConnection)
    //        m_networkConnection->sendMessage(message);
    //};
    //addAndMakeVisible(m_remoteComponent.get());

    //m_discoverComponent = std::make_unique<MemaClientDiscoverComponent>();
    //m_discoverComponent->setupServiceDiscovery(Mema::ServiceData::getServiceTypeUIDBase(), Mema::ServiceData::getRemoteServiceTypeUID());
    //m_discoverComponent->onServiceSelected = [=](const JUCEAppBasics::SessionMasterAwareService& selectedService) {
    //    m_selectedService = selectedService;
    //
    //    connectToMema();
    //
    //    if (m_config)
    //        m_config->triggerConfigurationDump(false);
    //};
    //addAndMakeVisible(m_discoverComponent.get());

    m_connectingComponent = std::make_unique<UmsciConnectingComponent>();
    addAndMakeVisible(m_connectingComponent.get());

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
    m_settingsItems[UmsciSettingsOption::LookAndFeel_FollowHost] = std::make_pair("Follow Mema", 0);
    m_settingsItems[UmsciSettingsOption::LookAndFeel_Dark] = std::make_pair("Dark", 1);
    m_settingsItems[UmsciSettingsOption::LookAndFeel_Light] = std::make_pair("Light", 0);
    // default output visu is normal meterbridge
    m_settingsItems[UmsciSettingsOption::ControlFormat_RawChannels] = std::make_pair("Faderbank", 1);
    m_settingsItems[UmsciSettingsOption::ControlFormat_PluginParameterControl] = std::make_pair("Plug-in parameter control", 1);
    // default panning colour is green
    m_settingsItems[UmsciSettingsOption::ControlColour_Green] = std::make_pair("Green", 1);
    m_settingsItems[UmsciSettingsOption::ControlColour_Red] = std::make_pair("Red", 0);
    m_settingsItems[UmsciSettingsOption::ControlColour_Blue] = std::make_pair("Blue", 0);
    m_settingsItems[UmsciSettingsOption::ControlColour_Pink] = std::make_pair("Anni Pink", 0);
    m_settingsItems[UmsciSettingsOption::ControlColour_Laser] = std::make_pair("Laser", 0);
#if JUCE_WINDOWS || JUCE_MAC
    // fullscreen toggling
    m_settingsItems[UmsciSettingsOption::FullscreenWindowMode] = std::make_pair("Toggle fullscreen mode" + fullscreenShortCutHint, 0);
#endif
    // Further components
    m_settingsButton = std::make_unique<juce::DrawableButton>("Settings", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_settingsButton->setTooltip(juce::String("Settings for") + juce::JUCEApplication::getInstance()->getApplicationName());
    m_settingsButton->onClick = [this] {
        juce::PopupMenu lookAndFeelSubMenu;
        for (int i = UmsciSettingsOption::LookAndFeel_First; i <= UmsciSettingsOption::LookAndFeel_Last; i++)
            lookAndFeelSubMenu.addItem(i, m_settingsItems[i].first, true, m_settingsItems[i].second == 1);

        juce::PopupMenu controlFormatSubMenu;
        for (int i = UmsciSettingsOption::ControlFormat_First; i <= UmsciSettingsOption::ControlFormat_Last; i++)
            controlFormatSubMenu.addItem(i, m_settingsItems[i].first, true, m_settingsItems[i].second == 1);

        juce::PopupMenu controlColourSubMenu;
        for (int i = UmsciSettingsOption::ControlColour_First; i <= UmsciSettingsOption::ControlColour_Last; i++)
            controlColourSubMenu.addItem(i, m_settingsItems[i].first, true, m_settingsItems[i].second == 1);

        juce::PopupMenu settingsMenu;
        settingsMenu.addSubMenu("LookAndFeel", lookAndFeelSubMenu);
        settingsMenu.addSubMenu("Control format", controlFormatSubMenu);
        settingsMenu.addSubMenu("Control colour", controlColourSubMenu);
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

    m_disconnectButton = std::make_unique<juce::DrawableButton>("Disconnect", juce::DrawableButton::ButtonStyle::ImageFitted);
    //m_disconnectButton->setTooltip(juce::String("Disconnect ") + juce::JUCEApplication::getInstance()->getApplicationName() + " from " + (m_selectedService.description.isNotEmpty() ? m_selectedService.description : "Nothing :)"));
    m_disconnectButton->onClick = [this] {
        if (m_ocp1Connection)
            m_ocp1Connection->disconnect();

        //if (m_remoteComponent)
        //    m_remoteComponent->resetCtrl();
        //
        //m_selectedService = {};
        //if (m_discoverComponent)
        //    m_discoverComponent->resetServices();

        if (m_config)
            m_config->triggerConfigurationDump();

        setStatus(Status::Discovering);
    };
    m_disconnectButton->setAlwaysOnTop(true);
    m_disconnectButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_disconnectButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(m_disconnectButton.get());

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
    auto updater = JUCEAppBasics::WebUpdateDetector::getInstance();
    updater->SetReferenceVersion(ProjectInfo::versionString);
    updater->SetDownloadUpdateWebAddress("https://github.com/christianahrens/mema/releases/latest");
    updater->CheckForNewVersion(true, "https://raw.githubusercontent.com/ChristianAhrens/Mema/refs/heads/main/");
#endif


    // add this main component to watchers
    m_config->addWatcher(this); // without initial update - that we have to do externally after lambdas were assigned

    // we want keyboard focus for fullscreen toggle shortcut
    setWantsKeyboardFocus(true);
}

MainComponent::~MainComponent()
{
}

void MainComponent::resized()
{
    auto safety = JUCEAppBasics::iOS_utils::getDeviceSafetyMargins();
    auto safeBounds = getLocalBounds();
    safeBounds.removeFromTop(safety._top);
    safeBounds.removeFromBottom(safety._bottom);
    safeBounds.removeFromLeft(safety._left);
    safeBounds.removeFromRight(safety._right);
    
    switch (m_currentStatus)
    {
        case Status::Running:
            m_connectingComponent->setVisible(false);
            m_discoverComponent->setVisible(false);
            m_remoteComponent->setVisible(true);
            m_remoteComponent->setBounds(safeBounds);
            break;
        case Status::Connecting:
            m_remoteComponent->setVisible(false);
            m_discoverComponent->setVisible(false);
            m_connectingComponent->setVisible(true);
            m_connectingComponent->setBounds(safeBounds);
            break;
        case Status::Discovering:
        default:
            m_connectingComponent->setVisible(false);
            m_remoteComponent->setVisible(false);
            m_discoverComponent->setVisible(true);
            m_discoverComponent->setBounds(safeBounds);
            break;
    }

    auto leftButtons = safeBounds.removeFromLeft(36);
    auto rightButtons = safeBounds.removeFromLeft(36);
    m_aboutButton->setBounds(leftButtons.removeFromTop(35).removeFromBottom(30));
    m_settingsButton->setBounds(leftButtons.removeFromTop(35).removeFromBottom(30));
    m_disconnectButton->setBounds(rightButtons.removeFromTop(35).removeFromBottom(30));
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

    auto disconnectDrawable = juce::Drawable::createFromSVG(*juce::XmlDocument::parse(BinaryData::link_off_24dp_svg).get());
    disconnectDrawable->replaceColour(juce::Colours::black, getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId));
    m_disconnectButton->setImages(disconnectDrawable.get());

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
    else if (UmsciSettingsOption::ControlFormat_First <= selectedId && UmsciSettingsOption::ControlFormat_Last >= selectedId)
        handleSettingsControlFormatMenuResult(selectedId);
    else if (UmsciSettingsOption::ControlColour_First <= selectedId && UmsciSettingsOption::ControlColour_Last >= selectedId)
        handleSettingsControlColourMenuResult(selectedId);
    //else if (UmsciSettingsOption::ExternalControl == selectedId)
    //    showExternalControlSettings();
    else if (UmsciSettingsOption::FullscreenWindowMode == selectedId)
        handleSettingsFullscreenModeToggleResult();
    else
        jassertfalse; // unhandled menu entry!?
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

void MainComponent::handleSettingsControlFormatMenuResult(int selectedId)
{
    // helper internal function to avoid code clones
    std::function<void(int, int, int, int, int, int, int, int, int, int, int, int)> setSettingsItemsCheckState = [=](int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l) {
        m_settingsItems[UmsciSettingsOption::ControlFormat_RawChannels].second = a;
        m_settingsItems[UmsciSettingsOption::ControlFormat_PluginParameterControl].second = l;
    };

    switch (selectedId)
    {
    //case UmsciSettingsOption::ControlFormat_RawChannels:
    //    setSettingsItemsCheckState(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setFaderbankCtrlActive();
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_LRS:
    //    setSettingsItemsCheckState(0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::createLRS());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_LCRS:
    //    setSettingsItemsCheckState(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::createLCRS());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_5point0:
    //    setSettingsItemsCheckState(0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::create5point0());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_5point1:
    //    setSettingsItemsCheckState(0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::create5point1());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_5point1point2:
    //    setSettingsItemsCheckState(0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::create5point1point2());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_7point0:
    //    setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::create7point0());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_7point1:
    //    setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::create7point1());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_7point1point4:
    //    setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::create7point1point4());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_9point1point6:
    //    setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::create9point1point6());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PanningType_Quadrophonic:
    //    setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setOutputPanningCtrlActive(juce::AudioChannelSet::quadraphonic());
    //    break;
    //case UmsciSettingsOption::ControlFormat_PluginParameterControl:
    //    setSettingsItemsCheckState(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
    //    if (m_remoteComponent)
    //        m_remoteComponent->setPluginCtrlActive();
    //    break;
    default:
        jassertfalse; // unknown id fed in unintentionally ?!
        break;
    }

    resized();
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

void MainComponent::showExternalControlSettings()
{
    m_messageBox = std::make_unique<juce::AlertWindow>(
        "External control setup",
        "Enter remote control parameters to externally connect to " + juce::JUCEApplication::getInstance()->getApplicationName() + " and control its parameters.\n" + 
        "Info: This machine uses IP " + juce::IPAddress::getLocalAddress().toString(),
        juce::MessageBoxIconType::NoIcon);

    m_messageBox->addTextBlock("\nADM-OSC connection parameters:");
    //if (m_remoteComponent)
    //{
    //    auto admOscSettings = m_remoteComponent->getExternalAdmOscSettings();
    //    m_messageBox->addTextEditor("ADM local port", juce::String(std::get<0>(admOscSettings)), "ADM-OSC port");
    //    m_messageBox->addTextEditor("ADM remote IP", std::get<1>(admOscSettings).toString(), "Target IP");
    //    m_messageBox->addTextEditor("ADM remote port", juce::String(std::get<2>(admOscSettings)), "Target port");
    //}

    //m_messageBox->addTextBlock("\nOCP.1 connection parameters:");
    //if (m_remoteComponent)
    //{
    //    m_messageBox->addTextEditor("OCP.1 local port", juce::String(50014), "OCP.1 port");
    //}

    m_messageBox->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    m_messageBox->addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey));
    m_messageBox->enterModalState(true, juce::ModalCallbackFunction::create([=](int returnValue) {
        if (returnValue == 1)
        {
            auto ADMOSCport = m_messageBox->getTextEditorContents("ADM local port").getIntValue();
            auto ADMOSCremoteIP = juce::IPAddress(m_messageBox->getTextEditorContents("ADM remote IP"));
            auto ADMOSCremotePort = m_messageBox->getTextEditorContents("ADM remote port").getIntValue();
            if (m_remoteComponent)
            {
                //m_remoteComponent->setExternalAdmOscSettings(ADMOSCport, ADMOSCremoteIP, ADMOSCremotePort);

                if (m_config)
                    m_config->triggerConfigurationDump();
            }
        }

        m_messageBox.reset();
    }));
}

void MainComponent::setControlColour(const juce::Colour& controlColour)
{
    m_controlColour = controlColour;

    applyControlColour();
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
}

void MainComponent::setStatus(const Status& s)
{
    m_currentStatus = s;
    resized();
}

const MainComponent::Status MainComponent::getStatus()
{
    return m_currentStatus;
}

void MainComponent::connectToMema()
{
    //if (m_connectingComponent)
    //    m_connectingComponent->setMasterServiceDescription(m_selectedService.description);
    //if (m_discoverComponent)
    //    m_discoverComponent->setMasterServiceDescription(m_selectedService.description);

    setStatus(Status::Connecting);

    timerCallback(); // avoid codeclones by manually trigger the timed connection attempt once

    // restart connection attempt after 5s, in case something got stuck...
    startTimer(5000);
}

void MainComponent::timerCallback()
{
    if (Status::Connecting == getStatus())
    {
        //auto sl = m_discoverComponent->getAvailableServices();
        //auto const& iter = std::find_if(sl.begin(), sl.end(), [=](const auto& service) { return service.description == m_selectedService.description; });
        //if (iter != sl.end())
        {
            //if ((m_selectedService.address != iter->address && m_selectedService.port != iter->port && m_selectedService.description != iter->description) || !m_networkConnection->isConnected())
            //{
            //    m_selectedService = *iter;
            //    if (m_networkConnection)
            //        m_networkConnection->ConnectToSocket(m_selectedService.address.toString(), m_selectedService.port);
            //}
            //else if (m_networkConnection && !m_networkConnection->isConnected())
            //    m_networkConnection->RetryConnectToSocket();
        }
    }
    else
        stopTimer();
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
        // connection config
        auto connectionConfigXmlElement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONNECTIONCONFIG));

        auto serviceDescriptionXmlElmement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::SERVICEDESCRIPTION));
        serviceDescriptionXmlElmement->addTextElement(/*m_selectedService.description*/"");
        connectionConfigXmlElement->addChildElement(serviceDescriptionXmlElmement.release());

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

        auto controlFormatXmlElmement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLFORMAT));
        for (int i = UmsciSettingsOption::ControlFormat_First; i <= UmsciSettingsOption::ControlFormat_Last; i++)
        {
            if (m_settingsItems[i].second == 1)
                controlFormatXmlElmement->addTextElement(juce::String(i));
        }
        visuConfigXmlElement->addChildElement(controlFormatXmlElmement.release());
        
        auto panningColourXmlElmement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLCOLOUR));
        for (int i = UmsciSettingsOption::ControlColour_First; i <= UmsciSettingsOption::ControlColour_Last; i++)
        {
            if (m_settingsItems[i].second == 1)
                panningColourXmlElmement->addTextElement(juce::String(i));
        }
        visuConfigXmlElement->addChildElement(panningColourXmlElmement.release());

        m_config->setConfigState(std::move(visuConfigXmlElement), UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::VISUCONFIG));

        //// external control config
        //auto extCtrlConfigXmlElement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::EXTCTRLCONFIG));
        //
        //auto admOscHostXmlElmement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::ADMOSCHOST));
        //if (m_remoteComponent)
        //    admOscHostXmlElmement->setAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::PORT), std::get<0>(m_remoteComponent->getExternalAdmOscSettings()));
        //extCtrlConfigXmlElement->addChildElement(admOscHostXmlElmement.release());
        //
        //auto admOscClientXmlElmement = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::ADMOSCCLIENT));
        //if (m_remoteComponent)
        //{
        //    admOscClientXmlElmement->setAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::IP), std::get<1>(m_remoteComponent->getExternalAdmOscSettings()).toString());
        //    admOscClientXmlElmement->setAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::PORT), std::get<2>(m_remoteComponent->getExternalAdmOscSettings()));
        //}
        //extCtrlConfigXmlElement->addChildElement(admOscClientXmlElmement.release());
        //
        //m_config->setConfigState(std::move(extCtrlConfigXmlElement), UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::EXTCTRLCONFIG));
    }
}

void MainComponent::onConfigUpdated()
{
    auto connectionConfigState = m_config->getConfigState(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONNECTIONCONFIG));
    if (connectionConfigState)
    {
        auto serviceDescriptionXmlElement = connectionConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::SERVICEDESCRIPTION));
        if (serviceDescriptionXmlElement)
        {
            auto serviceDescription = serviceDescriptionXmlElement->getAllSubText();
            //if (serviceDescription.isNotEmpty() && m_selectedService.description != serviceDescription)
            //{
            //    if (m_networkConnection)
            //        m_networkConnection->disconnect();
            //
            //    m_selectedService = {};
            //    m_selectedService.description = serviceDescription;
            //
            //    connectToDevice();
            //}
        }
    }

    auto visuConfigState = m_config->getConfigState(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::VISUCONFIG));
    if (visuConfigState)
    {
        auto lookAndFeelXmlElement = visuConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::LOOKANDFEEL));
        if (lookAndFeelXmlElement)
        {
            auto lookAndFeelSettingsOptionId = lookAndFeelXmlElement->getAllSubText().getIntValue();
            handleSettingsLookAndFeelMenuResult(lookAndFeelSettingsOptionId);
        }

        auto controlFormatXmlElement = visuConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLFORMAT));
        if (controlFormatXmlElement)
        {
            auto controlFormatSettingsOptionId = controlFormatXmlElement->getAllSubText().getIntValue();
            handleSettingsControlFormatMenuResult(controlFormatSettingsOptionId);
        }

        auto controlColourXmlElement = visuConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLCOLOUR));
        if (controlColourXmlElement)
        {
            auto controlColourSettingsOptionId = controlColourXmlElement->getAllSubText().getIntValue();
            handleSettingsControlColourMenuResult(controlColourSettingsOptionId);
        }
    }

    //auto externalControlConfigState = m_config->getConfigState(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::EXTCTRLCONFIG));
    //if (externalControlConfigState)
    //{
    //    int ADMOSCport = 0;
    //    juce::IPAddress ADMOSCremoteIP = juce::IPAddress::local();
    //    int ADMOSCremotePort = 0;
    //
    //    auto admOscHostXmlElmement = externalControlConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::ADMOSCHOST));
    //    if (admOscHostXmlElmement)
    //        ADMOSCport = admOscHostXmlElmement->getIntAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::PORT));
    //
    //    auto admOscClientXmlElmement = externalControlConfigState->getChildByName(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::ADMOSCCLIENT));
    //    if (admOscClientXmlElmement)
    //    {
    //        ADMOSCremoteIP = juce::IPAddress(admOscClientXmlElmement->getStringAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::IP)));
    //        ADMOSCremotePort = admOscClientXmlElmement->getIntAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::PORT));
    //    }
    //
    //    if (m_remoteComponent)
    //        m_remoteComponent->setExternalAdmOscSettings(ADMOSCport, ADMOSCremoteIP, ADMOSCremotePort);
    //}
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

