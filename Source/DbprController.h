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

#include "dbprProjectUtils.h"

/**
 * @class DbprController
 * @brief Loads and holds a d&b audiotechnik .dbpr project file's data.
 *
 * Owned by MainComponent. Call loadProjectFromFile() to read a .dbpr SQLite
 * database; on success the internal ProjectData is updated and onProjectLoaded
 * is fired on the JUCE message thread so UI components can update safely.
 *
 * An empty (default-constructed) ProjectData is held until the first successful
 * load; hasProjectLoaded() distinguishes "nothing loaded yet" from "load failed".
 */
class DbprController
{
public:
    DbprController();
    ~DbprController();

    //==============================================================================
    /**
     * @brief Opens the .dbpr file at the given path and populates the internal
     *        ProjectData.  If the file cannot be read or yields no data, the
     *        existing data is left unchanged and no callback is fired.
     * @param filePath  Absolute path to a .dbpr SQLite database file.
     */
    void loadProjectFromFile(const std::string& filePath);

    /** @brief Returns the currently held project data (empty until first successful load). */
    const dbpr::ProjectData& getProjectData() const;

    /** @brief Returns true after at least one successful loadProjectFromFile() call. */
    bool hasProjectLoaded() const;

    //==============================================================================
    /**
     * @brief Fired on the JUCE message thread after each successful project load.
     * Parameter: the freshly loaded ProjectData.
     */
    std::function<void(const dbpr::ProjectData&)> onProjectLoaded;

private:
    dbpr::ProjectData   m_projectData;
    bool                m_hasProject = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DbprController)
};
