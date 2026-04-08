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

#include <set>

DbprController::DbprController() = default;
DbprController::~DbprController() = default;

void DbprController::loadProjectFromFile(const std::string& filePath)
{
    auto data = dbpr::ProjectData::openAndReadProject(filePath);
    if (data.isEmpty())
        return; // openAndReadProject already logged the error to stderr

    // Validate: only a single DeviceId is supported
    {
        std::set<int> deviceIds;
        for (auto const& kv : data.matrixInputData)
            deviceIds.insert(kv.second.deviceId);

        if (deviceIds.size() > 1)
        {
            auto msg = std::string("The project contains inputs from ")
                + std::to_string(deviceIds.size())
                + " different devices. Umsci supports only a single DS100 device per project.";
            if (onProjectLoadFailed)
            {
                juce::MessageManager::callAsync([this, msg] {
                    if (onProjectLoadFailed)
                        onProjectLoadFailed(msg);
                });
            }
            return;
        }
    }

    // Validate: at least one En-Scene (InputMode=1) input must exist
    {
        int enSceneCount = 0;
        for (auto const& kv : data.matrixInputData)
            if (kv.second.isEnScene())
                enSceneCount++;

        if (enSceneCount == 0)
        {
            auto msg = std::string("The project contains no En-Scene inputs (InputMode=1). "
                "Umsci requires at least one sound object to function.");
            if (onProjectLoadFailed)
            {
                juce::MessageManager::callAsync([this, msg] {
                    if (onProjectLoadFailed)
                        onProjectLoadFailed(msg);
                });
            }
            return;
        }
    }

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

void DbprController::setProjectData(const dbpr::ProjectData& data)
{
    m_projectData = data;
    m_hasProject  = true;
}

bool DbprController::hasProjectLoaded() const
{
    return m_hasProject;
}
