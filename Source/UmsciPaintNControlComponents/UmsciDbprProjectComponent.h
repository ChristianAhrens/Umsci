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
class UmsciDbprProjectComponent : public juce::Component, private juce::Timer
{
public:
    /** Panel visibility state. Managed externally by the parent component. */
    enum class PanelState { Tucked, Visible };

    // ── Layout constants (pixels, S size) ───────────────────────────────────
    /** Left-edge margin from the window when the panel is fully visible (all sizes). */
    static constexpr int s_panelMargin = 8;

    //==============================================================================
    /** @brief Sets the panel size level: 0=S (default), 1=M, 2=L.
     *         Triggers resized() and repaint(). */
    void setControlSize(int sizeLevel);

    /** @brief Returns the current panel width for the active size level. */
    int getPanelWidth()     const;
    /** @brief Returns the current panel height for the active size level. */
    int getPanelHeight()    const;
    /** @brief Returns the current grab-strip width for the active size level. */
    int getGrabStripWidth() const;

    UmsciDbprProjectComponent();
    ~UmsciDbprProjectComponent() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

    //==============================================================================
    /** @brief Replaces the displayed project summary. Triggers a repaint. */
    void setProjectData(const dbpr::ProjectData& data);

    /** @brief Clears the displayed project summary and reverts to the empty state. Triggers a repaint. */
    void clearProjectData();

    /** @brief Sets the accent colour used for the panel border and grab-strip
     *         indicator. Triggers a repaint. */
    void setHighlightColour(const juce::Colour& colour);

    /**
     * @brief Starts or stops the cyclic border-flash animation.
     *
     * When @p mismatch is true, the panel's highlight-coloured border flashes
     * at 500 ms intervals to indicate that the loaded dbpr data differs from
     * the values reported by the connected device.  Passing false stops the
     * timer and restores the solid border.
     */
    void setMismatchFlashing(bool mismatch);

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

    /** @brief Fired when the user clicks the load button. The parent should
     *         open a file chooser to select a .dbpr project file. */
    std::function<void()> onLoadRequested;

    /** @brief Fired when the user clicks the delete button. The parent should
     *         clear all project data and reset the app state. */
    std::function<void()> onDeleteRequested;

    /** @brief Placeholder – fired when the user clicks the sync button. Will be
     *         used in a future version to push project data to connected devices. */
    std::function<void()> onSyncRequested;

private:
    void paintGrabStrip(juce::Graphics& g, juce::Rectangle<int> stripBounds);
    void paintContent(juce::Graphics& g, juce::Rectangle<int> contentBounds);
    void updateButtonImages();
    void timerCallback() override;

    int   getButtonSize()      const;
    int   getButtonMargin()    const;
    int   getPadding()         const;
    int   getTitleRowHeight()  const;
    int   getContentRowHeight() const;
    float getTitleFontSize()   const;
    float getContentFontSize() const;

    PanelState          m_state     = PanelState::Tucked;
    int                 m_sizeLevel = 0;
    dbpr::ProjectData   m_projectData;
    bool                m_hasData    = false;
    bool                m_flashState = false;
    juce::Colour        m_highlightColour { juce::Colours::forestgreen };

    std::unique_ptr<juce::DrawableButton> m_syncButton;
    std::unique_ptr<juce::DrawableButton> m_loadButton;
    std::unique_ptr<juce::DrawableButton> m_deleteButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciDbprProjectComponent)
};
