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

#include "UmsciLoudspeakersPaintComponent.h"
#include "UmsciSoundobjectsPaintComponent.h"
#include "UmsciUpmixIndicatorPaintNControlComponent.h"


UmsciControlComponent::UmsciControlComponent()
    : juce::Component()
{
    setOcp1IOSize({ 64, 64 });

    jassert(!DeviceController::getInstance()->onRemoteObjectReceived); // this lambda can only be used once, and we expect that to be here
    DeviceController::getInstance()->onRemoteObjectReceived = [=](const DeviceController::RemoteObject& obj) {
        setRemoteObject(obj);
        return true;
    };

    m_loudspeakersInAreaPaintComponent = std::make_unique<UmsciLoudspeakersPaintComponent>();
    addAndMakeVisible(m_loudspeakersInAreaPaintComponent.get());
    m_soundobjectsInAreaPaintComponent = std::make_unique<UmsciSoundobjectsPaintComponent>();
    addAndMakeVisible(m_soundobjectsInAreaPaintComponent.get());
    m_soundobjectsInAreaPaintComponent->onSourcePositionChanged = [this](std::int16_t sourceId, std::array<std::float_t, 3> position) {
        m_sourcePosition[sourceId] = position;
        DeviceController::getInstance()->SetObjectValue(
            DeviceController::RemoteObject(
                DeviceController::RemoteObject::Positioning_SourcePosition,
                DeviceController::RemObjAddr(sourceId, DeviceController::RemObjAddr::sc_INV),
                NanoOcp1::Variant(position.at(0), position.at(1), position.at(2))
            )
        );
    };
    m_upmixIndicatorPaintAndControlComponent = std::make_unique<UmsciUpmixIndicatorPaintNControlComponent>();
    addAndMakeVisible(m_upmixIndicatorPaintAndControlComponent.get());
    m_upmixIndicatorPaintAndControlComponent->onTransformChanged = [this]() {
        if (onUpmixTransformChanged)
            onUpmixTransformChanged();
    };
    m_upmixIndicatorPaintAndControlComponent->onSourcePositionChanged = [this](std::int16_t sourceId, std::array<std::float_t, 3> position) {
        m_sourcePosition[sourceId] = position;
        m_soundobjectsInAreaPaintComponent->setSourcePosition(sourceId, position);
        DeviceController::getInstance()->SetObjectValue(
            DeviceController::RemoteObject(
                DeviceController::RemoteObject::Positioning_SourcePosition,
                DeviceController::RemObjAddr(sourceId, DeviceController::RemObjAddr::sc_INV),
                NanoOcp1::Variant(position.at(0), position.at(1), position.at(2))
            )
        );
    };
}

UmsciControlComponent::~UmsciControlComponent()
{
}

void UmsciControlComponent::paint(Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::Slider::ColourIds::backgroundColourId));

    if (!isDatabaseComplete())
    {
        g.setColour(getLookAndFeel().findColour(juce::TextEditor::ColourIds::textColourId));
        g.drawFittedText("Data not ready...", getLocalBounds().reduced(35), juce::Justification::centred, 2);
        return;
    }
}

