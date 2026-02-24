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

#include "UmsciControlComponent.h"


UmsciControlComponent::UmsciControlComponent()
    : juce::Component()
{
    setOcp1IOSize({ 64, 64 });

    jassert(!DeviceController::getInstance()->onRemoteObjectReceived); // this lambda can only be used once, and we expect that to be here
    DeviceController::getInstance()->onRemoteObjectReceived = [=](const DeviceController::RemoteObject& obj) {
        setRemoteObject(obj);
        return true;
    };
}

UmsciControlComponent::~UmsciControlComponent()
{
}

void UmsciControlComponent::paint(Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));

    g.setColour(getLookAndFeel().findColour(juce::TextEditor::ColourIds::textColourId));
    g.drawFittedText("Controlling now - what to do next?", getLocalBounds().reduced(35), juce::Justification::centred, 2);
}

void UmsciControlComponent::resized()
{
    //if (m_faderbankCtrlComponent && m_faderbankCtrlComponent->isVisible())
    //    m_faderbankCtrlComponent->setBounds(getLocalBounds());
    //if (m_panningCtrlComponent && m_panningCtrlComponent->isVisible())
    //    m_panningCtrlComponent->setBounds(getLocalBounds());
    //if (m_pluginCtrlComponent && m_pluginCtrlComponent->isVisible())
    //    m_pluginCtrlComponent->setBounds(getLocalBounds());
}

std::unique_ptr<XmlElement> UmsciControlComponent::createStateXml()
{
    auto controlConfigStateXml = std::make_unique<juce::XmlElement>(UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLCONFIG));

    controlConfigStateXml->setAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::IOSIZE), juce::String(getOcp1IOSize().first) + "x" + juce::String(getOcp1IOSize().second));

    return controlConfigStateXml;
}

bool UmsciControlComponent::setStateXml(XmlElement* stateXml)
{
    if (!stateXml || (stateXml->getTagName() != UmsciAppConfiguration::getTagName(UmsciAppConfiguration::TagID::CONTROLCONFIG)))
        return false;

    auto ocp1IOSize = stateXml->getStringAttribute(UmsciAppConfiguration::getAttributeName(UmsciAppConfiguration::AttributeID::IOSIZE));
    auto newIoSize = std::make_pair(ocp1IOSize.upToFirstOccurrenceOf("x", false, true).getIntValue(), ocp1IOSize.fromLastOccurrenceOf("x", false, true).getIntValue());
    if (getOcp1IOSize() != newIoSize)
        setOcp1IOSize(newIoSize);

    return true;
}

const std::pair<int, int>& UmsciControlComponent::getOcp1IOSize()
{
    return m_ocp1IOSize;
}

void UmsciControlComponent::setOcp1IOSize(const std::pair<int, int>& ioSize)
{
    m_ocp1IOSize = ioSize;

    // todo react to changes
    rebuildOcp1ObjectTree();
}

void UmsciControlComponent::rebuildOcp1ObjectTree()
{
    auto ocp1ObjectTree = std::vector<DeviceController::RemoteObject>();

    ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Settings_DeviceName, DeviceController::RemObjAddr(), NanoOcp1::Variant()));
    for (std::int16_t i = 1; i <= m_ocp1IOSize.first; i++)
    {
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixInput_Mute, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixInput_Gain, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Positioning_SourceSpread, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Positioning_SourceDelayMode, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Positioning_SourcePosition, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
    }
    for (std::int16_t o = 1; o <= m_ocp1IOSize.second; o++)
    {
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixOutput_Mute, DeviceController::RemObjAddr(o, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixOutput_Gain, DeviceController::RemObjAddr(o, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Positioning_SpeakerPosition, DeviceController::RemObjAddr(o, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
    }

    DeviceController::getInstance()->SetActiveRemoteObjects(ocp1ObjectTree);
}

void UmsciControlComponent::setRemoteObject(const DeviceController::RemoteObject& obj)
{
    DBG(juce::String(__FUNCTION__) << " " << DeviceController::RemoteObject::GetObjectDescription(obj.Id) << " " << obj.Addr.toNiceString());
}

