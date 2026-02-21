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


class UmsciAppConfiguration : public JUCEAppBasics::AppConfigurationBase
{

public:
    enum TagID
    {
        CONNECTIONCONFIG,
        VISUCONFIG,
        CONTROLCOLOUR,
        LOOKANDFEEL,
        CONTROLSSIZE,
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
        case CONTROLSSIZE:
            return "CONTROLSSIZE";
        default:
            return "INVALID";
        }
    };

    enum AttributeID
    {
        ENABLED,
        IP,
        PORT,
        IOSIZE
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
        default:
            return "-";
        }
    };

public:
    explicit UmsciAppConfiguration(const File &file);
    ~UmsciAppConfiguration() override;

    bool isValid() override;
    static bool isValid(const std::unique_ptr<juce::XmlElement>& xmlConfig);

    bool ResetToDefault();

protected:
    bool HandleConfigVersionConflict(const Version& configVersionFound) override;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciAppConfiguration)
};