void UmsciControlComponent::resized()
{
    auto bounds = getLocalBounds();

    if (m_loudspeakersInAreaPaintComponent && m_loudspeakersInAreaPaintComponent->isVisible())
        m_loudspeakersInAreaPaintComponent->setBounds(bounds);
    if (m_soundobjectsInAreaPaintComponent && m_soundobjectsInAreaPaintComponent->isVisible())
        m_soundobjectsInAreaPaintComponent->setBounds(bounds);
    if (m_upmixIndicatorPaintAndControlComponent && m_upmixIndicatorPaintAndControlComponent->isVisible())
        m_upmixIndicatorPaintAndControlComponent->setBounds(bounds);
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
    // reset the expected database
    setDatabaseComplete(false);
    m_deviceName.clear();
    m_speakerPosition.clear();
    m_sourcePosition.clear();
    m_sourceDelayMode.clear();
    m_sourceSpread.clear();
    m_sourceName.clear();
    m_speakerName.clear();

    // rebuild the object tree
    auto ocp1ObjectTree = std::vector<DeviceController::RemoteObject>();

    ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Settings_DeviceName, DeviceController::RemObjAddr(), NanoOcp1::Variant()));
    for (std::int16_t i = 1; i <= m_ocp1IOSize.first; i++)
    {
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixInput_ChannelName, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixInput_Mute, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixInput_Gain, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Positioning_SourceSpread, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Positioning_SourceDelayMode, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Positioning_SourcePosition, DeviceController::RemObjAddr(i, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
    }
    for (std::int16_t o = 1; o <= m_ocp1IOSize.second; o++)
    {
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixOutput_ChannelName, DeviceController::RemObjAddr(o, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixOutput_Mute, DeviceController::RemObjAddr(o, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::MatrixOutput_Gain, DeviceController::RemObjAddr(o, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
        ocp1ObjectTree.push_back(DeviceController::RemoteObject(DeviceController::RemoteObject::Positioning_SpeakerPosition, DeviceController::RemObjAddr(o, DeviceController::RemObjAddr::sc_INV), NanoOcp1::Variant()));
    }

    DeviceController::getInstance()->SetActiveRemoteObjects(ocp1ObjectTree);
}

void UmsciControlComponent::setRemoteObject(const DeviceController::RemoteObject& obj)
{
    DBG(juce::String(__FUNCTION__) << " " << DeviceController::RemoteObject::GetObjectDescription(obj.Id) << " " << obj.Addr.toNiceString());

    switch (obj.Id)
    {
    case DeviceController::RemoteObject::Settings_DeviceName:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_STRING == obj.Var.GetDataType());
        setDeviceName(obj.Var.ToString());
        break;
    case DeviceController::RemoteObject::MatrixInput_ChannelName:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_STRING == obj.Var.GetDataType());
        setSourceName(obj.Addr.pri, obj.Var.ToString());
        break;
    case DeviceController::RemoteObject::MatrixInput_Mute:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT8 == obj.Var.GetDataType());
        setSourceMute(obj.Addr.pri, obj.Var.ToUInt8());
        break;
    case DeviceController::RemoteObject::MatrixInput_Gain:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_FLOAT32 == obj.Var.GetDataType());
        setSourceGain(obj.Addr.pri, obj.Var.ToFloat());
        break;
    case DeviceController::RemoteObject::Positioning_SourcePosition:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_BLOB /*OCP1DATATYPE_DB_POSITION ?!*/ == obj.Var.GetDataType());
        setSourcePosition(obj.Addr.pri, obj.Var.ToPosition());
        break;
    case DeviceController::RemoteObject::Positioning_SourceDelayMode:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT16 == obj.Var.GetDataType());
        setSourceDelayMode(obj.Addr.pri, obj.Var.ToUInt16());
        break;
    case DeviceController::RemoteObject::Positioning_SourceSpread:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_FLOAT32 == obj.Var.GetDataType());
        setSourceSpread(obj.Addr.pri, obj.Var.ToFloat());
        break;
    case DeviceController::RemoteObject::MatrixOutput_ChannelName:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_STRING == obj.Var.GetDataType());
        setSpeakerName(obj.Addr.pri, obj.Var.ToString());
        break;
    case DeviceController::RemoteObject::MatrixOutput_Mute:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT8 == obj.Var.GetDataType());
        setSpeakerMute(obj.Addr.pri, obj.Var.ToUInt8());
        break;
    case DeviceController::RemoteObject::MatrixOutput_Gain:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_FLOAT32 == obj.Var.GetDataType());
        setSpeakerGain(obj.Addr.pri, obj.Var.ToFloat());
        break;
    case DeviceController::RemoteObject::Positioning_SpeakerPosition:
        jassert(NanoOcp1::Ocp1DataType::OCP1DATATYPE_BLOB /*OCP1DATATYPE_DB_POSITION ?!*/ == obj.Var.GetDataType());
        setSpeakerPosition(obj.Addr.pri, obj.Var.ToAimingAndPosition());
        break;
    //all below fallthrough as unhandled
    case DeviceController::RemoteObject::CoordinateMappingSettings_Flip:
    case DeviceController::RemoteObject::MatrixNode_Enable:
    case DeviceController::RemoteObject::MatrixNode_DelayEnable:
    case DeviceController::RemoteObject::MatrixInput_DelayEnable:
    case DeviceController::RemoteObject::MatrixInput_EqEnable:
    case DeviceController::RemoteObject::MatrixOutput_DelayEnable:
    case DeviceController::RemoteObject::MatrixOutput_EqEnable:
    case DeviceController::RemoteObject::MatrixSettings_ReverbRoomId:
    case DeviceController::RemoteObject::ReverbInputProcessing_EqEnable:
        //datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT16;
    case DeviceController::RemoteObject::ReverbInputProcessing_Mute:
    case DeviceController::RemoteObject::SoundObjectRouting_Mute:
        //datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT8;
    case DeviceController::RemoteObject::MatrixNode_Delay:
    case DeviceController::RemoteObject::MatrixInput_Delay:
    case DeviceController::RemoteObject::MatrixOutput_Delay:
    case DeviceController::RemoteObject::FunctionGroup_Delay:
    case DeviceController::RemoteObject::MatrixNode_Gain:
    case DeviceController::RemoteObject::MatrixInput_ReverbSendGain:
    case DeviceController::RemoteObject::MatrixInput_LevelMeterPreMute:
    case DeviceController::RemoteObject::MatrixInput_LevelMeterPostMute:
    case DeviceController::RemoteObject::MatrixOutput_LevelMeterPreMute:
    case DeviceController::RemoteObject::MatrixOutput_LevelMeterPostMute:
    case DeviceController::RemoteObject::MatrixSettings_ReverbPredelayFactor:
    case DeviceController::RemoteObject::MatrixSettings_ReverbRearLevel:
    case DeviceController::RemoteObject::FunctionGroup_SpreadFactor:
    case DeviceController::RemoteObject::ReverbInput_Gain:
    case DeviceController::RemoteObject::ReverbInputProcessing_Gain:
    case DeviceController::RemoteObject::ReverbInputProcessing_LevelMeter:
    case DeviceController::RemoteObject::SoundObjectRouting_Gain:
        //datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_FLOAT32;
    case DeviceController::RemoteObject::CoordinateMappingSettings_Name:
    case DeviceController::RemoteObject::Fixed_GUID:
    case DeviceController::RemoteObject::Status_StatusText:
    case DeviceController::RemoteObject::Error_ErrorText:
    case DeviceController::RemoteObject::Scene_SceneIndex:
    case DeviceController::RemoteObject::Scene_SceneName:
    case DeviceController::RemoteObject::Scene_SceneComment:
    case DeviceController::RemoteObject::FunctionGroup_Name:
        //datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_STRING;
    case DeviceController::RemoteObject::CoordinateMapping_SourcePosition:
    case DeviceController::RemoteObject::CoordinateMappingSettings_P1real:
    case DeviceController::RemoteObject::CoordinateMappingSettings_P2real:
    case DeviceController::RemoteObject::CoordinateMappingSettings_P3real:
    case DeviceController::RemoteObject::CoordinateMappingSettings_P4real:
    case DeviceController::RemoteObject::CoordinateMappingSettings_P1virtual:
    case DeviceController::RemoteObject::CoordinateMappingSettings_P3virtual:
        //datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_DB_POSITION;
    case DeviceController::RemoteObject::Status_AudioNetworkSampleStatus:
        //datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_INT32;
    case DeviceController::RemoteObject::Error_GnrlErr:
        //datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT8;
    case DeviceController::RemoteObject::MatrixInput_Polarity:
    case DeviceController::RemoteObject::MatrixOutput_Polarity:
        //datatype = NanoOcp1::Ocp1DataType::OCP1DATATYPE_UINT8;
    default:
        DBG(juce::String(__FUNCTION__) << " unhandled/unkown: " << DeviceController::RemoteObject::GetObjectDescription(obj.Id)
            << " (" << static_cast<int>(obj.Addr.pri) << "," << static_cast<int>(obj.Addr.sec) << ") ");
        break;
    }

    if (!m_databaseComplete && checkIsDatabaseComplete())
    {
        setDatabaseComplete(true);
    }
}

