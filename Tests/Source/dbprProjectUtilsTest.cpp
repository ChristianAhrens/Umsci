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

// Coverage for the dbpr:: value types' string round-trip serialisation and
// derived-state helpers (isNull/isEmpty/getInfoString/clear) -- the part of
// dbprProjectUtils that is pure data manipulation, independent of the SQLite
// file reading done by ProjectData::openAndReadProject(). That entry point
// needs a real .dbpr fixture file and is deliberately left uncovered here.
//
// Numeric fields are given values that round-trip exactly through
// std::ostringstream's default 6-significant-digit formatting (used by
// toString()), so equality checks below can be exact rather than
// tolerance-based.

#include <JuceHeader.h>
#include <dbprProjectUtils.h>

using namespace dbpr;

namespace
{
    CoordinateMappingData makeCoordinateMappingData()
    {
        CoordinateMappingData d;
        d.name = "MappingA";
        d.venueObjectId = 42; // deliberately not round-tripped, see toString() note below
        d.flip = true;
        d.vp1x = 1.5;  d.vp1y = 2.5;  d.vp1z = 3.5;
        d.vp3x = 4.5;  d.vp3y = 5.5;  d.vp3z = 6.5;
        d.rp1x = 7.5;  d.rp1y = 8.5;  d.rp1z = 9.5;
        d.rp2x = 10.5; d.rp2y = 11.5; d.rp2z = 12.5;
        d.rp3x = 13.5; d.rp3y = 14.5; d.rp3z = 15.5;
        d.rp4x = 16.5; d.rp4y = 17.5; d.rp4z = 18.5;
        return d;
    }
}

class CoordinateMappingDataTest : public juce::UnitTest
{
public:
    CoordinateMappingDataTest() : juce::UnitTest ("dbpr::CoordinateMappingData", "Umsci") {}

    void runTest() override
    {
        beginTest ("toString()/fromString() round-trips all coordinate fields and flip");
        {
            auto original = makeCoordinateMappingData();
            auto roundTripped = CoordinateMappingData::fromString (original.toString());

            expectEquals (roundTripped.name, original.name);
            expect (roundTripped.flip == original.flip);
            expectEquals (roundTripped.vp1x, original.vp1x);
            expectEquals (roundTripped.vp1y, original.vp1y);
            expectEquals (roundTripped.vp1z, original.vp1z);
            expectEquals (roundTripped.rp4x, original.rp4x);
            expectEquals (roundTripped.rp4y, original.rp4y);
            expectEquals (roundTripped.rp4z, original.rp4z);
        }

        beginTest ("toString()/fromString() does not round-trip venueObjectId (by design)");
        {
            auto original = makeCoordinateMappingData();
            auto roundTripped = CoordinateMappingData::fromString (original.toString());

            expectEquals (roundTripped.venueObjectId, 0);
        }

        beginTest ("Default-constructed data is null");
        {
            CoordinateMappingData d;
            expect (d.isNull());
        }

        beginTest ("Data with a non-zero coordinate is not null");
        {
            auto d = makeCoordinateMappingData();
            expect (! d.isNull());
        }
    }
};

static CoordinateMappingDataTest coordinateMappingDataTest;

class SpeakerPositionDataTest : public juce::UnitTest
{
public:
    SpeakerPositionDataTest() : juce::UnitTest ("dbpr::SpeakerPositionData", "Umsci") {}

    void runTest() override
    {
        beginTest ("toString()/fromString() round-trips position and aiming angles");
        {
            SpeakerPositionData original;
            original.x = 1.5; original.y = -2.5; original.z = 3.5;
            original.hor = 45.0; original.vrt = -10.5; original.rot = 180.0;

            auto roundTripped = SpeakerPositionData::fromString (original.toString());

            expectEquals (roundTripped.x, original.x);
            expectEquals (roundTripped.y, original.y);
            expectEquals (roundTripped.z, original.z);
            expectEquals (roundTripped.hor, original.hor);
            expectEquals (roundTripped.vrt, original.vrt);
            expectEquals (roundTripped.rot, original.rot);
        }

        beginTest ("Default-constructed data is null");
        {
            SpeakerPositionData d;
            expect (d.isNull());
        }

        beginTest ("Data with a non-zero field is not null");
        {
            SpeakerPositionData d;
            d.rot = 90.0;
            expect (! d.isNull());
        }
    }
};

static SpeakerPositionDataTest speakerPositionDataTest;

class FunctionGroupDataTest : public juce::UnitTest
{
public:
    FunctionGroupDataTest() : juce::UnitTest ("dbpr::FunctionGroupData", "Umsci") {}

