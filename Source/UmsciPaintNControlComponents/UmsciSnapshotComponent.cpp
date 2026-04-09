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

#include "UmsciSnapshotComponent.h"

UmsciSnapshotComponent::UmsciSnapshotComponent()
{
    setOpaque(false);
    setInterceptsMouseClicks(true, false);

    m_storeButton = std::make_unique<juce::DrawableButton>("Store", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_storeButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_storeButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    m_storeButton->setTooltip("Store upmix indicator state");
    m_storeButton->onClick = [this] { if (onStoreRequested) onStoreRequested(); };
    addAndMakeVisible(m_storeButton.get());

    m_recallButton = std::make_unique<juce::DrawableButton>("Recall", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_recallButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_recallButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    m_recallButton->setTooltip("Recall upmix indicator state");
    m_recallButton->onClick = [this] { if (onRecallRequested) onRecallRequested(); };
    m_recallButton->setEnabled(false);
    addAndMakeVisible(m_recallButton.get());

    updateButtonImages();
}

UmsciSnapshotComponent::~UmsciSnapshotComponent() = default;

//==============================================================================

void UmsciSnapshotComponent::paint(juce::Graphics& g)
{
    const auto bounds        = getLocalBounds();
    const int  contentWidth  = s_panelWidth - s_grabStripWidth;
    const auto contentBounds = bounds.withWidth(contentWidth);
    const auto stripBounds   = bounds.withLeft(contentWidth);

    // ── Panel background ──────────────────────────────────────────────────────
    g.setColour(findColour(juce::ResizableWindow::backgroundColourId).withAlpha(0.93f));
    g.fillRoundedRectangle(bounds.toFloat().reduced(1.0f), 5.0f);

    // ── Border in highlight colour ─────────────────────────────────────────────
    g.setColour(m_highlightColour);
    g.drawRoundedRectangle(bounds.toFloat().reduced(1.0f), 5.0f, 1.5f);

    // ── Vertical separator between content and grab strip ─────────────────────
    g.setColour(m_highlightColour.withAlpha(0.35f));
    g.drawVerticalLine(contentWidth, 5.0f, static_cast<float>(getHeight()) - 5.0f);

    // ── Content ───────────────────────────────────────────────────────────────
    paintContent(g, contentBounds);

    // ── Grab strip ────────────────────────────────────────────────────────────
    paintGrabStrip(g, stripBounds);
}

void UmsciSnapshotComponent::resized()
{
    constexpr int buttonSize   = 24;
    constexpr int buttonMargin = 6;
    const int     contentWidth = s_panelWidth - s_grabStripWidth;

    m_recallButton->setBounds(contentWidth - buttonSize - buttonMargin,
                              buttonMargin,
                              buttonSize, buttonSize);

    m_storeButton->setBounds(contentWidth - buttonSize - buttonMargin,
                             getHeight() - buttonSize - buttonMargin,
                             buttonSize, buttonSize);
}

void UmsciSnapshotComponent::paintContent(juce::Graphics& g, juce::Rectangle<int> contentBounds)
{
    auto inner = contentBounds.reduced(12, 12);

    g.setColour(findColour(juce::Label::textColourId));
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.drawText("Upmix indicator snapshot", inner.removeFromTop(24), juce::Justification::centredLeft, true);

    if (m_snapshotData.has_value())
    {
        const auto& s = *m_snapshotData;
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::plain)));
        g.drawText("Rotation: "    + juce::String(s.rot,          3), inner.removeFromTop(22), juce::Justification::centredLeft, true);
        g.drawText("Scale: "       + juce::String(s.scale,        3), inner.removeFromTop(22), juce::Justification::centredLeft, true);
        g.drawText("H.Scale: "     + juce::String(s.heightScale,  3) +
                   "  A.Stretch: " + juce::String(s.angleStretch, 3), inner.removeFromTop(22), juce::Justification::centredLeft, true);
        g.drawText("Offset: "      + juce::String(s.offsetX,      3) +
                   ", "            + juce::String(s.offsetY,      3), inner.removeFromTop(22), juce::Justification::centredLeft, true);
    }
    else
    {
        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.45f));
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::plain)));
        g.drawText("No snapshot stored", inner, juce::Justification::centred, true);
    }
}

void UmsciSnapshotComponent::paintGrabStrip(juce::Graphics& g, juce::Rectangle<int> stripBounds)
{
    constexpr float pillW = 4.0f;
    constexpr float pillH = 2.0f;
    constexpr float gap   = 5.0f;

    const float cx = static_cast<float>(stripBounds.getCentreX());
    const float cy = static_cast<float>(stripBounds.getCentreY());

    g.setColour(m_highlightColour.withAlpha(0.75f));
    for (int i = -1; i <= 1; ++i)
    {
        const auto pill = juce::Rectangle<float>(
            cx - pillW * 0.5f,
            cy + static_cast<float>(i) * gap - pillH * 0.5f,
            pillW, pillH);
        g.fillRoundedRectangle(pill, pillH * 0.5f);
    }
}

void UmsciSnapshotComponent::mouseUp(const juce::MouseEvent& e)
{
    if (m_recallButton->getBounds().contains(e.getPosition()))
    {
        if (onRecallRequested) onRecallRequested();
        return;
    }

    if (m_storeButton->getBounds().contains(e.getPosition()))
    {
        if (onStoreRequested) onStoreRequested();
        return;
    }

    if (e.x >= (s_panelWidth - s_grabStripWidth) && onStateChangeRequested)
    {
        const auto newState = (m_state == PanelState::Tucked) ? PanelState::Visible : PanelState::Tucked;
        onStateChangeRequested(newState);
    }
}

//==============================================================================

void UmsciSnapshotComponent::setSnapshotData(const UpmixSnapshot& s)
{
    m_snapshotData = s;
    repaint();
}

void UmsciSnapshotComponent::clearSnapshotData()
{
    m_snapshotData = std::nullopt;
    repaint();
}

void UmsciSnapshotComponent::setHighlightColour(const juce::Colour& colour)
{
    m_highlightColour = colour;
    updateButtonImages();
    repaint();
}

void UmsciSnapshotComponent::setRecallEnabled(bool enabled)
{
    if (m_recallButton)
        m_recallButton->setEnabled(enabled);
}

void UmsciSnapshotComponent::setPanelState(PanelState state)
{
    m_state = state;
}

UmsciSnapshotComponent::PanelState UmsciSnapshotComponent::getPanelState() const
{
    return m_state;
}

//==============================================================================

void UmsciSnapshotComponent::updateButtonImages()
{
    auto storeDrawable = juce::Drawable::createFromSVG(
        *juce::XmlDocument::parse(BinaryData::variable_add_24dp_svg).get());
    storeDrawable->replaceColour(juce::Colours::black, m_highlightColour);
    m_storeButton->setImages(storeDrawable.get());

    auto recallDrawable = juce::Drawable::createFromSVG(
        *juce::XmlDocument::parse(BinaryData::variable_insert_24dp_svg).get());
    recallDrawable->replaceColour(juce::Colours::black, m_highlightColour);
    m_recallButton->setImages(recallDrawable.get());
}