bool UmsciControlComponent::checkIsDatabaseComplete()
{
    bool complete = true;

    complete = complete && !m_deviceName.empty();

    complete = complete && m_sourceName.size() == m_ocp1IOSize.first;
    complete = complete && m_sourceMute.size() == m_ocp1IOSize.first;
    complete = complete && m_sourceGain.size() == m_ocp1IOSize.first;
    complete = complete && m_sourceSpread.size() == m_ocp1IOSize.first;
    complete = complete && m_sourceDelayMode.size() == m_ocp1IOSize.first;
    complete = complete && m_sourcePosition.size() == m_ocp1IOSize.first;

    complete = complete && m_speakerName.size() == m_ocp1IOSize.second;
    complete = complete && m_speakerMute.size() == m_ocp1IOSize.second;
    complete = complete && m_speakerGain.size() == m_ocp1IOSize.second;
    complete = complete && m_speakerPosition.size() == m_ocp1IOSize.second;

    return complete;
}

bool UmsciControlComponent::isDatabaseComplete()
{
    return m_databaseComplete;
}

void UmsciControlComponent::updatePaintComponents()
{
    auto realBoundingRect = getRealBoundingRect();
    auto expandAmount = ((realBoundingRect.getAspectRatio() > 1.0f) ? realBoundingRect.getWidth() * 0.2f : realBoundingRect.getHeight() * 0.2f);
    m_boundsRealRef = realBoundingRect.expanded(expandAmount, expandAmount);

    m_loudspeakersInAreaPaintComponent->setBoundsRealRef(m_boundsRealRef);
    m_loudspeakersInAreaPaintComponent->setSpeakerPositions(m_speakerPosition);

    m_soundobjectsInAreaPaintComponent->setBoundsRealRef(m_boundsRealRef);
    m_soundobjectsInAreaPaintComponent->setSourcePositions(m_sourcePosition);

    m_upmixIndicatorPaintAndControlComponent->setSpeakersRealBoundingCube(getRealBoundingCube());
    m_upmixIndicatorPaintAndControlComponent->setBoundsRealRef(m_boundsRealRef);
    m_upmixIndicatorPaintAndControlComponent->setSourcePositions(m_sourcePosition);

    resized();
}

