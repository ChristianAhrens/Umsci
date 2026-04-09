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
 * @class UmsciSnapshotComponent
 * @brief Floating side-panel for storing and recalling a single upmix transform snapshot.
 *
 * Shares the same tucked/visible side-panel geometry as `UmsciDbprProjectComponent`.
 * When tucked, only the rightmost grab strip is visible at the window's left edge.
 * The panel sits directly above the dbpr project panel.
 *
 * ## Parent responsibilities
 * The parent must:
 * 1. Size this component to exactly (`s_panelWidth` × `s_panelHeight`).
 * 2. Set the initial x-position: `-(s_panelWidth - s_grabStripWidth)` for Tucked,
 *    `s_panelMargin` for Visible.
 * 3. Subscribe to `onStateChangeRequested` and animate the x-position when it fires.
 * 4. Wire `onStoreRequested` / `onRecallRequested` to capture and apply the snapshot.
 * 5. Call `setHighlightColour()` whenever the app's accent colour changes.
 */
class UmsciSnapshotComponent : public juce::Component
{
public:
    /** Panel visibility state. Managed externally by the parent component. */
    enum class PanelState { Tucked, Visible };

    // ── Layout constants (pixels) ────────────────────────────────────────────
    static constexpr int s_panelWidth     = 260;
    static constexpr int s_panelHeight    = 148;
    static constexpr int s_grabStripWidth = 21;
    static constexpr int s_panelMargin    = 8;

    // ── Snapshot data ────────────────────────────────────────────────────────
    /** Six-parameter upmix transform snapshot. Serialises to/from a semicolon-delimited string. */
    struct UpmixSnapshot
    {
        float rot { 0.0f }, scale { 1.0f }, heightScale { 0.6f },
              angleStretch { 1.0f }, offsetX { 0.0f }, offsetY { 0.0f };

        juce::String toString() const
        {
            return "rot=" + juce::String(rot)
                + ";scale=" + juce::String(scale)
                + ";heightScale=" + juce::String(heightScale)
                + ";angleStretch=" + juce::String(angleStretch)
                + ";offsetX=" + juce::String(offsetX)
                + ";offsetY=" + juce::String(offsetY);
        }

        static UpmixSnapshot fromString(const juce::String& s)
        {
            UpmixSnapshot p;
            for (auto& token : juce::StringArray::fromTokens(s, ";", ""))
            {
                auto kv = juce::StringArray::fromTokens(token.trim(), "=", "");
                if (kv.size() != 2) continue;
                auto key = kv[0].trim();
                auto val = kv[1].trim().getFloatValue();
                if      (key == "rot")          p.rot          = val;
                else if (key == "scale")        p.scale        = val;
                else if (key == "heightScale")  p.heightScale  = val;
                else if (key == "angleStretch") p.angleStretch = val;
                else if (key == "offsetX")      p.offsetX      = val;
                else if (key == "offsetY")      p.offsetY      = val;
            }
            return p;
        }
    };

    UmsciSnapshotComponent();
    ~UmsciSnapshotComponent() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

    //==============================================================================
    /** @brief Stores and displays the given snapshot values. Triggers a repaint. */
    void setSnapshotData(const UpmixSnapshot& s);

    /** @brief Clears the stored snapshot and reverts to the empty state. Triggers a repaint. */
    void clearSnapshotData();

    /** @brief Sets the accent colour used for the panel border and buttons. Triggers a repaint. */
    void setHighlightColour(const juce::Colour& colour);

    /** @brief Enables or disables the recall button. */
    void setRecallEnabled(bool enabled);

    /** @brief Returns the currently stored snapshot, or std::nullopt if none has been stored. */
    const std::optional<UpmixSnapshot>& getSnapshotData() const { return m_snapshotData; }

    //==============================================================================
    /** @brief Records the current state without animating the position.
     *         The parent calls this *after* starting the animation. */
    void setPanelState(PanelState state);

    /** @brief Returns the currently recorded panel state. */
    PanelState getPanelState() const;

    //==============================================================================
    /** @brief Fired when the user clicks the grab strip and requests a state toggle. */
    std::function<void(PanelState newState)> onStateChangeRequested;

    /** @brief Fired when the user clicks the store button. The parent should
     *         capture the current upmix transform and call setSnapshotData(). */
    std::function<void()> onStoreRequested;

    /** @brief Fired when the user clicks the recall button. The parent should
     *         apply the stored snapshot to the control component. */
    std::function<void()> onRecallRequested;

private:
    void paintContent(juce::Graphics& g, juce::Rectangle<int> contentBounds);
    void paintGrabStrip(juce::Graphics& g, juce::Rectangle<int> stripBounds);
    void updateButtonImages();

    PanelState                   m_state = PanelState::Tucked;
    std::optional<UpmixSnapshot> m_snapshotData;
    juce::Colour                 m_highlightColour { juce::Colours::forestgreen };

    std::unique_ptr<juce::DrawableButton> m_storeButton;
    std::unique_ptr<juce::DrawableButton> m_recallButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciSnapshotComponent)
};
