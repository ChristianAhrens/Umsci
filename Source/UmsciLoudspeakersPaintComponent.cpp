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
    : juce::Component()
{
}

UmsciLoudspeakersPaintComponent::~UmsciLoudspeakersPaintComponent()
{
}

void UmsciLoudspeakersPaintComponent::paint(juce::Graphics &g)
{
    //g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::ColourIds::backgroundColourId));

    // paint Speaker positions
    g.setColour(m_speakerDrawablesCurrentColour);
    for (auto const& speakerDrawableKV : m_speakerDrawables)
    {
        // draw speaker icons in target area
        speakerDrawableKV.second->drawWithin(g, m_speakerDrawableAreas[speakerDrawableKV.first], juce::RectanglePlacement::centred, 1.0f);
        // draw framing rect around icons, 3px larger than icon target area itself
        g.drawRect(m_speakerDrawableAreas[speakerDrawableKV.first].expanded(2.0f));
    }
}

void UmsciLoudspeakersPaintComponent::resized()
{
    PrerenderSpeakersInBounds();
}

void UmsciLoudspeakersPaintComponent::lookAndFeelChanged()
{
    juce::Component::lookAndFeelChanged();

    for (auto const& speakerDrawableKV : m_speakerDrawables)
        if (speakerDrawableKV.second)
            speakerDrawableKV.second->replaceColour(m_speakerDrawablesCurrentColour, getLookAndFeel().findColour(juce::TextButton::textColourOnId));

    m_speakerDrawablesCurrentColour = getLookAndFeel().findColour(juce::TextButton::textColourOnId);
}

void UmsciLoudspeakersPaintComponent::setBoundsRealRef(const juce::Rectangle<float>& boundsRealRef)
{
    m_boundsRealRef = boundsRealRef;
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
        {
            auto& speakerId = speakerPositionKV.first;
            auto& speakerRotNPos = speakerPositionKV.second;
            // check if speaker is set (position other than 0,0,0)
            if (speakerRotNPos.at(3) != 0.0f && speakerRotNPos.at(4) != 0.0f && speakerRotNPos.at(5) != 0.0f)
            {
                // use icon without directivity if angle is too steep (90deg +- 15deg)
                if (juce::isWithin<int>((int(std::abs(speakerRotNPos.at(1))) % 180), 90, 15))
                    m_speakerDrawables[speakerId] = Drawable::createFromSVG(*XmlDocument::parse(BinaryData::loudspeaker_vert24px_svg));
                else
                    m_speakerDrawables[speakerId] = Drawable::createFromSVG(*XmlDocument::parse(BinaryData::loudspeaker_hor24px_svg));
                auto& drawable = m_speakerDrawables.at(speakerId);
                drawable->replaceColour(Colours::black, m_speakerDrawablesCurrentColour);
                auto drawableBounds = drawable->getBounds().toFloat();
                drawable->setTransform(juce::AffineTransform::rotation(juce::degreesToRadians(speakerRotNPos.at(0)), drawableBounds.getCentreX(), drawableBounds.getCentreY()));
            }
        }

        PrerenderSpeakersInBounds();
        repaint();
    }
}

void UmsciLoudspeakersPaintComponent::PrerenderSpeakersInBounds()
{
    // Speaker positions
    for (auto const speakerPositionKV : m_speakerPositions)
    {
        auto& speakerId = speakerPositionKV.first;
        auto& speakerRotNPos = speakerPositionKV.second;
        auto& x = speakerRotNPos.at(3);
        auto& y = speakerRotNPos.at(4);
        auto& z = speakerRotNPos.at(5);
        if (x != 0.0f && y != 0.0f && z != 0.0f)
        {
            auto speakerArea = juce::Rectangle<float>(0.0f, 0.0f, 16.0f, 16.0f).withCentre(GetPointForRealCoordinate({ x, y, z }));
            m_speakerDrawableAreas[speakerId] = speakerArea;
        }
    }
}

juce::Point<float> UmsciLoudspeakersPaintComponent::GetPointForRealCoordinate(const std::array<float, 3>& realCoordinate)
{
    auto& x = realCoordinate.at(0);
    auto& y = realCoordinate.at(1);
    //auto& z = realCoordinate.at(2);

    if (m_boundsRealRef.getWidth() == 0.0f || m_boundsRealRef.getHeight() == 0.0f)
        return { 0.0f, 0.0f };

    auto relativeX = (x - m_boundsRealRef.getX()) / m_boundsRealRef.getWidth();
    auto relativeY = (y - m_boundsRealRef.getY()) / m_boundsRealRef.getHeight();

    return getLocalBounds().getRelativePoint(relativeX, relativeY).toFloat();
}

