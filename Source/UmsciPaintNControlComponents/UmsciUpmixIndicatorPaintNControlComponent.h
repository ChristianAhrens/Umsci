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

#include <TwoDFieldBase.h>

/**
 * @class UmsciUpmixIndicatorPaintNControlComponent
 * @brief The top layer of the `UmsciControlComponent` stack — renders an interactive
 *        upmix speaker ring and lets the user adjust its spatial transform.
 *
 * ## Purpose
 * Umsci is a monitoring tool for a DS100 that is being used as a upmix renderer:
 * an external upmix algorithm feeds its virtual speaker outputs into consecutive DS100
 * sound objects (starting at `m_sourceStartId`).  This component visualises where
 * those virtual speakers are currently positioned in the real room (using live DS100
 * position data when `m_liveMode` is true), and lets the user interactively adjust
 * the transform (rotation, scale, height, stretch, offset) that maps the ideal upmix
 * geometry onto the physical room.
 *
 * ## What is rendered
 * - A *floor ring* (circle or rectangle path) with one labelled dot per channel of
 *   the selected `juce::AudioChannelSet` (e.g. L/C/R/Ls/Rs for 5.0).
 * - A *height ring* (smaller, concentric) for height channels if the format includes
 *   them (e.g. Ltf/Rtf for 5.1.2).
 * - A *centre handle* (draggable) to shift the entire ring in XY.
 * - A *stretch handle* (draggable arc arrow) to adjust the `m_upmixAngleStretch`
 *   which compresses or expands the angular spacing of front/rear channels.
 * - When `m_liveMode` is true, actual DS100 source positions for the upmix channels
 *   are overlaid so the operator can see how well the current transform matches
 *   the algorithm's output.
 * - A "refit" button (top-right) that snaps the transform to best-fit the bounding
 *   cube of the connected loudspeaker system.
 *
 * ## Transform parameters
 * | Parameter        | Meaning |
 * |---|---|
 * | `m_upmixRot`     | Rotation of the ring around the Z axis (normalised 0–1 = 0–360°). |
 * | `m_upmixTrans`   | Radial scale — 1.0 means the ring radius equals the base radius. |
 * | `m_upmixHeightTrans` | Ratio of height ring to floor ring radius (0.6 default). |
 * | `m_upmixAngleStretch` | Compresses front/rear angular spread (1.0 = uniform). |
 * | `m_upmixOffsetX/Y`   | XY offset of the ring centre in units of base radius. |
 *
 * ## Drag gestures
 * - Dragging the floor ring arc → rotation (`m_upmixRot`).
 * - Dragging the height ring arc → height scale (`m_upmixHeightTrans`).
 * - Dragging the stretch handle → angular stretch (`m_upmixAngleStretch`).
 * - Dragging the centre handle → XY offset (`m_upmixOffsetX/Y`).
 * Any transform change fires `onTransformChanged` so the caller can persist the values.
 *
 * ## hitTest and touch routing
 * `hitTest()` returns true only over the interactive elements (ring arc, sub-circles,
 * handles, refit button, and the full area when the hint is flashing).  Areas of the
 * component outside those elements return false and pass touch/mouse events through to
 * lower layers.
 *
 * On iOS this means the two fingers of a pinch gesture may individually hit different
 * components (or no component at all), making JUCE-level two-touch tracking
 * unreliable.  Pinch zoom on iOS is therefore handled by a native
 * `UIPinchGestureRecognizer` at the UIKit layer (see `UmsciControlComponent`), which
 * fires independently of per-component `hitTest()` results.
 *
 * ## Timer
 * A JUCE `Timer` drives the `m_flashState` flag used to animate the live-mode
 * overlay (flashing dots when source positions are updating).
 *
 * Inherits `JUCEAppBasics::TwoDFieldBase` for the `setChannelConfiguration()` API
 * which provides the per-channel angle and label data for the selected surround format.
 *
 * @note [MANUAL CONTEXT NEEDED] Document the exact angular convention for
 *       `m_naturalFloorMaxAngleDeg` (110°) — what does it mean physically for the
 *       front/rear extent of a standard surround layout?
 */
class UmsciUpmixIndicatorPaintNControlComponent :   public UmsciPaintNControlComponentBase, public JUCEAppBasics::TwoDFieldBase, public juce::Timer
{
public:
    /** @brief The geometric shape used to draw the upmix speaker ring. */
    enum class IndicatorShape { Circle, Rectangle };
    static juce::String getShapeName(IndicatorShape shape)
    {
        switch (shape)
        {
        case IndicatorShape::Rectangle:
            return "Rectangle";
        case IndicatorShape::Circle:
        default:
            return "Circle";
        }
    }
    static IndicatorShape getShapeForName(const juce::String& name)
    {
        if (name == "Rectangle")
            return IndicatorShape::Rectangle;
        else if (name == "Circle")
            return IndicatorShape::Circle;
        
        jassertfalse; // unknown string passed as name
        return IndicatorShape::Circle;
    }

