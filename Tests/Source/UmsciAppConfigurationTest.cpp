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

// Coverage for UmsciAppConfiguration::isValid() (the static, XmlElement-taking
// overload) -- the gate that decides whether a loaded/received config file is
// complete enough to apply. Deliberately does NOT construct a full
// UmsciAppConfiguration instance: its constructor does real file I/O and starts a
// background flush thread (see AppConfigurationBase::InitializeBase()), which is
// both unnecessary for testing the pure validation logic and adds real risk of an
// untested lifecycle interaction in a plain console-app test harness. The static
// isValid() overload only needs a live juce::JUCEApplication instance (for the
// application-name tag check in JUCEAppBasics::AppConfigurationBase::isValid()),
// which is provided locally and torn down at the end of the test.

#include <JuceHeader.h>
#include <UmsciAppConfiguration.h>

namespace
{
    // UmsciAppConfiguration::isValid() calls (via AppConfigurationBase::isValid())
    // juce::JUCEApplication::getInstance()->getApplicationName(), which is null outside a
    // real JUCEApplication. Constructing this subclass is enough to satisfy that --
    // JUCEApplicationBase's constructor just registers the global instance pointer; the
    // message loop is never started and initialise()/shutdown() are never invoked.
    class DummyUmsciApplication : public juce::JUCEApplication
    {
    public:
        const juce::String getApplicationName() override { return "Umsci"; }
        const juce::String getApplicationVersion() override { return "0.0.0"; }
        void initialise (const juce::String&) override {}
        void shutdown() override {}
    };

    // Mirrors the structure MainComponent/UmsciAppConfiguration build up in the
    // real app, and what UmsciDefault.config contains -- one of each section
    // isValid() dereferences without a null check.
    std::unique_ptr<juce::XmlElement> makeValidConfig()
    {
        auto root = std::make_unique<juce::XmlElement> ("Umsci");

        auto* connectionConfig = root->createNewChildElement (UmsciAppConfiguration::getTagName (UmsciAppConfiguration::TagID::CONNECTIONCONFIG));
        connectionConfig->setAttribute (UmsciAppConfiguration::getAttributeName (UmsciAppConfiguration::AttributeID::IP), "127.0.0.1");
        connectionConfig->setAttribute (UmsciAppConfiguration::getAttributeName (UmsciAppConfiguration::AttributeID::PORT), 50014);

        auto* visuConfig = root->createNewChildElement (UmsciAppConfiguration::getTagName (UmsciAppConfiguration::TagID::VISUCONFIG));
        visuConfig->createNewChildElement (UmsciAppConfiguration::getTagName (UmsciAppConfiguration::TagID::CONTROLCOLOUR));
        visuConfig->createNewChildElement (UmsciAppConfiguration::getTagName (UmsciAppConfiguration::TagID::LOOKANDFEEL));

        auto* controlConfig = root->createNewChildElement (UmsciAppConfiguration::getTagName (UmsciAppConfiguration::TagID::CONTROLCONFIG));
        controlConfig->setAttribute (UmsciAppConfiguration::getAttributeName (UmsciAppConfiguration::AttributeID::IOSIZE), "64,64");

        auto* upmixConfig = root->createNewChildElement (UmsciAppConfiguration::getTagName (UmsciAppConfiguration::TagID::UPMIXCONFIG));
        upmixConfig->setAttribute (UmsciAppConfiguration::getAttributeName (UmsciAppConfiguration::AttributeID::UPMIXSOURCESTARTID), 1);
        upmixConfig->setAttribute (UmsciAppConfiguration::getAttributeName (UmsciAppConfiguration::AttributeID::UPMIXLIVEMODE), 1);
        upmixConfig->setAttribute (UmsciAppConfiguration::getAttributeName (UmsciAppConfiguration::AttributeID::UPMIXSHAPE), "Circle");

        return root;
    }
}

class UmsciAppConfigurationIsValidTest : public juce::UnitTest
{
public:
    UmsciAppConfigurationIsValidTest() : juce::UnitTest ("UmsciAppConfiguration::isValid", "Umsci") {}

    void runTest() override
    {
        DummyUmsciApplication dummyApp;

        beginTest ("A fully populated config tree is valid");
        {
            auto config = makeValidConfig();
            expect (UmsciAppConfiguration::isValid (config));
        }

        beginTest ("A null config is invalid");
        {
            std::unique_ptr<juce::XmlElement> nullConfig;
            expect (! UmsciAppConfiguration::isValid (nullConfig));
        }

        beginTest ("Wrong root tag name is invalid");
        {
            auto config = std::make_unique<juce::XmlElement> ("NotUmsci");
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("Missing CONNECTIONCONFIG is invalid");
        {
            auto config = makeValidConfig();
            config->removeChildElement (config->getChildByName ("CONNECTIONCONFIG"), true);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("CONNECTIONCONFIG with IP=ERROR is invalid");
        {
            auto config = makeValidConfig();
            config->getChildByName ("CONNECTIONCONFIG")->setAttribute ("IP", "ERROR");
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("CONNECTIONCONFIG with PORT=-1 is invalid");
        {
            auto config = makeValidConfig();
            config->getChildByName ("CONNECTIONCONFIG")->setAttribute ("PORT", -1);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("Missing VISUCONFIG is invalid");
        {
            auto config = makeValidConfig();
            config->removeChildElement (config->getChildByName ("VISUCONFIG"), true);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("VISUCONFIG missing CONTROLCOLOUR is invalid");
        {
            auto config = makeValidConfig();
            auto* visuConfig = config->getChildByName ("VISUCONFIG");
            visuConfig->removeChildElement (visuConfig->getChildByName ("CONTROLCOLOUR"), true);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("VISUCONFIG missing LOOKANDFEEL is invalid");
        {
            auto config = makeValidConfig();
            auto* visuConfig = config->getChildByName ("VISUCONFIG");
            visuConfig->removeChildElement (visuConfig->getChildByName ("LOOKANDFEEL"), true);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("Missing CONTROLCONFIG is invalid");
        {
            auto config = makeValidConfig();
            config->removeChildElement (config->getChildByName ("CONTROLCONFIG"), true);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("CONTROLCONFIG with IOSIZE=ERROR is invalid");
        {
            auto config = makeValidConfig();
            config->getChildByName ("CONTROLCONFIG")->setAttribute ("IOSIZE", "ERROR");
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("Missing UPMIXCONFIG is invalid");
        {
            auto config = makeValidConfig();
            config->removeChildElement (config->getChildByName ("UPMIXCONFIG"), true);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("UPMIXCONFIG with UPMIXSOURCESTARTID=-1 is invalid");
        {
            auto config = makeValidConfig();
            config->getChildByName ("UPMIXCONFIG")->setAttribute ("UPMIXSOURCESTARTID", -1);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("UPMIXCONFIG with UPMIXLIVEMODE=-1 is invalid");
        {
            auto config = makeValidConfig();
            config->getChildByName ("UPMIXCONFIG")->setAttribute ("UPMIXLIVEMODE", -1);
            expect (! UmsciAppConfiguration::isValid (config));
        }

        beginTest ("UPMIXCONFIG with UPMIXSHAPE=ERROR is invalid");
        {
            auto config = makeValidConfig();
            config->getChildByName ("UPMIXCONFIG")->setAttribute ("UPMIXSHAPE", "ERROR");
            expect (! UmsciAppConfiguration::isValid (config));
        }
    }
};

static UmsciAppConfigurationIsValidTest umsciAppConfigurationIsValidTest;
