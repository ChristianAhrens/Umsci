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

#include "DbprController.h"

DbprController::DbprController() = default;
DbprController::~DbprController() = default;

void DbprController::loadProjectFromFile(const std::string& filePath)
{
    auto data = dbpr::ProjectData::openAndReadProject(filePath);
    if (data.isEmpty())
        return; // openAndReadProject already logged the error to stderr

    m_projectData = std::move(data);
    m_hasProject  = true;

    if (onProjectLoaded)
    {
        // Fire the callback on the message thread so UI updates are safe.
        juce::MessageManager::callAsync([this] {
            if (onProjectLoaded)
                onProjectLoaded(m_projectData);
        });
    }
}

const dbpr::ProjectData& DbprController::getProjectData() const
{
    return m_projectData;
}

bool DbprController::hasProjectLoaded() const
{
    return m_hasProject;
}
