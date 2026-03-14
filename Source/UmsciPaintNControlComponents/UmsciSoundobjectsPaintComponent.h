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

#include "UmsciPaintNControlComponentBase.h"

#include <set>


/**
 * @class UmsciSoundobjectsPaintComponent
 * @brief The middle layer of the `UmsciControlComponent` stack — paints a filled circle
 *        for each sound object and lets the user drag them to new positions.
 *
 * ## Rendering pipeline
 * 1. `setSourcePosition()` stores the incoming 3D world position {X, Y, Z} for a
 *    sound object and calls `PrerenderSourcesInBounds()` to re-convert all positions
 *    to pixel coordinates via `GetPointForRealCoordinate()`.
 * 2. `paint()` draws a filled circle at each prerendered pixel position.
 *
 * Prerender is retriggered in `onZoomChanged()` and `resized()`.
 *
 * ## Source ID filter
 * `setSourceIdFilter()` restricts which source IDs are rendered and interactive.
 * When `UmsciControlComponent::setShowAllSources(false)` is set, only the channels
 * belonging to the upmix group are passed to the filter.
 *
 * ## Drag interaction
 * `hitTest()` returns true only over the painted circle of a source, so mouse events
 * pass through to the loudspeakers layer (below) if no source is hit.
 * On `mouseDown`, the nearest source within a hit-radius is identified and tracked as
 * `m_draggedSourceId`.  On `mouseDrag`, the new world position is computed via
 * `GetRealCoordinateForPoint()` and sent to the DS100 via `onSourcePositionChanged`.
 */
class UmsciSoundobjectsPaintComponent :   public UmsciPaintNControlComponentBase
{
public:
    UmsciSoundobjectsPaintComponent();
    ~UmsciSoundobjectsPaintComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;

    //==============================================================================
    /** @brief Returns true only when the point falls within a rendered source circle. */
    bool hitTest(int x, int y) override;
    /** @brief Identifies which source is under the cursor and begins a drag. */
    void mouseDown(const juce::MouseEvent& e) override;
    /** @brief Converts the drag position to world coordinates and fires `onSourcePositionChanged`. */
    void mouseDrag(const juce::MouseEvent& e) override;
    /** @brief Clears the active drag state. */
    void mouseUp(const juce::MouseEvent& e) override;

    //==============================================================================
    /** @brief Replaces all source positions at once (e.g. on reconnect or initial query). */
    void setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions);
    /**
     * @brief Updates a single source position.
     * @param sourceId  1-based DS100 input channel (sound object) index.
     * @param position  Normalised real-world {X, Y, Z}.
     */
    void setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position);

    //==============================================================================
    /**
     * @brief Restricts rendering and interaction to the given set of source IDs.
     * An empty set means no sources are visible.  Called by `UmsciControlComponent`
     * whenever the show-all / upmix-only mode changes.
     */
    void setSourceIdFilter(const std::set<std::int16_t>& allowedIds);

    //==============================================================================
    /**
     * @brief Fired during a drag with the new world position.
     * `UmsciControlComponent` receives this and calls `DeviceController::SetObjectValue()`
     * to push the new position to the DS100 via OCP.1.
     * Parameters: (sourceId, newPosition {X, Y, Z}).
     */
    std::function<void(std::int16_t, std::array<std::float_t, 3>)> onSourcePositionChanged;

private:
    //==============================================================================
    /** @brief Zoom changed — re-convert world positions to pixel coordinates. */
    void onZoomChanged() override;
    /** @brief Re-runs `GetPointForRealCoordinate()` for all visible sources and stores pixel positions. */
    void PrerenderSourcesInBounds();

    //==============================================================================
    std::map<std::int16_t, std::array<std::float_t, 3>> m_sourcePositions;       ///< World positions keyed by source ID.
    std::map<std::int16_t, juce::Point<int>>            m_sourceScreenPositions;  ///< Prerendered pixel positions.
    std::set<std::int16_t>                              m_sourceIdFilter;          ///< Which IDs are visible/interactive.

    std::int16_t m_draggedSourceId{ -1 }; ///< ID of the source currently being dragged, or -1.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciSoundobjectsPaintComponent)
};

