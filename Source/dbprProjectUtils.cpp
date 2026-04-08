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

#include "dbprProjectUtils.h"

#ifdef USE_DBPR_PROJECT_UTILS

#include <SQLiteCpp/SQLiteCpp.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{

//==============================================================================
// Internal helpers

std::vector<std::string> splitString(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(s);
    while (std::getline(stream, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

int parseIntValue(const std::string& s)
{
    try { return std::stoi(s); }
    catch (...) { return 0; }
}

double parseDoubleValue(const std::string& s)
{
    try { return std::stod(s); }
    catch (...) { return 0.0; }
}

//==============================================================================
/**
 * Minimal 2D column-vector affine transform:
 *   x' = mat00 * x + mat01 * y + mat02
 *   y' = mat10 * x + mat11 * y + mat12
 */
struct AffineTransform2D
{
    double mat00 = 1.0, mat01 = 0.0, mat02 = 0.0;
    double mat10 = 0.0, mat11 = 1.0, mat12 = 0.0;

    static AffineTransform2D translation(double tx, double ty)
    {
        return { 1.0, 0.0, tx, 0.0, 1.0, ty };
    }

    static AffineTransform2D scale(double sx, double sy, double cx, double cy)
    {
        return { sx, 0.0, cx * (1.0 - sx), 0.0, sy, cy * (1.0 - sy) };
    }

    static AffineTransform2D rotation(double angleRad, double cx, double cy)
    {
        const double cosA = std::cos(angleRad);
        const double sinA = std::sin(angleRad);
        return { cosA, -sinA, cx - cosA * cx + sinA * cy,
                 sinA,  cosA, cy - sinA * cx - cosA * cy };
    }

    void applyTo(double x, double y, double& outX, double& outY) const
    {
        outX = mat00 * x + mat01 * y + mat02;
        outY = mat10 * x + mat11 * y + mat12;
    }
};

} // anonymous namespace

//==============================================================================

namespace dbpr
{

//==============================================================================
// CoordinateMappingData

std::string CoordinateMappingData::toString() const
{
    std::ostringstream oss;
    oss << name << ","
        << static_cast<int>(flip) << ","
        << vp1x << "," << vp1y << "," << vp1z << ","
        << vp3x << "," << vp3y << "," << vp3z << ","
        << rp1x << "," << rp1y << "," << rp1z << ","
        << rp2x << "," << rp2y << "," << rp2z << ","
        << rp3x << "," << rp3y << "," << rp3z << ","
        << rp4x << "," << rp4y << "," << rp4z;
    return oss.str();
}

CoordinateMappingData CoordinateMappingData::fromString(const std::string& s)
{
    const auto tokens = splitString(s, ',');
    if (tokens.size() != 20)
    {
        assert(false);
        return {};
    }

    CoordinateMappingData retv;
    retv.name = tokens[0];
    retv.flip = parseIntValue(tokens[1]) == 1;
    retv.vp1x = parseDoubleValue(tokens[2]);
    retv.vp1y = parseDoubleValue(tokens[3]);
    retv.vp1z = parseDoubleValue(tokens[4]);
    retv.vp3x = parseDoubleValue(tokens[5]);
    retv.vp3y = parseDoubleValue(tokens[6]);
    retv.vp3z = parseDoubleValue(tokens[7]);
    retv.rp1x = parseDoubleValue(tokens[8]);
    retv.rp1y = parseDoubleValue(tokens[9]);
    retv.rp1z = parseDoubleValue(tokens[10]);
    retv.rp2x = parseDoubleValue(tokens[11]);
    retv.rp2y = parseDoubleValue(tokens[12]);
    retv.rp2z = parseDoubleValue(tokens[13]);
    retv.rp3x = parseDoubleValue(tokens[14]);
    retv.rp3y = parseDoubleValue(tokens[15]);
    retv.rp3z = parseDoubleValue(tokens[16]);
    retv.rp4x = parseDoubleValue(tokens[17]);
    retv.rp4y = parseDoubleValue(tokens[18]);
    retv.rp4z = parseDoubleValue(tokens[19]);
    return retv;
}

bool CoordinateMappingData::isNull() const
{
    return (vp1x == 0.0 && vp1y == 0.0 && vp1z == 0.0 &&
            vp3x == 0.0 && vp3y == 0.0 && vp3z == 0.0 &&
            rp1x == 0.0 && rp1y == 0.0 && rp1z == 0.0 &&
            rp2x == 0.0 && rp2y == 0.0 && rp2z == 0.0 &&
            rp3x == 0.0 && rp3y == 0.0 && rp3z == 0.0 &&
            rp4x == 0.0 && rp4y == 0.0 && rp4z == 0.0);
}

//==============================================================================
// SpeakerPositionData

std::string SpeakerPositionData::toString() const
{
    std::ostringstream oss;
    oss << x << "," << y << "," << z << "," << hor << "," << vrt << "," << rot;
    return oss.str();
}

SpeakerPositionData SpeakerPositionData::fromString(const std::string& s)
{
    const auto tokens = splitString(s, ',');
    if (tokens.size() != 6)
    {
        assert(false);
        return {};
    }

    SpeakerPositionData retv;
    retv.x   = parseDoubleValue(tokens[0]);
    retv.y   = parseDoubleValue(tokens[1]);
    retv.z   = parseDoubleValue(tokens[2]);
    retv.hor = parseDoubleValue(tokens[3]);
    retv.vrt = parseDoubleValue(tokens[4]);
    retv.rot = parseDoubleValue(tokens[5]);
    return retv;
}

bool SpeakerPositionData::isNull() const
{
    return (x == 0.0 && y == 0.0 && z == 0.0 &&
            hor == 0.0 && vrt == 0.0 && rot == 0.0);
}

//==============================================================================
// ProjectData

bool ProjectData::isEmpty() const
{
    return coordinateMappingData.empty() && speakerPositionData.empty();
}

std::string ProjectData::getInfoString() const
{
    auto cmdCount = 0;
    auto spdCount = 0;
    for (auto const& kv : coordinateMappingData)
        if (!kv.second.isNull())
            cmdCount++;
    for (auto const& kv : speakerPositionData)
        if (!kv.second.isNull())
            spdCount++;
    return std::to_string(cmdCount) + " CMP, " + std::to_string(spdCount) + " SPK";
}

void ProjectData::clear()
{
    coordinateMappingData.clear();
    speakerPositionData.clear();
}

std::string ProjectData::toString() const
{
    if (isEmpty())
        return {};

    std::ostringstream oss;
    oss << "|COORDMAPDATA|";
    for (auto const& kv : coordinateMappingData)
        oss << kv.first << ":" << kv.second.toString() << ";";
    oss << "|SPKPOSDATA|";
    for (auto const& kv : speakerPositionData)
        oss << kv.first << ":" << kv.second.toString() << ";";
    oss << "|INPUTNAMEDATA|";
    for (auto const& kv : inputNameData)
        oss << kv.first << ":" << kv.second << ";";
    return oss.str();
}

ProjectData ProjectData::fromString(const std::string& s)
{
    if (s.empty())
        return ProjectData();

    const auto sa = splitString(s, '|');
    if (sa.size() != 7)
    {
        assert(false);
        return {};
    }

    ProjectData retv;

    for (auto const& entry : splitString(sa[2], ';'))
    {
        const auto kv = splitString(entry, ':');
        if (kv.size() == 2)
            retv.coordinateMappingData[parseIntValue(kv[0])] = CoordinateMappingData::fromString(kv[1]);
    }

    for (auto const& entry : splitString(sa[4], ';'))
    {
        const auto kv = splitString(entry, ':');
        if (kv.size() == 2)
            retv.speakerPositionData[parseIntValue(kv[0])] = SpeakerPositionData::fromString(kv[1]);
    }

    for (auto const& entry : splitString(sa[6], ';'))
    {
        const auto kv = splitString(entry, ':');
        if (kv.size() == 2)
            retv.inputNameData[parseIntValue(kv[0])] = kv[1];
    }

    return retv;
}

ProjectData ProjectData::openAndReadProject(const std::string& projectFilePath)
{
    // SQLiteCpp requires exception handling
    try
    {
        ProjectData projectData;

        // Open the database file in read-only mode
        auto db = SQLite::Database(projectFilePath, SQLite::OPEN_READONLY);

        // Read coordinate mapping settings
        auto queryCM = SQLite::Statement(db, "SELECT * FROM MatrixCoordinateMappings");
        while (queryCM.executeStep())
        {
            auto mappingAreaId   = queryCM.getColumn(1).getInt();                      // RecordNumber col 1
            auto venueObjectId   = queryCM.getColumn(2).getInt();                      // VenueObjectId col 2
            auto flip            = queryCM.getColumn(3).getUInt() != 0u;               // Flip col 3
            auto mappingAreaName = std::string(queryCM.getColumn(4).getString());      // Name col 4

            projectData.coordinateMappingData[mappingAreaId].venueObjectId = venueObjectId;
            projectData.coordinateMappingData[mappingAreaId].flip          = flip;
            projectData.coordinateMappingData[mappingAreaId].name          = mappingAreaName;
        }

        // Resolve venue-object transforms and read real-point corners
        for (auto& cmDataKV : projectData.coordinateMappingData)
        {
            auto queryVO = SQLite::Statement(db,
                "SELECT * FROM VenueObjects WHERE VenueObjectID==" +
                std::to_string(cmDataKV.second.venueObjectId));
            while (queryVO.executeStep())
            {
                auto originX   = queryVO.getColumn("OriginX").getDouble();
                auto originY   = queryVO.getColumn("OriginY").getDouble();
                auto originZ   = queryVO.getColumn("OriginZ").getDouble();
                auto rotationX = queryVO.getColumn("RotationX").getDouble();
                auto rotationY = queryVO.getColumn("RotationY").getDouble();
                auto rotationZ = queryVO.getColumn("RotationZ").getDouble();
                auto scaleX    = queryVO.getColumn("ScaleX").getDouble();
                auto scaleY    = queryVO.getColumn("ScaleY").getDouble();
                auto scaleZ    = queryVO.getColumn("ScaleZ").getDouble();
                auto parentVenueObject = queryVO.getColumn("ParentVenueObjectId").getInt();
                assert(parentVenueObject == 0);
                (void)rotationX; (void)rotationY; (void)scaleZ; (void)originZ; (void)parentVenueObject;

                const auto translationMatrix = AffineTransform2D::translation(originX, originY);
                const auto scalingMatrix     = AffineTransform2D::scale(scaleX, scaleY, originX, originY);
                const auto rotationMatrix    = AffineTransform2D::rotation(rotationZ * (M_PI / 180.0), originX, originY);

                auto queryVOP = SQLite::Statement(db,
                    "SELECT * FROM VenueObjectPoints WHERE VenueObjectID==" +
                    std::to_string(cmDataKV.second.venueObjectId));
                while (queryVOP.executeStep())
                {
                    auto pointIndex = queryVOP.getColumn("PointIndex").getInt(); // col 1
                    auto x = queryVOP.getColumn("X").getDouble();                // col 2
                    auto y = queryVOP.getColumn("Y").getDouble();                // col 3
                    auto z = queryVOP.getColumn("Z").getDouble();                // col 4

                    double tx = 0.0, ty = 0.0;
                    translationMatrix.applyTo(x, y, tx, ty);
                    double sx = 0.0, sy = 0.0;
                    scalingMatrix.applyTo(tx, ty, sx, sy);
                    double rx = 0.0, ry = 0.0;
                    rotationMatrix.applyTo(sx, sy, rx, ry);

                    switch (pointIndex)
                    {
                    case 0:
                        cmDataKV.second.rp1x = rx; cmDataKV.second.rp1y = ry; cmDataKV.second.rp1z = z;
                        break;
                    case 1:
                        cmDataKV.second.rp2x = rx; cmDataKV.second.rp2y = ry; cmDataKV.second.rp2z = z;
                        break;
                    case 2:
                        cmDataKV.second.rp3x = rx; cmDataKV.second.rp3y = ry; cmDataKV.second.rp3z = z;
                        break;
                    case 3:
                        cmDataKV.second.rp4x = rx; cmDataKV.second.rp4y = ry; cmDataKV.second.rp4z = z;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        // Read virtual-point corners of each mapping area
        auto queryCMP = SQLite::Statement(db, "SELECT * FROM MatrixCoordinateMappingPoints");
        while (queryCMP.executeStep())
        {
            auto mappingAreaId = queryCMP.getColumn("RecordNumber").getInt(); // col 1
            auto pIdx = queryCMP.getColumn("PointIndex").getInt();            // col 2
            auto x = queryCMP.getColumn("X").getDouble();                     // col 3
            auto y = queryCMP.getColumn("Y").getDouble();                     // col 4
            auto z = queryCMP.getColumn("Z").getDouble();                     // col 5

            if (pIdx == 0)
            {
                projectData.coordinateMappingData[mappingAreaId].vp1x = x;
                projectData.coordinateMappingData[mappingAreaId].vp1y = y;
                projectData.coordinateMappingData[mappingAreaId].vp1z = z;
            }
            else
            {
                projectData.coordinateMappingData[mappingAreaId].vp3x = x;
                projectData.coordinateMappingData[mappingAreaId].vp3y = y;
                projectData.coordinateMappingData[mappingAreaId].vp3z = z;
            }
        }

        // Read speaker output positions
        auto queryMO = SQLite::Statement(db, "SELECT * FROM MatrixOutputs");
        while (queryMO.executeStep())
        {
            auto outputNumber = queryMO.getColumn("MatrixOutput").getInt(); // col 1

            projectData.speakerPositionData[outputNumber].x   = queryMO.getColumn("CenterOfAudioX").getDouble();        // col 4
            projectData.speakerPositionData[outputNumber].y   = queryMO.getColumn("CenterOfAudioY").getDouble();        // col 5
            projectData.speakerPositionData[outputNumber].z   = queryMO.getColumn("CenterOfAudioZ").getDouble();        // col 6
            projectData.speakerPositionData[outputNumber].hor = queryMO.getColumn("AimingAngleHorizontal").getDouble(); // col 7
            projectData.speakerPositionData[outputNumber].vrt = queryMO.getColumn("AimingAngleVertical").getDouble();   // col 8
            projectData.speakerPositionData[outputNumber].rot = queryMO.getColumn("AimingAngleRotation").getDouble();   // col 9
        }

        // Read matrix input names
        auto queryMI = SQLite::Statement(db, "SELECT * FROM MatrixInputs");
        while (queryMI.executeStep())
        {
            auto inputNumber = queryMI.getColumn("MatrixInput").getInt(); // col 1

            projectData.inputNameData[inputNumber] = queryMI.getColumn("Name").getString(); // col 2
        }

        return projectData;
    }
    catch (std::exception& e)
    {
        std::cerr << __FUNCTION__ << " SQLite exception: " << e.what() << std::endl;
    }

    return {};
}

} // namespace dbpr

#endif // USE_DBPR_PROJECT_UTILS
