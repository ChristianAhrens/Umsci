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

#include "UmsciLoudspeakersPaintComponent.h"


UmsciLoudspeakersPaintComponent::UmsciLoudspeakersPaintComponent()
    : UmsciPaintNControlComponentBase()
{
}

UmsciLoudspeakersPaintComponent::~UmsciLoudspeakersPaintComponent()
{
}

void UmsciLoudspeakersPaintComponent::paint(juce::Graphics &g)
{
    // paint Speaker positions
    g.setColour(m_speakerDrawablesCurrentColour);
    for (auto const& speakerDrawableKV : m_speakerDrawables)
    {
        // draw speaker icons in target area
        speakerDrawableKV.second->drawWithin(g, m_speakerDrawableAreas[speakerDrawableKV.first], juce::RectanglePlacement::centred, 1.0f);
        // draw framing rect around icons, proportional to drawable size
        g.drawRect(m_speakerDrawableAreas[speakerDrawableKV.first].expanded(2.0f * getControlsSizeMultiplier()));
    }
}

void UmsciLoudspeakersPaintComponent::resized()
{
    PrerenderSpeakersInBounds();
}

void UmsciLoudspeakersPaintComponent::lookAndFeelChanged()
{
    UmsciPaintNControlComponentBase::lookAndFeelChanged();

    for (auto const& speakerDrawableKV : m_speakerDrawables)
        if (speakerDrawableKV.second)
            speakerDrawableKV.second->replaceColour(m_speakerDrawablesCurrentColour, getLookAndFeel().findColour(juce::TextButton::textColourOnId));

    m_speakerDrawablesCurrentColour = getLookAndFeel().findColour(juce::TextButton::textColourOnId);
}

void UmsciLoudspeakersPaintComponent::PrerenderSpeakerDrawable(std::int16_t speakerId, const std::array<std::float_t, 6>& rotNPos)
{
    auto& hor = rotNPos.at(0);
    auto& ver = rotNPos.at(1);
    auto& rot = rotNPos.at(2);
    auto& x   = rotNPos.at(3);
    auto& y   = rotNPos.at(4);
    auto& z   = rotNPos.at(5);

    if (hor != 0.0f || ver != 0.0f || rot != 0.0f || x != 0.0f || y != 0.0f || z != 0.0f)
    {
        if (juce::isWithin<int>((int(std::abs(ver)) % 180), 90, 15))
            m_speakerDrawables[speakerId] = Drawable::createFromSVG(*XmlDocument::parse(BinaryData::loudspeaker_vert24px_svg));
        else
            m_speakerDrawables[speakerId] = Drawable::createFromSVG(*XmlDocument::parse(BinaryData::loudspeaker_hor24px_svg));
        auto& drawable = m_speakerDrawables.at(speakerId);
        drawable->replaceColour(Colours::black, m_speakerDrawablesCurrentColour);
        auto drawableBounds = drawable->getBounds().toFloat();
        // Combine all three angles for correct 2D screen rotation:
        // hor (azimuth) contributes fully when ver=0 (horizontal speaker), fades with cos(ver)
        // rot (roll) is invisible in top-down view for ver=0, but maps directly to 2D rotation when ver=90 (vertical speaker), weighted by sin(ver)
        // +90deg adjusts d&b coordinate convention to screen orientation
        auto verRad = juce::degreesToRadians(ver);
        auto angle2D = -hor * std::cos(verRad) + rot * std::sin(verRad) + 90.0f;
        drawable->setTransform(juce::AffineTransform::rotation(juce::degreesToRadians(angle2D), drawableBounds.getCentreX(), drawableBounds.getCentreY()));
    }
}

void UmsciLoudspeakersPaintComponent::setSpeakerPositions(const std::map<std::int16_t, std::array<std::float_t, 6>>& speakerPositions)
{
    if (speakerPositions.empty())
    {
        m_speakerPositions.clear();
        m_speakerDrawableAreas.clear();
        m_speakerDrawables.clear();
    }
    else
    {
        m_speakerPositions = speakerPositions;

        for (auto const speakerPositionKV : speakerPositions)
            PrerenderSpeakerDrawable(speakerPositionKV.first, speakerPositionKV.second);

        PrerenderSpeakersInBounds();
    }

    repaint();
}

void UmsciLoudspeakersPaintComponent::setSpeakerPosition(std::int16_t speakerId, const std::array<std::float_t, 6>& position)
{
    m_speakerPositions[speakerId] = position;
    PrerenderSpeakerDrawable(speakerId, position);
    m_speakerDrawableAreas[speakerId] = juce::Rectangle<float>(0.0f, 0.0f, 16.0f * getControlsSizeMultiplier(), 16.0f * getControlsSizeMultiplier())
                                            .withCentre(GetPointForRealCoordinate({ position.at(3), position.at(4), position.at(5) }));
    repaint();
}

void UmsciLoudspeakersPaintComponent::setControlsSize(ControlsSize size)
{
    UmsciPaintNControlComponentBase::setControlsSize(size);
    PrerenderSpeakersInBounds();
}

void UmsciLoudspeakersPaintComponent::PrerenderSpeakersInBounds()
{
    // Speaker positions
    for (auto const speakerPositionKV : m_speakerPositions)
    {
        auto& speakerId = speakerPositionKV.first;
        auto& speakerRotNPos = speakerPositionKV.second;
        auto& hor = speakerRotNPos.at(0);
        auto& ver = speakerRotNPos.at(1);
        auto& rot = speakerRotNPos.at(2);
        auto& x = speakerRotNPos.at(3);
        auto& y = speakerRotNPos.at(4);
        auto& z = speakerRotNPos.at(5);
        // check if speaker is set (position other than 0,0,0,0,0,0)
        if (hor != 0.0f || ver != 0.0f || rot != 0.0f || x != 0.0f || y != 0.0f || z != 0.0f)
        {
            auto speakerArea = juce::Rectangle<float>(0.0f, 0.0f, 16.0f * getControlsSizeMultiplier(), 16.0f * getControlsSizeMultiplier()).withCentre(GetPointForRealCoordinate({ x, y, z }));
            m_speakerDrawableAreas[speakerId] = speakerArea;
        }
    }
}

