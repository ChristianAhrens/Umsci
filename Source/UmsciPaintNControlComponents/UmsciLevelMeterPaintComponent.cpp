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

#include "UmsciLevelMeterPaintComponent.h"

#include <CustomLookAndFeel.h>


UmsciLevelMeterPaintComponent::UmsciLevelMeterPaintComponent()
{
    startTimer(kTimerIntervalMs);
}

UmsciLevelMeterPaintComponent::~UmsciLevelMeterPaintComponent()
{
    stopTimer();
}

//==============================================================================
UmsciLevelMeterPaintComponent::MeterRing UmsciLevelMeterPaintComponent::buildMeterRing(
    juce::Point<float> centre, float meterRadius,
    const std::vector<ChannelPosition>& positions) const
{
    MeterRing ring;
    ring.meterRadius = meterRadius;
    for (auto const& pos : positions)
    {
        juce::Point<float> dir = pos.screenPos - centre;
        float dirLen = dir.getDistanceFromOrigin();
        if (dirLen < 1.0f)
            continue;
        MeterRing::Channel ch;
        ch.sourceId = pos.sourceId;
        ch.maxPoint = centre + (dir / dirLen) * meterRadius;
        ring.channels.push_back(ch);
    }
    return ring;
}

void UmsciLevelMeterPaintComponent::setRingPositions(
    juce::Point<float>              centre,
    float                           subCircleRadius,
    const std::vector<ChannelPosition>& floorPositions,
    const std::vector<ChannelPosition>& heightPositions)
{
    m_centre = centre;

    // The meter circle for each ring sits just inside the indicator sub-circles:
    // radius = min(distance from centre to any dot) - subCircleRadius.
    // In circle mode all dots are equidistant; in rectangle mode this picks the
    // closest edge-centre dots, giving a consistent circular meter shape.
    auto computeMeterRadius = [&](const std::vector<ChannelPosition>& positions) -> float
    {
        float minDist = std::numeric_limits<float>::max();
        for (auto const& pos : positions)
            minDist = std::min(minDist, pos.screenPos.getDistanceFrom(centre));
        return std::max(0.0f, minDist - subCircleRadius);
    };

    // Channels arrive in bed order (L, R, C, Ls, Rs, …); sort by screen angle so the
    // polygon connects them in clockwise order and never self-intersects.
    auto sortByAngle = [&](std::vector<ChannelPosition> positions) -> std::vector<ChannelPosition>
    {
        std::sort(positions.begin(), positions.end(), [&](const ChannelPosition& a, const ChannelPosition& b)
        {
            return std::atan2(a.screenPos.y - centre.y, a.screenPos.x - centre.x)
                 < std::atan2(b.screenPos.y - centre.y, b.screenPos.x - centre.x);
        });
        return positions;
    };

    m_floorRing   = buildMeterRing(centre, computeMeterRadius(floorPositions),  sortByAngle(floorPositions));
    m_heightRing  = buildMeterRing(centre, computeMeterRadius(heightPositions), sortByAngle(heightPositions));
    m_hasGeometry = !m_floorRing.channels.empty();

    repaint();
}

void UmsciLevelMeterPaintComponent::setLevelValue(std::int16_t sourceId, float normalizedLevel)
{
    normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);
    m_currentLevel[sourceId] = normalizedLevel;

    auto& hold = m_holdLevel[sourceId];
    if (normalizedLevel >= hold)
    {
        hold = normalizedLevel;
        m_holdTimestampMs[sourceId] = juce::Time::getMillisecondCounterHiRes();
    }

    m_levelDirty.store(true, std::memory_order_relaxed);
}