    void runTest() override
    {
        beginTest ("toString()/fromString() round-trips name and mode");
        {
            FunctionGroupData original;
            original.name = "Group1";
            original.mode = 1;

            auto roundTripped = FunctionGroupData::fromString (original.toString());

            expectEquals (roundTripped.name, original.name);
            expectEquals (roundTripped.mode, original.mode);
        }
    }
};

static FunctionGroupDataTest functionGroupDataTest;

class ProjectDataTest : public juce::UnitTest
{
public:
    ProjectDataTest() : juce::UnitTest ("dbpr::ProjectData", "Umsci") {}

    void runTest() override
    {
        beginTest ("A default-constructed ProjectData is empty");
        {
            ProjectData data;
            expect (data.isEmpty());
        }

        beginTest ("isEmpty() only inspects coordinateMappingData/speakerPositionData -- "
                    "matrixInputData/functionGroupData alone do not make a project non-empty");
        {
            ProjectData data;
            data.matrixInputData[1] = MatrixInputData { 1, "Input1", 1 };
            data.functionGroupData[1] = FunctionGroupData { "Group1", 1 };

            expect (data.isEmpty());
        }

        beginTest ("A populated coordinateMappingData makes the project non-empty");
        {
            ProjectData data;
            data.coordinateMappingData[1] = makeCoordinateMappingData();

            expect (! data.isEmpty());
        }

        beginTest ("clear() empties all five member containers");
        {
            ProjectData data;
            data.coordinateMappingData[1] = makeCoordinateMappingData();
            data.speakerPositionData[1] = SpeakerPositionData();
            data.matrixInputData[1] = MatrixInputData { 1, "Input1", 1 };
            data.functionGroupData[1] = FunctionGroupData { "Group1", 1 };
            data.matrixInputDeviceIds.insert (1);

            data.clear();

            expect (data.coordinateMappingData.empty());
            expect (data.speakerPositionData.empty());
            expect (data.matrixInputData.empty());
            expect (data.functionGroupData.empty());
            expect (data.matrixInputDeviceIds.empty());
        }

        beginTest ("getInfoString() counts only non-null mapping/speaker entries and En-Scene inputs");
        {
            ProjectData data;
            data.coordinateMappingData[1] = makeCoordinateMappingData();
            data.coordinateMappingData[2] = CoordinateMappingData(); // null -- not counted
            data.speakerPositionData[1] = SpeakerPositionData { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
            data.matrixInputData[1] = MatrixInputData { 1, "Input1", 1 };  // En-Scene -- counted
            data.matrixInputData[2] = MatrixInputData { 1, "Input2", 0 };  // matrix-only -- not counted
            data.functionGroupData[1] = FunctionGroupData { "Group1", 1 };

            expectEquals (data.getInfoString(), std::string ("1 CMP, 1 SPK, 1 SO, 1 FG"));
        }

        beginTest ("toString() returns an empty string for an empty project");
        {
            ProjectData data;
            expect (data.toString().empty());
        }

        beginTest ("toString()/fromString() round-trips coordinateMappingData, speakerPositionData, "
                    "matrixInputData and functionGroupData (matrixInputDeviceIds is not part of the "
                    "serialised format and is therefore not round-tripped)");
        {
            ProjectData original;
            original.coordinateMappingData[1] = makeCoordinateMappingData();
            original.speakerPositionData[3] = SpeakerPositionData { 1.5, 2.5, 3.5, 4.5, 5.5, 6.5 };
            original.matrixInputData[5] = MatrixInputData { 2, "Input5", 1 };
            original.functionGroupData[7] = FunctionGroupData { "Group7", 0 };
            original.matrixInputDeviceIds.insert (2);

            auto roundTripped = ProjectData::fromString (original.toString());

            expect (roundTripped.coordinateMappingData.count (1) == 1);
            expectEquals (roundTripped.coordinateMappingData.at (1).name, original.coordinateMappingData.at (1).name);

            expect (roundTripped.speakerPositionData.count (3) == 1);
            expectEquals (roundTripped.speakerPositionData.at (3).hor, original.speakerPositionData.at (3).hor);

            expect (roundTripped.matrixInputData.count (5) == 1);
            expectEquals (roundTripped.matrixInputData.at (5).name, original.matrixInputData.at (5).name);
            expect (roundTripped.matrixInputData.at (5).isEnScene());

            expect (roundTripped.functionGroupData.count (7) == 1);
            expectEquals (roundTripped.functionGroupData.at (7).name, original.functionGroupData.at (7).name);

            expect (roundTripped.matrixInputDeviceIds.empty());
        }

        beginTest ("fromString() on an empty string returns a default-constructed (empty) ProjectData");
        {
            auto data = ProjectData::fromString ("");
            expect (data.isEmpty());
        }
    }
};

static ProjectDataTest projectDataTest;
