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

#include <atomic>


/**
 * @class UmsciLevelMeterPaintComponent
 * @brief Transparent overlay component that paints circular radial level meters for
 *        the upmix channels, aligned with the upmix indicator ring geometry.
 *
 * ## Overview
 * Level values come from the DS100 `MatrixInput_LevelMeterPostMute` OCP.1 object,
 * received via `setLevelValue()`.  The ring geometry (centre, sub-circle radius and
 * per-channel screen positions) is delivered by the orchestrating
 * `UmsciControlComponent` via `setRingPositions()` whenever the upmix indicator fires
 * its `onRingPositionsChanged` callback.
 *
 * ## Drawing
 * For each group of channels (floor + optionally height) a semi-transparent polygon
 * is drawn whose vertices are the current-level bar endpoints for each channel.
 * The bar length is proportional to the normalised level; the bar direction follows
 * the channel's screen position relative to the ring centre.  A thin perpendicular
 * hold line marks the 2-second peak-hold position.
 *
 * The drawing algorithm is analogous to
 * `Mema::TwoDFieldOutputComponent::paintCircularLevelIndication`.
 *
 * ## Transparency / layering
 * `hitTest()` always returns false so all mouse / touch events pass through to the
 * layers beneath.  Fill colour uses reduced alpha so the upmix indicator, loudspeakers
 * and sound objects remain visible below the meter.
 *
 * ## Hold decay
 * A `juce::Timer` fires every ~33 ms (≈ 30 fps).  Each tick checks whether any hold
 * timestamp is older than `kHoldDurationMs` (2000 ms) and resets those hold values to
 * zero before triggering a `repaint()`.
 */
class UmsciLevelMeterPaintComponent : public juce::Component, private juce::Timer
{
public:
    /** @brief Minimal channel descriptor used by `setRingPositions()`. */
    struct ChannelPosition
    {
        std::int16_t       sourceId = 0;  ///< DS100 matrix-input channel (1-based).
        juce::Point<float> screenPos;     ///< Pixel position of the channel dot, component-local.
    };

    UmsciLevelMeterPaintComponent();
    ~UmsciLevelMeterPaintComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    bool hitTest(int, int) override { return false; }

    //==============================================================================
    /**
     * @brief Delivers the current ring geometry so meter bars can be positioned correctly.
     *
     * Called by `UmsciControlComponent` whenever the upmix indicator fires
     * `onRingPositionsChanged`.  This component derives its own internal meter
     * geometry from the raw data — no indicator-specific types leak in.
     *
     * @param centre          Ring centre in component-local pixel coordinates.
     * @param subCircleRadius Radius of each channel dot on the ring (pixels).
     * @param floorPositions  Floor-ring channels in render order.
     * @param heightPositions Height-ring channels in render order (empty if no heights).
     */
    void setRingPositions(juce::Point<float>              centre,
                          float                           subCircleRadius,
                          const std::vector<ChannelPosition>& floorPositions,
                          const std::vector<ChannelPosition>& heightPositions);

    /**
     * @brief Delivers one normalised linear level value for the given source channel.
     *
     * @param sourceId        DS100 matrix-input channel number (1-based).
     * @param normalizedLevel Linear level in [0, 1].  Values above the current hold
     *                        level reset the 2-second hold timer.
     */
    void setLevelValue(std::int16_t sourceId, float normalizedLevel);

private:
    //==============================================================================
    void timerCallback() override;

    /** @brief Internal geometry for one ring of meter bars. */
    struct MeterRing
    {
        struct Channel
        {
            std::int16_t       sourceId  = 0;
            juce::Point<float> maxPoint;   ///< Point on the meter circle edge in this channel's direction.
        };
        std::vector<Channel> channels;
        float meterRadius = 0.0f;          ///< Radius of the reference circle drawn at full scale.
    };

    /** @brief Derives a `MeterRing` from raw channel positions. */
    MeterRing buildMeterRing(juce::Point<float>              centre,
                             float                           meterRadius,
                             const std::vector<ChannelPosition>& positions) const;

    /** @brief Paints one ring of radial level bars (fill polygon + hold lines). */
    void paintMeterRing(juce::Graphics& g, const MeterRing& ring) const;

    //==============================================================================
    juce::Point<float> m_centre;
    MeterRing          m_floorRing;
    MeterRing          m_heightRing;
    bool               m_hasGeometry = false;

    std::map<std::int16_t, float>  m_currentLevel;      ///< Latest level per source ID, [0,1].
    std::map<std::int16_t, float>  m_holdLevel;          ///< 2-second peak hold per source ID.
    std::map<std::int16_t, double> m_holdTimestampMs;    ///< Time of last hold update (hi-res ms).
    std::atomic<bool>              m_levelDirty { false }; ///< Set by setLevelValue(); cleared after repaint.

    static constexpr float kHoldDurationMs  = 2000.0f;  ///< Hold window in milliseconds.
    static constexpr float kHalfMeterWidth  = 2.5f;     ///< Half the perpendicular bar width in pixels.
    static constexpr float kMeterLineWidth  = 4.0f;     ///< Stroke width for per-channel level lines.
    static constexpr int   kTimerIntervalMs = 50;        ///< Timer interval ≈ 20 fps (covers the ~40 ms OCP update cycle).

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciLevelMeterPaintComponent)
};