void UmsciControlComponent::setDatabaseComplete(bool complete)
{
    DBG(juce::String(__FUNCTION__) << (complete ? " compl." : " incmplt."));
    m_databaseComplete = complete;

    if (complete)
    {
        updatePaintComponents();

        if (onDatabaseComplete)
            onDatabaseComplete();
    }
    else
    {
        m_deviceName.clear();

        m_sourceName.clear();
        m_sourceMute.clear();
        m_sourceGain.clear();
        m_sourceSpread.clear();
        m_sourceDelayMode.clear();
        m_sourcePosition.clear();

        m_speakerName.clear();
        m_speakerMute.clear();
        m_speakerGain.clear();
        m_speakerPosition.clear();
    }
}

const juce::Rectangle<float> UmsciControlComponent::getRealBoundingRect()
{
    if (m_speakerPosition.size() > 0)
    {
        juce::Range<float> xRange = { m_speakerPosition.begin()->second.at(3), m_speakerPosition.begin()->second.at(3) };
        juce::Range<float> yRange = { m_speakerPosition.begin()->second.at(4), m_speakerPosition.begin()->second.at(4) };
        juce::Range<float> zRange = { m_speakerPosition.begin()->second.at(5), m_speakerPosition.begin()->second.at(5) };

        for (auto const& speakerPosition : m_speakerPosition)
        {
            xRange = xRange.getUnionWith(speakerPosition.second.at(3));
            yRange = yRange.getUnionWith(speakerPosition.second.at(4));
            zRange = zRange.getUnionWith(speakerPosition.second.at(5));
        }

        // Build rect with screen-space-aligned semantics:
        // getX()/getWidth()  = d&b y span (across stage    = screen horizontal)
        // getY()/getHeight() = d&b x span (towards audience = screen vertical)
        return juce::Rectangle<float>({ yRange.getStart(), xRange.getStart() }, { yRange.getEnd(), xRange.getEnd() });
    }

    return {};
}


