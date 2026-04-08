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

#ifdef USE_DBPR_PROJECT_UTILS

#include <map>
#include <string>

/**
 * @namespace dbpr
 * @brief Utilities for reading d&b audiotechnik .dbpr project files (SQLite databases).
 *
 * Entry point: `ProjectData::openAndReadProject(path)` opens a .dbpr file and returns
 * a fully populated `ProjectData` value. All three data categories — coordinate
 * mapping areas, speaker output positions, and matrix input names — are populated
 * in a single call.
 *
 * The structs also support round-trip serialisation to/from a compact string
 * representation suitable for embedding in config files or log output.
 */
namespace dbpr
{

//==============================================================================
/**
 * @struct CoordinateMappingData
 * @brief Represents one DS100 coordinate-mapping area (a MatrixCoordinateMapping row).
 *
 * Virtual points (vp1 / vp3) are the two corners of the mapping area as stored in
 * the MatrixCoordinateMappingPoints table.  Real points (rp1–rp4) are the four
 * corners of the corresponding venue object after its origin / scale / rotation
 * transform has been applied.
 *
 * `venueObjectId` is a database-internal foreign key resolved at read time and is
 * intentionally excluded from the string serialisation.
 */
struct CoordinateMappingData
{
    std::string name;
    int         venueObjectId = 0;
    bool        flip          = false;

    double vp1x = 0.0, vp1y = 0.0, vp1z = 0.0; ///< Virtual point 1 (P1 corner of mapping area).
    double vp3x = 0.0, vp3y = 0.0, vp3z = 0.0; ///< Virtual point 3 (P3 corner of mapping area).

    double rp1x = 0.0, rp1y = 0.0, rp1z = 0.0; ///< Real point 1 (VenueObject corner, index 0).
    double rp2x = 0.0, rp2y = 0.0, rp2z = 0.0; ///< Real point 2 (VenueObject corner, index 1).
    double rp3x = 0.0, rp3y = 0.0, rp3z = 0.0; ///< Real point 3 (VenueObject corner, index 2).
    double rp4x = 0.0, rp4y = 0.0, rp4z = 0.0; ///< Real point 4 (VenueObject corner, index 3).

    /** @brief Serialises to a comma-separated string (venueObjectId excluded). */
    std::string toString() const;

    /**
     * @brief Deserialises from a string produced by toString().
     * @note  Expects exactly 20 comma-separated tokens. Asserts on mismatch.
     */
    static CoordinateMappingData fromString(const std::string& s);

    /** @brief Returns true when all coordinate fields are zero (default-constructed). */
    bool isNull() const;
};
typedef std::map<int, CoordinateMappingData> CoordinateMappingDataMap;

//==============================================================================
/**
 * @struct SpeakerPositionData
 * @brief Position and aiming angles of one DS100 matrix output (speaker).
 *
 * Coordinates are in the venue coordinate system as stored in the MatrixOutputs table.
 * `hor` / `vrt` / `rot` are the aiming angles in degrees.
 */
struct SpeakerPositionData
{
    double x   = 0.0; ///< Centre-of-audio X position.
    double y   = 0.0; ///< Centre-of-audio Y position.
    double z   = 0.0; ///< Centre-of-audio Z position.
    double hor = 0.0; ///< Aiming angle — horizontal (degrees).
    double vrt = 0.0; ///< Aiming angle — vertical (degrees).
    double rot = 0.0; ///< Aiming angle — rotation (degrees).

    /** @brief Serialises to a comma-separated string. */
    std::string toString() const;

    /**
     * @brief Deserialises from a string produced by toString().
     * @note  Expects exactly 6 comma-separated tokens. Asserts on mismatch.
     */
    static SpeakerPositionData fromString(const std::string& s);

    /** @brief Returns true when all fields are zero (default-constructed). */
    bool isNull() const;
};
typedef std::map<int, SpeakerPositionData> SpeakerPositionDataMap;

/** Maps matrix-input number → display name (from the MatrixInputs table). */
typedef std::map<int, std::string> InputNameDataMap;

//==============================================================================
/**
 * @struct ProjectData
 * @brief Aggregates all data read from a .dbpr project file.
 *
 * Typical usage:
 * @code
 *   auto data = dbpr::ProjectData::openAndReadProject("/path/to/project.dbpr");
 *   if (!data.isEmpty())
 *       std::cout << data.getInfoString() << '\n';
 * @endcode
 */
struct ProjectData
{
    CoordinateMappingDataMap coordinateMappingData; ///< Keyed by mapping-area record number.
    SpeakerPositionDataMap   speakerPositionData;   ///< Keyed by matrix-output number.
    InputNameDataMap         inputNameData;          ///< Keyed by matrix-input number.

    /** @brief Returns true when both coordinate-mapping and speaker maps are empty. */
    bool isEmpty() const;

    /** @brief Returns a short human-readable summary, e.g. "4 CMP, 24 SPK". */
    std::string getInfoString() const;

    /** @brief Clears coordinate-mapping and speaker data (input names are preserved). */
    void clear();

    /** @brief Serialises to a pipe-delimited string (all three data sets). */
    std::string toString() const;

    /**
     * @brief Deserialises from a string produced by toString().
     * @note  Returns a default-constructed ProjectData on parse failure.
     */
    static ProjectData fromString(const std::string& s);

    /**
     * @brief Opens a .dbpr file and reads all project data in one call.
     *
     * Reads three tables in sequence:
     * - `MatrixCoordinateMappings` / `VenueObjects` / `VenueObjectPoints`
     *   → `coordinateMappingData`
     * - `MatrixCoordinateMappingPoints` → virtual-point corners of each mapping area
     * - `MatrixOutputs` → `speakerPositionData`
     * - `MatrixInputs`  → `inputNameData`
     *
     * @param projectFilePath  Absolute path to the .dbpr SQLite database file.
     * @return Populated ProjectData, or a default-constructed (empty) instance on error.
     */
    static ProjectData openAndReadProject(const std::string& projectFilePath);
};

} // namespace dbpr

#endif // USE_DBPR_PROJECT_UTILS