    UmsciUpmixIndicatorPaintNControlComponent();
    ~UmsciUpmixIndicatorPaintNControlComponent() override;

    //==============================================================================
    void paint(Graphics&) override;
    void resized() override;
    void setControlsSize(ControlsSize size) override;
    bool hitTest(int x, int y) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    //==============================================================================
    void timerCallback() override;

    //==============================================================================
    bool setChannelConfiguration(const juce::AudioChannelSet& channelLayout) override;

    //==============================================================================
    /**
     * @brief Provides the axis-aligned bounding cube of all loudspeaker positions.
     *
     * Used by the "refit" button to snap the upmix ring transform so the ring fits
     * neatly inside the physical speaker array.
     * @param speakersRealBoundingCube  {minX, minY, minZ, maxX, maxY, maxZ}.
     */
    void setSpeakersRealBoundingCube(const std::array<float, 6>& speakersRealBoundingCube);

    /**
     * @brief Provides live DS100 source positions for all upmix channels.
     * Only rendered when `m_liveMode` is true.
     */
    void setSourcePositions(const std::map<std::int16_t, std::array<std::float_t, 3>>& sourcePositions);
    /** @brief Updates a single source position (called on each OCP.1 notification). */
    void setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position);

    //==============================================================================
    /**
     * @brief Sets the first DS100 input channel (1-based) assigned to the upmix renderer.
     *
     * Channels from `startId` to `startId + channelCount - 1` are treated as upmix
     * inputs, where `channelCount` is the number of channels in the selected
     * `juce::AudioChannelSet`.
     */
    void setSourceStartId(int startId);
    int  getSourceStartId() const;

    //==============================================================================
    /**
     * @brief When true, actual DS100 positions for the upmix channels are overlaid
     *        on the ideal indicator ring so the operator can see alignment errors.
     */
    void setLiveMode(bool liveMode);
    bool getLiveMode() const;

    //==============================================================================
    /** @brief Sets the indicator ring geometry (circle or rectangle). */
    void setShape(IndicatorShape shape);
    IndicatorShape getShape() const;

    //==============================================================================
    /** @brief Applies all four transform parameters and triggers a prerender + repaint. */
    void setUpmixTransform(float rot, float trans, float heightTrans, float angleStretch = 1.0f);
    float getUpmixRot() const;          ///< Ring rotation (normalised 0–1 = 0–360°).
    float getUpmixTrans() const;        ///< Radial scale factor.
    float getUpmixHeightTrans() const;  ///< Height ring radius as a fraction of floor radius.
    float getUpmixAngleStretch() const; ///< Front/rear angular compression factor.

    //==============================================================================
    /** @brief Sets the ring centre offset in units of base radius. */
    void  setUpmixOffset(float x, float y);
    float getUpmixOffsetX() const;
    float getUpmixOffsetY() const;

    //==============================================================================
    /**
     * @brief Fires live-mode position callbacks and `onTransformChanged` after a
     *        programmatic transform change (e.g. from MIDI control).
     *
     * `setUpmixTransform()` and `setUpmixOffset()` deliberately do not fire callbacks
     * (to avoid side effects during config restore).  Call this immediately after a
     * programmatic update to produce the same side effects as an interactive drag.
     */
    void notifyTransformChanged();

    /**
     * @brief Checks whether the ideal ring positions diverge from the stored DS100
     *        positions and starts the flash timer if they do.
     *
     * Use this after a programmatic transform update in manual mode (live mode off)
     * to give the operator a visual cue that the device positions are out of sync and
     * need to be sent manually.  In live mode `notifyTransformChanged()` should be
     * called instead (it sends the positions immediately).
     */
    void triggerFlashCheck();

    //==============================================================================
    /**
     * @brief Fired when the user drags a source circle in live mode (pass-through
     *        from this component, analogous to `UmsciSoundobjectsPaintComponent`).
     */
    std::function<void(std::int16_t, std::array<std::float_t, 3>)> onSourcePositionChanged;

    /**
     * @brief Fired whenever any transform parameter changes via an interactive drag,
     *        so `UmsciControlComponent` can persist the new values.
     */
    std::function<void()> onTransformChanged;

