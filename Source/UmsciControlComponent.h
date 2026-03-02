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
#include "DeviceController.h"


/*Fwd decls*/
class UmsciLoudspeakersPaintComponent;
class UmsciSoundobjectsPaintComponent;
class UmsciUpmixIndicatorPaintNControlComponent;

class UmsciControlComponent :   public juce::Component, public UmsciAppConfiguration::XmlConfigurableElement
{
public:
    UmsciControlComponent();
    ~UmsciControlComponent() override;

    //==============================================================================
    void resized() override;
    void paint(juce::Graphics& g) override;

    //==============================================================================
    std::unique_ptr<XmlElement> createStateXml() override;
    bool setStateXml(XmlElement* stateXml) override;

    //==============================================================================
    const std::pair<int, int>& getOcp1IOSize();
    void setOcp1IOSize(const std::pair<int, int>& ioSize);

    //==============================================================================
    void setDeviceName(const std::string& name);

    void setSourceName(std::int16_t sourceId, const std::string& name);
    void setSourceMute(std::int16_t sourceId, const std::uint8_t& mute);
    void setSourceGain(std::int16_t sourceId, const std::float_t& gain);
    void setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position);
    void setSourceDelayMode(std::int16_t sourceId, const std::uint16_t& delayMode);
    void setSourceSpread(std::int16_t sourceId, const std::float_t& spread);

    void setSpeakerName(std::int16_t speakerId, const std::string& name);
    void setSpeakerMute(std::int16_t speakerId, const std::uint8_t& mute);
    void setSpeakerGain(std::int16_t speakerId, const std::float_t& gain);
    void setSpeakerPosition(std::int16_t speakerId, const std::array<std::float_t, 6>& position);

    //==============================================================================
    void setUpmixChannelConfiguration(const juce::AudioChannelSet& upmixChannelConfig);

    //==============================================================================
    void resetData();

    //==============================================================================
    std::function<void()> onDatabaseComplete;

private:
    //==============================================================================
    void rebuildOcp1ObjectTree();
    void updatePaintComponents();

    void setRemoteObject(const DeviceController::RemoteObject& obj);

    bool checkIsDatabaseComplete();
    bool isDatabaseComplete();
    void setDatabaseComplete(bool complete);

    const juce::Rectangle<float> getRealBoundingRect();
    const std::array<float, 6> getRealBoundingCube();

    //==============================================================================
    std::unique_ptr<UmsciLoudspeakersPaintComponent>            m_loudspeakersInAreaPaintComponent;
    std::unique_ptr<UmsciSoundobjectsPaintComponent>            m_soundobjectsInAreaPaintComponent;
    std::unique_ptr<UmsciUpmixIndicatorPaintNControlComponent>  m_upmixIndicatorPaintAndControlComponent;

    std::pair<int, int> m_ocp1IOSize;

    bool    m_databaseComplete = false;

    juce::Rectangle<float>  m_boundsRealRef;
    juce::Rectangle<int>    m_umsciPaintComponentBounds;

    std::string                                         m_deviceName;

    std::map<std::int16_t, std::string>                 m_sourceName;
    std::map<std::int16_t, bool>                        m_sourceMute;
    std::map<std::int16_t, std::float_t>                m_sourceGain;
    std::map<std::int16_t, std::array<std::float_t, 3>> m_sourcePosition;
    std::map<std::int16_t, std::uint16_t>               m_sourceDelayMode;
    std::map<std::int16_t, std::float_t>                m_sourceSpread;

    std::map<std::int16_t, std::string>                 m_speakerName;
    std::map<std::int16_t, bool>                        m_speakerMute;
    std::map<std::int16_t, std::float_t>                m_speakerGain;
    std::map<std::int16_t, std::array<std::float_t, 6>> m_speakerPosition;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciControlComponent)
};

