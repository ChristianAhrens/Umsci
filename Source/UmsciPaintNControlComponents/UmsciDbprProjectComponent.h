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

#include "../dbprProjectUtils.h"

/**
 * @class UmsciDbprProjectComponent
 * @brief Floating side-panel that shows a condensed summary of a loaded .dbpr project.
 *
 * ## States
 * The panel has two states managed by the *parent* component (MainComponent):
 *
 * | State   | Description |
 * |---------|-------------|
 * | Tucked  | Panel is pushed left off-screen. Only the rightmost `grabStripWidth`
 * |         | pixels (the grab strip) remain visible at the window's left edge. |
 * | Visible | Full panel is on-screen, left edge at `panelMargin` from the window left. |
 *
 * ## Parent responsibilities
 * The parent must:
 * 1. Size this component to exactly (`panelWidth` × `panelHeight`).
 * 2. Set the initial x-position: `-(panelWidth - grabStripWidth)` for Tucked,
 *    `panelMargin` for Visible.
 * 3. Subscribe to `onStateChangeRequested` and animate/snap the component's
 *    x-position when the callback fires.
 * 4. Call `setHighlightColour()` whenever the app's accent colour changes.
 *
 * ## Grab-strip geometry (x = 0 is the panel's own left edge)
 * ```
 * +-------- contentWidth --------+-- grabStripWidth --+
 * | DBPR Project                 |        |||          |
 * | 4 Coordinate Mappings        |        |||          |
 * | 52 Speakers                  |        |||          |
 * +------------------------------+--------------------+
 * ```
 * When Tucked the parent positions the component so that only the grab strip is
 * on-screen (at the window's left edge).  Clicking anywhere in the grab strip
 * fires `onStateChangeRequested`.
 */
class UmsciDbprProjectComponent : public juce::Component
{
public:
    /** Panel visibility state. Managed externally by the parent component. */
    enum class PanelState { Tucked, Visible };

    // ── Layout constants (pixels) ────────────────────────────────────────────
    /** Total component width = contentWidth + grabStripWidth. */
    static constexpr int panelWidth     = 260;
    /** Fixed component height. */
    static constexpr int panelHeight    = 148;
    /** Width of the grab-handle strip on the panel's right edge. */
    static constexpr int grabStripWidth = 21;
    /** Left-edge margin from the window when the panel is fully visible. */
    static constexpr int panelMargin    = 8;

    UmsciDbprProjectComponent();
    ~UmsciDbprProjectComponent() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& e) override;

    //==============================================================================
    /** @brief Replaces the displayed project summary. Triggers a repaint. */
    void setProjectData(const dbpr::ProjectData& data);

    /** @brief Sets the accent colour used for the panel border and grab-strip
     *         indicator. Triggers a repaint. */
    void setHighlightColour(const juce::Colour& colour);

    //==============================================================================
    /** @brief Records the current state without animating the position.
     *         The parent calls this *after* starting the animation. */
    void setPanelState(PanelState state);

    /** @brief Returns the currently recorded panel state. */
    PanelState getPanelState() const;

    //==============================================================================
    /**
     * @brief Fired when the user clicks the grab strip and requests a state toggle.
     *
     * The parent must respond by:
     * - Calling `setPanelState(newState)` on this component.
     * - Animating the component's x-position to
     *   `-(panelWidth - grabStripWidth)` (Tucked) or `panelMargin` (Visible).
     */
    std::function<void(PanelState newState)> onStateChangeRequested;

private:
    void paintGrabStrip(juce::Graphics& g, juce::Rectangle<int> stripBounds);
    void paintContent(juce::Graphics& g, juce::Rectangle<int> contentBounds);

    PanelState          m_state = PanelState::Tucked;
    dbpr::ProjectData   m_projectData;
    bool                m_hasData = false;
    juce::Colour        m_highlightColour { juce::Colours::forestgreen };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciDbprProjectComponent)
};