private:
    //==============================================================================
    void onZoomChanged() override;

    //==============================================================================
    /**
     * @brief Holds the prerendered position of a single upmix output channel.
     *
     * Populated by `PrerenderUpmixIndicatorInBounds()` for each channel in the
     * selected `AudioChannelSet`.  Contains both the pixel position (for painting)
     * and the real-world position (for live-mode overlay comparison).
     */
    struct RenderedChannelPosition
    {
        std::int16_t            sourceId  = 0;       ///< DS100 source channel assigned to this speaker slot.
        juce::Point<float>      screenPos;            ///< Prerendered pixel position of the speaker dot.
        std::array<float, 3>    realPos   = { 0.0f, 0.0f, 0.0f }; ///< Ideal world position from the transform.
        juce::String            label;                ///< Channel label (e.g. "L", "C", "Ltf").
    };

    //==============================================================================
    /**
     * @brief Recomputes all ring geometry from the current transform parameters and
     *        stores the result in `m_renderedFloorPositions` and `m_renderedHeightPositions`.
     * Called by `onZoomChanged()`, `resized()`, and any setter that changes the transform.
     */
    void PrerenderUpmixIndicatorInBounds();
    /** @brief Toggles `m_flashState` and repaints (timer-driven, used for live-mode animation). */
    void updateFlashState();
    /** @brief Returns the pixel bounds of the "refit to speakers" button in the top-right corner. */
    juce::Rectangle<int> getRefitButtonBounds() const;

    //==============================================================================
    int m_sourceStartId = 1; ///< 1-based index of the first DS100 channel used as a upmix input.

    //==============================================================================
    std::array<float, 6>                                m_speakersRealBoundingCube; ///< {minX,minY,minZ,maxX,maxY,maxZ} of all speakers.
    std::map<std::int16_t, std::array<std::float_t, 3>> m_sourcePositions; ///< Live DS100 source positions (live mode only).

    float m_boundingFitFactor = 0.15f; ///< Inset fraction when fitting ring to speaker bounding cube.

    // Prerendered juce::Path objects updated by PrerenderUpmixIndicatorInBounds().
    juce::Path m_upmixIndicator;        ///< Floor ring (circle or rectangle).
    juce::Path m_upmixHeightIndicator;  ///< Height ring (scaled version of floor ring).
    juce::Path m_centerHandlePath;      ///< Draggable XY offset handle at ring centre.
    juce::Path m_stretchHandlePath;     ///< Draggable angular-stretch arrow handle.

    float m_upmixRot         = 0.0f;  ///< Ring rotation (normalised 0–1 = 0–360°).
    float m_upmixTrans       = 1.0f;  ///< Floor ring radial scale.
    float m_upmixHeightTrans = 0.6f;  ///< Height ring radius as fraction of floor (0.6 = 40% smaller).

    juce::Point<float>               m_upmixCenter;          ///< Pixel centre of the ring (computed during prerender).
    float                            m_subCircleRadius = 15.0f; ///< Radius of each channel dot in pixels.
    std::vector<RenderedChannelPosition> m_renderedFloorPositions;  ///< Prerendered floor channel positions.
    std::vector<RenderedChannelPosition> m_renderedHeightPositions; ///< Prerendered height channel positions.

    float             m_upmixAngleStretch       = 1.0f;   ///< Angular stretch of front/rear channels.
    float             m_naturalFloorMaxAngleDeg = 110.0f; ///< Half-angle used as reference for stretch normalisation.
    juce::Point<float> m_stretchHandlePos;    ///< Pixel position of the stretch handle dot.
    juce::Point<float> m_stretchHandleTangent; ///< Unit tangent vector along the stretch arrow direction.

    float m_upmixOffsetX = 0.0f; ///< Ring centre X offset in units of base radius.
    float m_upmixOffsetY = 0.0f; ///< Ring centre Y offset in units of base radius.
    float m_baseRadius   = 100.0f; ///< Current base radius in pixels (updated during prerender; used for drag conversion).

    // Drag state — values captured at mouseDown to allow incremental dragging.
    bool              m_draggingHeightRing    = false;
    bool              m_draggingStretchHandle = false;
    bool              m_draggingCenterHandle  = false;
    float             m_dragStartAngle        = 0.0f;
    float             m_dragStartDist         = 0.0f;
    float             m_dragStartRot          = 0.0f;
    float             m_dragStartTrans        = 0.0f;
    float             m_dragStartHeightTrans  = 0.6f;
    float             m_dragStartStretch      = 1.0f;
    float             m_dragStartOffsetX      = 0.0f;
    float             m_dragStartOffsetY      = 0.0f;
    juce::Point<float> m_dragStartMousePos;

    bool           m_flashState = false;                 ///< Toggled by timer for live-mode dot animation.
    bool           m_liveMode   = false;                 ///< When true, overlays real DS100 source positions.
    /** @brief Number of `setSourcePosition` echo-backs to absorb without calling
     *         `updateFlashState`.  Incremented by `notifyTransformChanged` for every
     *         position sent to DS100 so that the expected OCP.1 echoes do not
     *         spuriously start the flash timer. */
    int            m_inhibitFlashCount = 0;
    IndicatorShape m_shape      = IndicatorShape::Circle; ///< Current ring geometry.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciUpmixIndicatorPaintNControlComponent)
};

