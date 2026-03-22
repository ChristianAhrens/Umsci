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


/**
 * @class UmsciPaintNControlComponentBase
 * @brief Abstract base class for all three overlaid visualisation layers in
 *        `UmsciControlComponent`.
 *
 * ## Coordinate system
 * The DS100 reports positions in a normalised *real-world* space where both X and Y
 * are in the range [0.0, 1.0] (origin at top-left when viewed from above, X = left→right,
 * Y = top→bottom relative to the room).  `m_boundsRealRef` defines which rectangle
 * of that space is currently visible in the component's pixel bounds.
 *
 * `GetPointForRealCoordinate()` and `GetRealCoordinateForPoint()` perform the two-way
 * mapping between real-world coordinates and component pixel coordinates, taking the
 * current zoom state into account.
 *
 * ## Zoom
 * A viewport zoom is maintained as a scale factor and a normalised pan offset:
 * - `m_zoomFactor` (1.0 = no zoom, clamped [0.1, 10.0]).
 * - `m_zoomPanOffset` — centre offset expressed as a fraction of the *base* content
 *   width/height so that the pan survives component resizes without drifting.
 *
 * Zoom input is accepted from multiple sources depending on platform:
 * - **Mouse-wheel** (`mouseWheelMove`) — all desktop platforms.
 * - **Trackpad / Magic Mouse pinch** (`mouseMagnify`) — macOS and iPadOS pointer devices.
 * - **Native two-finger touch pinch** (`simulatePinchZoom`) — iOS/iPadOS touchscreens.
 *   `UmsciControlComponent` attaches a `UIPinchGestureRecognizer` to the JUCE peer
 *   UIView via `parentHierarchyChanged()`, then routes each incremental-scale callback
 *   to `simulatePinchZoom()`.  This is required because JUCE 8's iOS peer routes each
 *   finger touch to whichever JUCE component passes `hitTest()` at that position, so
 *   both fingers of a pinch rarely land on the same component — making the JUCE-level
 *   fallback below unreliable.
 * - **JUCE-level two-touch fallback** (`processPinchGesture`) — for platforms where
 *   neither `mouseMagnify` nor a native gesture recognizer is available (e.g. Android).
 *   Tracks the two lowest touch indices independently; activates only when both fingers
 *   arrive at the same component.
 *
 * All paths ultimately call `applyZoomAtScreenPoint()` which keeps the focal point
 * stationary in world space.  Double-clicking any empty area resets zoom to 1.0.
 *
 * `setZoom()` silently applies new zoom values (used when synchronising siblings).
 * `resetZoom()` fires `onViewportZoomChanged` so all siblings reset together.
 *
 * ## Sibling synchronisation
 * All three layers in `UmsciControlComponent` share the same `m_boundsRealRef` and
 * must always show the same viewport.  When the user zooms on any layer, that layer
 * fires `onViewportZoomChanged`; `UmsciControlComponent` catches it and calls
 * `setZoom()` on the other two layers (which does NOT re-fire the callback).
 *
 * ## Derived class contract
 * Derived classes must override `onZoomChanged()` to re-run any pre-render pass
 * (e.g. converting world positions to pixel positions) before `repaint()` is called.
 * The base implementation just calls `repaint()`.
 */
class UmsciPaintNControlComponentBase :   public juce::Component
{
public:
    /** @brief Visual size of source/speaker icons. Multiplier accessible via `getControlsSizeMultiplier()`. */
    enum class ControlsSize { S, M, L };

    UmsciPaintNControlComponentBase();
    virtual ~UmsciPaintNControlComponentBase() override;

    //==============================================================================
    /**
     * @brief Sets the real-world rectangle that the component's pixel bounds map to.
     *
     * Must be called on all three sibling layers whenever the bounding rectangle of
     * the speaker/source data changes (e.g. after the DS100 reports speaker positions
     * that extend beyond the previous bounds).  All layers share the same reference.
     */
    void setBoundsRealRef(const juce::Rectangle<float>& boundsRealRef);

    //==============================================================================
    /** @brief Updates the icon size; derived classes may override to re-prerender. */
    virtual void setControlsSize(ControlsSize size);
    ControlsSize getControlsSize() const;
    /** @brief Returns a multiplier (e.g. 0.5 / 1.0 / 1.5) for S/M/L icon sizes. */
    float getControlsSizeMultiplier() const;

    //==============================================================================
    /**
     * @brief Silently applies zoom without firing `onViewportZoomChanged`.
     * Used by `UmsciControlComponent` to synchronise sibling layers after one of
     * them fires the callback.
     * @param factor             Zoom scale factor (clamped to [0.1, 10.0]).
     * @param normalizedPanOffset Pan offset as a fraction of base content dimensions.
     */
    void  setZoom(float factor, juce::Point<float> normalizedPanOffset = {});
    float getZoomFactor() const;
    /** @brief Resets zoom to 1.0 / no pan and fires `onViewportZoomChanged`. */
    void  resetZoom();