//==============================================================================
void UmsciLevelMeterPaintComponent::timerCallback()
{
    auto nowMs = juce::Time::getMillisecondCounterHiRes();
    bool holdChanged = false;
    for (auto& [sourceId, holdTimestamp] : m_holdTimestampMs)
    {
        auto it = m_holdLevel.find(sourceId);
        if (it != m_holdLevel.end() && it->second > 0.0f
            && nowMs - holdTimestamp > static_cast<double>(kHoldDurationMs))
        {
            it->second = 0.0f;
            holdChanged = true;
        }
    }

    if (m_levelDirty.exchange(false, std::memory_order_relaxed) || holdChanged)
        repaint();
}

void UmsciLevelMeterPaintComponent::paint(juce::Graphics& g)
{
    if (!m_hasGeometry)
        return;

    paintMeterRing(g, m_floorRing);

    if (!m_heightRing.channels.empty())
        paintMeterRing(g, m_heightRing);
}

void UmsciLevelMeterPaintComponent::paintMeterRing(juce::Graphics& g, const MeterRing& ring) const
{
    if (ring.channels.empty() || ring.meterRadius < 1.0f)
        return;

    auto rmsColour  = getLookAndFeel()
        .findColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringRmsColourId);
    auto holdColour = getLookAndFeel()
        .findColour(JUCEAppBasics::CustomLookAndFeel::ColourIds::MeteringHoldColourId);

    // --- 1. Reference circle (always visible) ---
    g.setColour(rmsColour.withAlpha(0.40f));
    g.drawEllipse(m_centre.x - ring.meterRadius, m_centre.y - ring.meterRadius,
                  ring.meterRadius * 2.0f, ring.meterRadius * 2.0f, 1.0f);

    // Pre-compute per-channel vectors:
    //   centerToMax = centre − maxPoint  (points from edge back to centre)
    //   widthOffset = 90° rotation of unit-dir × halfMeterWidth  (perpendicular bar width)
    struct ChannelVecs
    {
        juce::Point<float> centerToMax;
        juce::Point<float> widthOffset;
        bool valid = false;
    };
    std::vector<ChannelVecs> vecs(ring.channels.size());

    for (size_t i = 0; i < ring.channels.size(); ++i)
    {
        juce::Point<float> dir = ring.channels[i].maxPoint - m_centre;
        float len = dir.getDistanceFromOrigin();
        if (len < 1.0f)
            continue;
        dir /= len;
        vecs[i].centerToMax = m_centre - ring.channels[i].maxPoint;
        vecs[i].widthOffset = { -dir.y * kHalfMeterWidth, dir.x * kHalfMeterWidth };
        vecs[i].valid = true;
    }

    // --- 2. Static spoke lines: centre → maxPoint (channel direction markers) ---
    g.setColour(rmsColour.withAlpha(0.35f));
    for (size_t i = 0; i < ring.channels.size(); ++i)
    {
        if (!vecs[i].valid)
            continue;
        g.drawLine(juce::Line<float>(m_centre, ring.channels[i].maxPoint), 1.0f);
    }

    // --- 3. Level polygon (filled) at current level ---
    {
        juce::Path levelPath;
        bool started = false;
        for (size_t i = 0; i < ring.channels.size(); ++i)
        {
            if (!vecs[i].valid)
                continue;

            float level = 0.0f;
            auto it = m_currentLevel.find(ring.channels[i].sourceId);
            if (it != m_currentLevel.end())
                level = it->second;

            auto levelPoint = m_centre - (vecs[i].centerToMax * level);

            if (!started)
            {
                levelPath.startNewSubPath(levelPoint - vecs[i].widthOffset);
                started = true;
            }
            else
            {
                levelPath.lineTo(levelPoint - vecs[i].widthOffset);
            }
            levelPath.lineTo(levelPoint + vecs[i].widthOffset);
        }
        levelPath.closeSubPath();

        g.setColour(rmsColour.withAlpha(0.35f));
        g.fillPath(levelPath);
    }

    // --- 4. Per-channel spoke lines (isolated channels only) ---
    // When both angular neighbours are silent the polygon is degenerate at this channel,
    // so draw a full spoke (centre → levelPoint) to keep the reading visible.
    // When at least one neighbour is active the polygon already covers the area.
    {
        const size_t N = ring.channels.size();
        auto neighbourHasLevel = [&](size_t idx) -> bool {
            auto nit = m_currentLevel.find(ring.channels[idx].sourceId);
            return nit != m_currentLevel.end() && nit->second > 0.001f;
        };

        g.setColour(rmsColour.withAlpha(0.75f));
        for (size_t i = 0; i < N; ++i)
        {
            if (!vecs[i].valid)
                continue;

            float level = 0.0f;
            auto it = m_currentLevel.find(ring.channels[i].sourceId);
            if (it != m_currentLevel.end())
                level = it->second;

            if (level < 0.001f)
                continue;

            if (neighbourHasLevel((i + N - 1) % N) || neighbourHasLevel((i + 1) % N))
                continue;

            auto levelPoint = m_centre - (vecs[i].centerToMax * level);
            g.drawLine(juce::Line<float>(m_centre, levelPoint), kMeterLineWidth);
        }
    }

    // --- 5. Hold: stroked outline polygon or isolated cap ---
    // Contiguous runs of hold-active channels are each drawn as a separate stroked path.
    // A full-ring run (all channels active) is closed into a polygon; partial runs are
    // left open so no diagonal closing segment crosses the silent gap.
    // Isolated channels (both angular neighbours silent) get a perpendicular cap tick.
    {
        const size_t N = ring.channels.size();

        auto holdOf = [&](size_t idx) -> float {
            auto hit = m_holdLevel.find(ring.channels[idx].sourceId);
            return (hit != m_holdLevel.end()) ? hit->second : 0.0f;
        };

        g.setColour(holdColour.darker(0.3f).withAlpha(0.90f));

        // Find the first channel after a gap so wrap-around runs are not split.
        size_t startIdx = 0;
        for (size_t k = 0; k < N; ++k)
        {
            if (holdOf(k) > 0.01f && holdOf((k + N - 1) % N) <= 0.01f)
            {
                startIdx = k;
                break;
            }
        }

        juce::Path holdPath;
        bool inRun = false;
        int runChannelCount = 0;

        auto flushRun = [&]()
        {
            if (inRun)
            {
                if (runChannelCount == static_cast<int>(N))
                    holdPath.closeSubPath();
                else
                {
                    // Partial run: route the closing edge through the origin so the
                    // polygon closes cleanly at the centre rather than leaving an open path.
                    holdPath.lineTo(m_centre);
                    holdPath.closeSubPath();
                }
                g.strokePath(holdPath, juce::PathStrokeType(1.5f));
                holdPath.clear();
                inRun = false;
                runChannelCount = 0;
            }
        };

        for (size_t ki = 0; ki < N; ++ki)
        {
            const size_t i = (startIdx + ki) % N;
            if (!vecs[i].valid) { flushRun(); continue; }

            const float hold = holdOf(i);
            const bool prevHasHold = holdOf((i + N - 1) % N) > 0.01f;
            const bool nextHasHold = holdOf((i + 1) % N) > 0.01f;

            if (hold < 0.01f) { flushRun(); continue; }

            if (!prevHasHold && !nextHasHold)
            {
                // Isolated: perpendicular cap tick
                flushRun();
                auto holdPoint = m_centre - (vecs[i].centerToMax * hold);
                g.drawLine(juce::Line<float>(
                    holdPoint - vecs[i].widthOffset,
                    holdPoint + vecs[i].widthOffset), 1.5f);
                continue;
            }

            auto holdPoint = m_centre - (vecs[i].centerToMax * hold);
            if (!inRun)
            {
                holdPath.startNewSubPath(holdPoint - vecs[i].widthOffset);
                inRun = true;
            }
            else
            {
                holdPath.lineTo(holdPoint - vecs[i].widthOffset);
            }
            holdPath.lineTo(holdPoint + vecs[i].widthOffset);
            ++runChannelCount;
        }
        flushRun();
    }
}
