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

#include <AppConfigurationBase.h>


#define Umsci_CONFIG_VERSION "1.0.0"


/**
 * @class UmsciAppConfiguration
 * @brief Persists all Umsci user settings to an XML file via the
 *        `JUCEAppBasics::AppConfigurationBase` framework.
 *
 * ## XML structure (top-level tags)
 * | Tag (TagID)       | Contents |
 * |---|---|
 * | CONNECTIONCONFIG  | IP, port, I/O size (enabled flag, target address). |
 * | VISUCONFIG        | Look-and-feel style, control colour. |
 * | CONTROLCOLOUR     | Packed colour value for source/speaker icons. |
 * | LOOKANDFEEL       | LookAndFeel variant index. |
 * | CONTROLCONFIG     | Control size (S/M/L). |
 * | CONTROLFORMAT     | Upmix channel format (e.g. 5.1, 7.1.4). |
 * | UPMIXCONFIG       | Start source ID, live mode, shape, show-all-sources flag. |
 * | CONTROLSIZE       | Icon size override. |
 * | UPMIXROT          | Upmix ring rotation (float). |
 * | UPMIXSCALE        | Upmix ring radial scale (float). |
 * | UPMIXHEIGHTSCALE  | Upmix height ring scale relative to floor ring (float). |
 * | UPMIXANGLESTRETCH | Upmix angular stretch factor (float). |
 * | UPMIXOFFSETX/Y    | Upmix ring X/Y centre offset (float each). |
 *
 * `MainComponent` owns the single instance, registered as both Dumper and Watcher
 * so that settings are written on change and re-applied on external file updates.
 */
class UmsciAppConfiguration : public JUCEAppBasics::AppConfigurationBase
{

public:
    /** @brief Enumerates every XML element tag used in the config file. */
    enum TagID
    {
        CONNECTIONCONFIG,   ///< OCP.1 connection parameters.
        VISUCONFIG,         ///< Visual appearance settings.
        CONTROLCOLOUR,      ///< Source/speaker icon colour.
        LOOKANDFEEL,        ///< Look-and-feel variant.
        CONTROLCONFIG,      ///< Control-component settings.
        CONTROLFORMAT,      ///< Upmix channel format.
        UPMIXCONFIG,        ///< Upmix behaviour settings.
        CONTROLSIZE,        ///< Icon size (S/M/L).
        EXTERNALCONTROLCONFIG,  ///< External (MIDI) control assignments container.
        MIDIINPUTDEVICE,        ///< Selected MIDI input device identifier.
        MIDI_UPMIXROT,          ///< MIDI assignment for upmix rotation.
        MIDI_UPMIXSCALE,        ///< MIDI assignment for upmix translation (radial scale).
        MIDI_UPMIXHEIGHTSCALE,  ///< MIDI assignment for upmix height translation.
        MIDI_UPMIXANGLESTRETCH, ///< MIDI assignment for upmix angle stretch.
        MIDI_UPMIXOFFSETX,      ///< MIDI assignment for upmix X offset.
        MIDI_UPMIXOFFSETY,      ///< MIDI assignment for upmix Y offset.
        UPMIXSNAPSHOTCONFIG     ///< Upmix snapshot container (optional; absent = no snapshot stored).
    };
    static juce::String getTagName(TagID ID)
    {
        switch(ID)
        {
        case CONNECTIONCONFIG:
            return "CONNECTIONCONFIG";
        case VISUCONFIG:
            return "VISUCONFIG";
        case CONTROLCOLOUR:
            return "CONTROLCOLOUR";
        case LOOKANDFEEL:
            return "LOOKANDFEEL";
        case CONTROLCONFIG:
            return "CONTROLCONFIG";
        case CONTROLFORMAT:
            return "CONTROLFORMAT";
        case UPMIXCONFIG:
            return "UPMIXCONFIG";
        case CONTROLSIZE:
            return "CONTROLSIZE";
        case EXTERNALCONTROLCONFIG:
            return "EXTERNALCONTROLCONFIG";
        case MIDIINPUTDEVICE:
            return "MIDIINPUTDEVICE";
        case MIDI_UPMIXROT:
            return "MIDI_UPMIXROT";
        case MIDI_UPMIXSCALE:
            return "MIDI_UPMIXSCALE";
        case MIDI_UPMIXHEIGHTSCALE:
            return "MIDI_UPMIXHEIGHTSCALE";
        case MIDI_UPMIXANGLESTRETCH:
            return "MIDI_UPMIXANGLESTRETCH";
        case MIDI_UPMIXOFFSETX:
            return "MIDI_UPMIXOFFSETX";
        case MIDI_UPMIXOFFSETY:
            return "MIDI_UPMIXOFFSETY";
        case UPMIXSNAPSHOTCONFIG:
            return "UPMIXSNAPSHOTCONFIG";
        default:
            return "INVALID";
        }
    };

    /** @brief Enumerates every XML attribute name used within config elements. */
    enum AttributeID
    {
        ENABLED,             ///< Boolean: whether the connection is active.
        IP,                  ///< String: target device IP address.
        PORT,                ///< Integer: target OCP.1 port (default 50014).
        IOSIZE,              ///< String: "inputs,outputs" channel count pair.
        UPMIXSOURCESTARTID,  ///< Integer: 1-based first DS100 channel for upmix inputs.
        UPMIXLIVEMODE,       ///< Boolean: follow live DS100 source positions.
        UPMIXSHAPE,          ///< String: "Circle" or "Rectangle".
        UPMIXSHOWALLSOURCES  ///< Boolean: render all sources or only upmix group.
    };
    static juce::String getAttributeName(AttributeID ID)
    {
        switch (ID)
        {
        case ENABLED:
            return "ENABLED";
        case IP:
            return "IP";
        case PORT:
            return "PORT";
        case IOSIZE:
            return "IOSIZE";
        case UPMIXSOURCESTARTID:
            return "UPMIXSOURCESTARTID";
        case UPMIXLIVEMODE:
            return "UPMIXLIVEMODE";
        case UPMIXSHAPE:
            return "UPMIXSHAPE";
        case UPMIXSHOWALLSOURCES:
            return "UPMIXSHOWALLSOURCES";
        default:
            return "-";
        }
    };

public:
    explicit UmsciAppConfiguration(const File &file);
    ~UmsciAppConfiguration() override;

    /** @brief Returns true if the XML file exists and contains the expected root tags. */
    bool isValid() override;
    /** @brief Static overload for validating a pre-loaded XmlElement without an instance. */
    static bool isValid(const std::unique_ptr<juce::XmlElement>& xmlConfig);

    /** @brief Overwrites all settings with built-in defaults and triggers a dump. */
    bool ResetToDefault();

protected:
    /**
     * @brief Called by the base class when the config file's version string does not
     *        match `Umsci_CONFIG_VERSION`.  Currently discards the old file and resets
     *        to defaults rather than attempting migration.
     */
    bool HandleConfigVersionConflict(const Version& configVersionFound) override;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciAppConfiguration)
};