const std::array<float, 6> UmsciControlComponent::getRealBoundingCube()
{
    if (m_speakerPosition.size() > 0)
    {
        juce::Range<float> xRange = { m_speakerPosition.begin()->second.at(3), m_speakerPosition.begin()->second.at(3) };
        juce::Range<float> yRange = { m_speakerPosition.begin()->second.at(4), m_speakerPosition.begin()->second.at(4) };
        juce::Range<float> zRange = { m_speakerPosition.begin()->second.at(5), m_speakerPosition.begin()->second.at(5) };

        for (auto const& speakerPosition : m_speakerPosition)
        {
            xRange = xRange.getUnionWith(speakerPosition.second.at(3));
            yRange = yRange.getUnionWith(speakerPosition.second.at(4));
            zRange = zRange.getUnionWith(speakerPosition.second.at(5));
        }

        return std::array<float, 6>({ xRange.getStart(), yRange.getStart(), zRange.getStart(), xRange.getEnd(), yRange.getEnd(), zRange.getEnd() });
    }

    return {};
}

void UmsciControlComponent::resetData()
{
    setDatabaseComplete(false);
}

void UmsciControlComponent::setDeviceName(const std::string& name)
{
    m_deviceName = name;
}

void UmsciControlComponent::setSourceName(std::int16_t sourceId, const std::string& name)
{
    m_sourceName[sourceId] = name;
}

void UmsciControlComponent::setSourceMute(std::int16_t sourceId, const std::uint8_t& ocp1MuteValue)
{
    switch (ocp1MuteValue)
    {
    case 2:
        m_sourceMute[sourceId] = false;
        break;
    case 1:
    default:
        m_sourceMute[sourceId] = true;
        break;
    }
    
}

void UmsciControlComponent::setSourceGain(std::int16_t sourceId, const std::float_t& gain)
{
    m_sourceGain[sourceId] = gain;
}

void UmsciControlComponent::setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position)
{
    m_sourcePosition[sourceId] = position;

    if (m_databaseComplete)
    {
        m_soundobjectsInAreaPaintComponent->setSourcePosition(sourceId, position);
        m_upmixIndicatorPaintAndControlComponent->setSourcePosition(sourceId, position);
    }
}

void UmsciControlComponent::setSourceDelayMode(std::int16_t sourceId, const std::uint16_t& delayMode)
{
    m_sourceDelayMode[sourceId] = delayMode;
}

void UmsciControlComponent::setSourceSpread(std::int16_t sourceId, const std::float_t& spread)
{
    m_sourceSpread[sourceId] = spread;
}

void UmsciControlComponent::setSpeakerName(std::int16_t speakerId, const std::string& name)
{
    m_speakerName[speakerId] = name;
}

void UmsciControlComponent::setSpeakerMute(std::int16_t speakerId, const std::uint8_t& ocp1MuteValue)
{
    switch (ocp1MuteValue)
    {
    case 2:
        m_speakerMute[speakerId] = false;
        break;
    case 1:
    default:
        m_speakerMute[speakerId] = true;
        break;
    }
}

void UmsciControlComponent::setSpeakerGain(std::int16_t speakerId, const std::float_t& gain)
{
    m_speakerGain[speakerId] = gain;
}

void UmsciControlComponent::setSpeakerPosition(std::int16_t speakerId, const std::array<std::float_t, 6>& position)
{
    m_speakerPosition[speakerId] = position;

    if (m_databaseComplete)
    {
        // Speaker positions define the coordinate space — recalculate bounds and propagate to all components
        auto realBoundingRect = getRealBoundingRect();
        auto expandAmount = ((realBoundingRect.getAspectRatio() > 1.0f) ? realBoundingRect.getWidth() * 0.2f : realBoundingRect.getHeight() * 0.2f);
        m_boundsRealRef = realBoundingRect.expanded(expandAmount, expandAmount);

        m_loudspeakersInAreaPaintComponent->setBoundsRealRef(m_boundsRealRef);
        m_loudspeakersInAreaPaintComponent->setSpeakerPosition(speakerId, position);

        m_soundobjectsInAreaPaintComponent->setBoundsRealRef(m_boundsRealRef);

        m_upmixIndicatorPaintAndControlComponent->setSpeakersRealBoundingCube(getRealBoundingCube());
        m_upmixIndicatorPaintAndControlComponent->setBoundsRealRef(m_boundsRealRef);

        resized(); // re-maps all screen positions in all children via their resized() callbacks
    }
}

