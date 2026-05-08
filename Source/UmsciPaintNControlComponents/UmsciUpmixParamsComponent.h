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
 * @class UmsciUpmixParamsComponent
 * @brief Floating left-side panel for batch-controlling Positioning_SourceSpread and
 *        Positioning_SourceDelayMode across all DS100 channels currently used by the
 *        upmix indicator.
 *
 * Sits directly above the snapshot panel on the left edge of the screen.  When tucked,
 * only the grab strip is visible.  Slides in/out via `onStateChangeRequested`.
 *
 * ## Controls
 * - **Spread slider** (0–1): sets `Positioning_SourceSpread` on all monitored channels.
 * - **Delay mode radio** (Off / Tight / Full): sets `Positioning_SourceDelayMode`
 *   (0 / 1 / 2) on all monitored channels.
 *
 * ## Mismatch detection
 * The panel subscribes to per-source device feedback via `setSourceSpreadDeviceValue()`
 * and `setSourceDelayModeDeviceValue()`.  Whenever any monitored channel's device value
 * differs from the aggregate control value, a `sync_problem` icon flashes over the panel
 * (same pattern as `UmsciDbprProjectComponent`).  Flashing stops once all channels match.
 * Call `clearDeviceValues()` on disconnect to suppress spurious flashing before the next
 * connection.
 */
class UmsciUpmixParamsComponent : public juce::Component, public juce::Timer
{
public:
    /** Panel visibility state — managed externally by MainComponent. */
    enum class PanelState { Tucked, Visible };

    /** Left-edge margin from the window when the panel is fully visible (all sizes). */
    static constexpr int s_panelMargin = 8;

    UmsciUpmixParamsComponent();
    ~UmsciUpmixParamsComponent() override;

    // ── Size management ──────────────────────────────────────────────────────
    /** @brief Sets size level 0=S (default), 1=M, 2=L. Triggers resized() + repaint(). */
    void setControlSize(int sizeLevel);
    int  getPanelWidth()     const;
    int  getPanelHeight()    const;
    int  getGrabStripWidth() const;

    // ── Panel state ──────────────────────────────────────────────────────────
    void       setPanelState(PanelState state);
    PanelState getPanelState() const;

    // ── Aggregate control values ─────────────────────────────────────────────
    /** @brief Sets spread (0–1) without firing `onSpreadChanged`. */
    void  setSpread(float spread);
    float getSpread() const;

    /** @brief Sets delay mode (0=Off, 1=Tight, 2=Full) without firing `onDelayModeChanged`. */
    void setDelayMode(int mode);
    int  getDelayMode() const;

    // ── Per-source device feedback ───────────────────────────────────────────
    /** @brief Called when a Positioning_SourceSpread echo arrives for `sourceId`. */
    void setSourceSpreadDeviceValue(std::int16_t sourceId, float spread);
    /** @brief Called when a Positioning_SourceDelayMode echo arrives for `sourceId`. */
    void setSourceDelayModeDeviceValue(std::int16_t sourceId, std::uint16_t mode);
    /** @brief Updates the set of DS100 channels to monitor for mismatch. */
    void setMonitoredSourceIds(const std::vector<std::int16_t>& ids);
    /** @brief Clears stored device values and stops flashing. Call on disconnect. */
    void clearDeviceValues();

    // ── Styling ──────────────────────────────────────────────────────────────
    void setHighlightColour(const juce::Colour& colour);

    // ── JUCE Component overrides ─────────────────────────────────────────────
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;
    void timerCallback() override;

    // ── Callbacks ────────────────────────────────────────────────────────────
    /** @brief Fired when the grab strip is clicked to request a state toggle. */
    std::function<void(PanelState)> onStateChangeRequested;
    /** @brief Fired when the user changes the spread slider (value 0–1). */
    std::function<void(float)>      onSpreadChanged;
    /** @brief Fired when the user clicks a delay mode button (0=Off, 1=Tight, 2=Full). */
    std::function<void(int)>        onDelayModeChanged;

private:
    void paintContent(juce::Graphics& g, juce::Rectangle<int> contentBounds);
    void paintGrabStrip(juce::Graphics& g, juce::Rectangle<int> stripBounds);
    void checkMismatch();
    void updateDelayModeButtonStates();
    void setChildrenMouseInterception(bool intercepts);

    int   getPadding()          const;
    int   getTitleRowHeight()   const;
    int   getContentRowHeight() const;
    float getTitleFontSize()    const;
    float getContentFontSize()  const;

    PanelState   m_state     = PanelState::Tucked;
    int          m_sizeLevel = 0;
    juce::Colour m_highlightColour { juce::Colours::forestgreen };
    bool         m_flashState = false;

    float m_spread    = 0.5f;
    int   m_delayMode = 0;

    std::vector<std::int16_t>             m_monitoredSourceIds;
    std::map<std::int16_t, float>         m_deviceSpread;
    std::map<std::int16_t, std::uint16_t> m_deviceDelayMode;

    bool m_suppressCallbacks = false;

    std::unique_ptr<juce::Slider>     m_spreadSlider;
    std::unique_ptr<juce::TextButton> m_delayModeOff;
    std::unique_ptr<juce::TextButton> m_delayModeTight;
    std::unique_ptr<juce::TextButton> m_delayModeFull;
    std::unique_ptr<juce::Drawable>   m_syncProblemDrawable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciUpmixParamsComponent)
};
