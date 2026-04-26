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


/**
 * @class UmsciLoudspeakersPaintComponent
 * @brief The bottom layer of the `UmsciControlComponent` stack — paints a speaker
 *        SVG icon at each loudspeaker's reported real-world position.
 *
 * ## Rendering pipeline
 * 1. `setSpeakerPosition()` stores the incoming 6-component position {horizontal angle,
 *    vertical angle, dispersion-axis roll, X, Y, Z} and calls `PrerenderSpeakerDrawable()`
 *    to create a rotated `juce::Drawable` scaled to the current pixel bounds.
 * 2. `PrerenderSpeakersInBounds()` converts all world positions to pixel coordinates
 *    (via `GetPointForRealCoordinate()`) and stores the target `Rectangle<float>` for
 *    each speaker icon in `m_speakerDrawableAreas`.
 * 3. `paint()` iterates the drawables and draws each one at its prerendered area.
 *
 * Prerender is retriggered in `onZoomChanged()` (zoom changed), `resized()` (component
 * resized), and `lookAndFeelChanged()` / `setControlsSize()` (appearance changed).
 *
 * This component is display-only — it does not handle any mouse interaction.
 * Interaction is handled by the `UmsciSoundobjectsPaintComponent` layer above.
 *
 * @note [MANUAL CONTEXT NEEDED] Describe which SVG asset is used for the speaker icon
 *       and where it originates (BinaryData resource name, source file).
 */
class UmsciLoudspeakersPaintComponent :   public UmsciPaintNControlComponentBase
{
public:
    UmsciLoudspeakersPaintComponent();
    ~UmsciLoudspeakersPaintComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;
    /** @brief Re-prerender all speaker drawables in the new look-and-feel colour. */
    void lookAndFeelChanged() override;
    /** @brief Re-prerender all speaker drawables at the new icon size. */
    void setControlsSize(ControlsSize size) override;

    //==============================================================================
    /** @brief Replaces all speaker positions at once (e.g. on reconnect). */
    void setSpeakerPositions(const std::map<std::int16_t, std::array<std::float_t, 6>>& speakerPositions);
    /**
     * @brief Updates a single speaker position and re-prerenders its drawable.
     * @param speakerId 1-based DS100 output channel index.
     * @param position  {X, Y, Z, horizontal angle, vertical angle, rotation}.
     */
    void setSpeakerPosition(std::int16_t speakerId, const std::array<std::float_t, 6>& position);

private:
    //==============================================================================
    /** @brief Zoom changed — re-convert world positions to pixel coords and repaint. */
    void onZoomChanged() override;
    /**
     * @brief Creates (or updates) the rotated `juce::Drawable` for one speaker.
     * The SVG template is recoloured to match the current look-and-feel and scaled
     * to the current `ControlsSize`.
     */
    void PrerenderSpeakerDrawable(std::int16_t speakerId, const std::array<std::float_t, 6>& rotNPos);
    /** @brief Converts all stored world positions to pixel rectangles ready for painting. */
    void PrerenderSpeakersInBounds();

    //==============================================================================
    juce::Colour                                             m_speakerDrawablesCurrentColour; ///< Colour used when last prerendering.
    std::map<std::int16_t, std::array<std::float_t, 6>>     m_speakerPositions;    ///< World positions keyed by speaker ID.
    std::map<std::int16_t, std::unique_ptr<juce::Drawable>> m_speakerDrawables;    ///< Prerendered (rotated, coloured) SVG drawables.
    std::map<std::int16_t, juce::Rectangle<float>>          m_speakerDrawableAreas; ///< Prerendered pixel rects keyed by speaker ID.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciLoudspeakersPaintComponent)
};