void UmsciControlComponent::setUpmixChannelConfiguration(const juce::AudioChannelSet& upmixChannelConfig)
{
    if (m_upmixIndicatorPaintAndControlComponent)
        m_upmixIndicatorPaintAndControlComponent->setChannelConfiguration(upmixChannelConfig);
}

const juce::AudioChannelSet UmsciControlComponent::getUpmixChannelConfiguration()
{
    if (m_upmixIndicatorPaintAndControlComponent)
        return m_upmixIndicatorPaintAndControlComponent->getChannelConfiguration();
    else
        return {};
}

void UmsciControlComponent::setUpmixSourceStartId(int startId)
{
    if (m_upmixIndicatorPaintAndControlComponent)
        m_upmixIndicatorPaintAndControlComponent->setSourceStartId(startId);
}

int UmsciControlComponent::getUpmixSourceStartId() const
{
    if (m_upmixIndicatorPaintAndControlComponent)
        return m_upmixIndicatorPaintAndControlComponent->getSourceStartId();
    return 1;
}

void UmsciControlComponent::setUpmixLiveMode(bool liveMode)
{
    if (m_upmixIndicatorPaintAndControlComponent)
        m_upmixIndicatorPaintAndControlComponent->setLiveMode(liveMode);
}

bool UmsciControlComponent::getUpmixLiveMode() const
{
    if (m_upmixIndicatorPaintAndControlComponent)
        return m_upmixIndicatorPaintAndControlComponent->getLiveMode();
    return false;
}

void UmsciControlComponent::setUpmixShape(UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape shape)
{
    if (m_upmixIndicatorPaintAndControlComponent)
        m_upmixIndicatorPaintAndControlComponent->setShape(shape);
}

UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape UmsciControlComponent::getUpmixShape() const
{
    if (m_upmixIndicatorPaintAndControlComponent)
        return m_upmixIndicatorPaintAndControlComponent->getShape();
    return UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape::Circle;
}

void UmsciControlComponent::setControlsSize(UmsciPaintNControlComponentBase::ControlsSize size)
{
    if (m_loudspeakersInAreaPaintComponent)
        m_loudspeakersInAreaPaintComponent->setControlsSize(size);
    if (m_soundobjectsInAreaPaintComponent)
        m_soundobjectsInAreaPaintComponent->setControlsSize(size);
    if (m_upmixIndicatorPaintAndControlComponent)
        m_upmixIndicatorPaintAndControlComponent->setControlsSize(size);
}

UmsciPaintNControlComponentBase::ControlsSize UmsciControlComponent::getControlsSize() const
{
    if (m_loudspeakersInAreaPaintComponent)
        return m_loudspeakersInAreaPaintComponent->getControlsSize();
    return UmsciPaintNControlComponentBase::ControlsSize::S;
}

void UmsciControlComponent::setUpmixTransform(float rot, float trans, float heightTrans)
{
    if (m_upmixIndicatorPaintAndControlComponent)
        m_upmixIndicatorPaintAndControlComponent->setUpmixTransform(rot, trans, heightTrans);
}

float UmsciControlComponent::getUpmixRot() const
{
    if (m_upmixIndicatorPaintAndControlComponent)
        return m_upmixIndicatorPaintAndControlComponent->getUpmixRot();
    return 0.0f;
}

float UmsciControlComponent::getUpmixTrans() const
{
    if (m_upmixIndicatorPaintAndControlComponent)
        return m_upmixIndicatorPaintAndControlComponent->getUpmixTrans();
    return 1.0f;
}

float UmsciControlComponent::getUpmixHeightTrans() const
{
    if (m_upmixIndicatorPaintAndControlComponent)
        return m_upmixIndicatorPaintAndControlComponent->getUpmixHeightTrans();
    return 0.6f;
}