    /**
     * @brief Fired after every user-initiated zoom/pan change (wheel, pinch, double-click).
     * Parameters: (newFactor, newNormalisedPanOffset).
     * `UmsciControlComponent` uses this to synchronise the other two layers via `setZoom()`.
     */
    std::function<void(float, juce::Point<float>)> onViewportZoomChanged;

    /**
     * @brief Applies an incremental pinch-zoom step, as if the user performed a native
     *        pinch gesture centred at @p centre (in component-local pixel coordinates).
     *
     * Intended for use with a platform-native gesture recognizer (e.g. UIPinchGestureRecognizer
     * on iOS) that delivers incremental scale factors.  The call fires `onViewportZoomChanged`
     * so that sibling layers are synchronised via the normal zoom-sync path.
     *
     * @param scaleFactor  Incremental scale factor since the previous callback (>0, 1.0 = no change).
     * @param centre       Focal point in component-local pixel space.
     */
    void simulatePinchZoom(float scaleFactor, juce::Point<float> centre);

protected:
    //==============================================================================
    /**
     * @brief Converts a 3D real-world coordinate to a 2D screen pixel point.
     *
     * Uses `m_boundsRealRef` and the current zoom state.  The Z component of the
     * real coordinate is ignored for 2D rendering (XY plane only).
     */
    juce::Point<float> GetPointForRealCoordinate(const std::array<float, 3>& realCoordinate);

    /**
     * @brief Inverse of `GetPointForRealCoordinate` — converts a screen pixel point
     *        back to a 3D real-world coordinate (Z is set to 0).
     */
    std::array<float, 3> GetRealCoordinateForPoint(const juce::Point<float>& screenPoint);

    //==============================================================================
    /** @brief Double-click resets zoom to 1.0 via `resetZoom()`. */
    void mouseDoubleClick(const juce::MouseEvent&) override;
    /** @brief Mouse-wheel zooms about the cursor position. */
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    /** @brief Trackpad pinch-to-zoom (macOS). */
    void mouseMagnify(const juce::MouseEvent&, float scaleFactor) override;

    /**
     * @brief JUCE-level two-touch pinch-zoom fallback for platforms where neither
     *        `mouseMagnify` nor a native gesture recognizer is available.
     *
     * Tracks the two lowest `source.getIndex()` values independently.  A pinch
     * activates only when both touches are tracked by the same component — which
     * requires both fingers to pass that component's `hitTest()`.  On iOS this
     * condition is rarely met (fingers land on different JUCE components), so the
     * native `UIPinchGestureRecognizer` path via `simulatePinchZoom()` is preferred
     * there.  On Android or other platforms without a native recognizer this fallback
     * remains the primary two-finger zoom mechanism.
     *
     * Call this at the top of each derived-class `mouseDown`, `mouseDrag`, and
     * `mouseUp` override, passing the event and the appropriate `isDown`/`isUp`
     * flags.  When the method returns `true` the event has been consumed by the
     * pinch recogniser and the caller should return immediately without performing
     * any normal single-touch interaction.
     *
     * @param e       The incoming mouse event.
     * @param isDown  True when called from `mouseDown`.
     * @param isUp    True when called from `mouseUp`.
     * @return        True if the event was consumed by pinch tracking.
     */
    bool processPinchGesture(const juce::MouseEvent& e, bool isDown, bool isUp);

    /**
     * @brief Called after any zoom state change.
     *
     * Base implementation calls `repaint()`.  Derived classes should override to
     * re-run their prerender pass (world→pixel coordinate conversion) before painting,
     * then call the base or call `repaint()` directly.
     */
    virtual void onZoomChanged();

private:
    //==============================================================================
    /** @brief Returns the unzoomed content rectangle in pixel space. */
    juce::Rectangle<float> computeBaseContentBounds() const;
    /** @brief Returns the zoom-adjusted content rectangle in pixel space (used by coordinate transforms). */
    juce::Rectangle<float> getContentBounds() const;

    /**
     * @brief Core zoom logic: adjusts factor and pan offset so the world point
     *        under `screenFocus` stays fixed, then calls `onZoomChanged()` and
     *        fires `onViewportZoomChanged`.
     */
    void applyZoomAtScreenPoint(float newFactor, juce::Point<float> screenFocus);

    juce::Rectangle<float>  m_boundsRealRef;              ///< Real-world rect currently displayed.
    ControlsSize             m_controlsSize = ControlsSize::S;

    float                   m_zoomFactor     = 1.0f;      ///< Current zoom scale (1.0 = no zoom).
    juce::Point<float>      m_zoomPanOffset;               ///< Pan as fraction of base content size.

    // Two-touch pinch state (used by processPinchGesture).
    juce::Point<float>      m_pinchPos[2]         = {};
    bool                    m_pinchDown[2]         = { false, false };
    float                   m_pinchStartDistance   = 0.0f;
    float                   m_pinchStartZoom       = 1.0f;
    bool                    m_pinchActive          = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciPaintNControlComponentBase)
};

