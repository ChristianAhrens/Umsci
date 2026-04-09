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

#include "UmsciDbprProjectComponent.h"

UmsciDbprProjectComponent::UmsciDbprProjectComponent()
{
    setOpaque(false);

    m_syncButton = std::make_unique<juce::DrawableButton>("Sync", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_syncButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_syncButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    m_syncButton->setTooltip("Sync project data to device");
    m_syncButton->onClick = [this] { if (onSyncRequested) onSyncRequested(); };
    addAndMakeVisible(m_syncButton.get());

    m_loadButton = std::make_unique<juce::DrawableButton>("Load", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_loadButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_loadButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    m_loadButton->setTooltip("Load dbpr project file");
    m_loadButton->onClick = [this] { if (onLoadRequested) onLoadRequested(); };
    addAndMakeVisible(m_loadButton.get());

    m_deleteButton = std::make_unique<juce::DrawableButton>("Delete", juce::DrawableButton::ButtonStyle::ImageFitted);
    m_deleteButton->setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    m_deleteButton->setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
    m_deleteButton->setTooltip("Clear loaded project");
    m_deleteButton->onClick = [this] { if (onDeleteRequested) onDeleteRequested(); };
    addAndMakeVisible(m_deleteButton.get());

    updateButtonImages();
}

UmsciDbprProjectComponent::~UmsciDbprProjectComponent() = default;

//==============================================================================

void UmsciDbprProjectComponent::setControlSize(int sizeLevel)
{
    m_sizeLevel = juce::jlimit(0, 2, sizeLevel);
    resized();
    repaint();
}

int UmsciDbprProjectComponent::getPanelWidth() const
{
    static constexpr int v[] = { 260, 325, 390 };
    return v[m_sizeLevel];
}

int UmsciDbprProjectComponent::getPanelHeight() const
{
    // 2*getPadding() + getTitleRowHeight() + 4*getContentRowHeight()
    static constexpr int v[] = { 148, 172, 202 };
    return v[m_sizeLevel];
}

int UmsciDbprProjectComponent::getGrabStripWidth() const
{
    static constexpr int v[] = { 21, 27, 32 };
    return v[m_sizeLevel];
}

int UmsciDbprProjectComponent::getButtonSize() const
{
    static constexpr int v[] = { 24, 30, 36 };
    return v[m_sizeLevel];
}

int UmsciDbprProjectComponent::getButtonMargin() const
{
    static constexpr int v[] = { 6, 8, 10 };
    return v[m_sizeLevel];
}

int UmsciDbprProjectComponent::getPadding() const
{
    static constexpr int v[] = { 12, 14, 16 };
    return v[m_sizeLevel];
}

int UmsciDbprProjectComponent::getTitleRowHeight() const
{
    static constexpr int v[] = { 27, 32, 38 };
    return v[m_sizeLevel];
}

int UmsciDbprProjectComponent::getContentRowHeight() const
{
    static constexpr int v[] = { 24, 28, 33 };
    return v[m_sizeLevel];
}

float UmsciDbprProjectComponent::getTitleFontSize() const
{
    static constexpr float v[] = { 16.5f, 20.0f, 24.0f };
    return v[m_sizeLevel];
}

float UmsciDbprProjectComponent::getContentFontSize() const
{
    static constexpr float v[] = { 16.5f, 18.0f, 22.0f };
    return v[m_sizeLevel];
}

//==============================================================================

void UmsciDbprProjectComponent::paint(juce::Graphics& g)
{
    const auto bounds        = getLocalBounds();
    const int  contentWidth  = getWidth() - getGrabStripWidth();
    const auto contentBounds = bounds.withWidth(contentWidth);
    const auto stripBounds   = bounds.withLeft(contentWidth);

    // ── Panel background ──────────────────────────────────────────────────────
    g.setColour(findColour(juce::ResizableWindow::backgroundColourId).withAlpha(0.93f));
    g.fillRoundedRectangle(bounds.toFloat().reduced(1.0f), 5.0f);

    // ── Border in highlight colour (dims during flash-off phase) ──────────────
    const auto borderOpacity = (isTimerRunning() && !m_flashState) ? 0.25f : 1.0f;
    g.setColour(m_highlightColour.withAlpha(borderOpacity));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1.0f), 5.0f, 1.5f);

    // ── Vertical separator between content and grab strip ─────────────────────
    g.setColour(m_highlightColour.withAlpha(0.35f));
    g.drawVerticalLine(contentWidth, 5.0f, static_cast<float>(getHeight()) - 5.0f);

    // ── Content ───────────────────────────────────────────────────────────────
    paintContent(g, contentBounds);

    // ── Grab strip ────────────────────────────────────────────────────────────
    paintGrabStrip(g, stripBounds);
}

void UmsciDbprProjectComponent::resized()
{
    const int buttonSize   = getButtonSize();
    const int buttonMargin = getButtonMargin();
    const int contentWidth = getWidth() - getGrabStripWidth();

    m_syncButton->setBounds(contentWidth - buttonSize - buttonMargin,
                            buttonMargin,
                            buttonSize, buttonSize);

    m_deleteButton->setBounds(contentWidth - buttonSize - buttonMargin,
                              getHeight() - buttonSize - buttonMargin,
                              buttonSize, buttonSize);

    m_loadButton->setBounds(contentWidth - buttonSize - buttonMargin,
                            getHeight() - 2 * (buttonSize + buttonMargin),
                            buttonSize, buttonSize);
}

void UmsciDbprProjectComponent::paintContent(juce::Graphics& g, juce::Rectangle<int> contentBounds)
{
    const int   pad       = getPadding();
    const int   titleRowH = getTitleRowHeight();
    const int   rowH      = getContentRowHeight();
    const float titleFont = getTitleFontSize();
    const float bodyFont  = getContentFontSize();

    auto inner = contentBounds.reduced(pad, pad);

    if (m_hasData)
    {
        g.setColour(findColour(juce::Label::textColourId));
        g.setFont(juce::Font(juce::FontOptions(titleFont, juce::Font::bold)));
        g.drawText("DBPR Project", inner.removeFromTop(titleRowH), juce::Justification::centredLeft, true);

        g.setFont(juce::Font(juce::FontOptions(bodyFont, juce::Font::plain)));

        auto cmCount  = 0;
        for (auto const& kv : m_projectData.coordinateMappingData)
            if (!kv.second.isNull()) ++cmCount;
        auto spkCount = 0;
        for (auto const& kv : m_projectData.speakerPositionData)
            if (!kv.second.isNull()) ++spkCount;
        auto soCount = 0;
        for (auto const& kv : m_projectData.matrixInputData)
            if (kv.second.isEnScene()) ++soCount;

        const auto fgCount = static_cast<int>(m_projectData.functionGroupData.size());

        const auto cmText  = juce::String(cmCount)  + " Coordinate Mapping"  + (cmCount  != 1 ? "s" : "");
        const auto spkText = juce::String(spkCount) + " Speaker position"    + (spkCount != 1 ? "s" : "");
        const auto soText  = juce::String(soCount)  + " Soundobject"         + (soCount  != 1 ? "s" : "");
        const auto fgText  = juce::String(fgCount)  + " Function group"      + (fgCount  != 1 ? "s" : "");

        g.drawText(cmText,  inner.removeFromTop(rowH), juce::Justification::centredLeft, true);
        g.drawText(spkText, inner.removeFromTop(rowH), juce::Justification::centredLeft, true);
        g.drawText(soText,  inner.removeFromTop(rowH), juce::Justification::centredLeft, true);
        g.drawText(fgText,  inner.removeFromTop(rowH), juce::Justification::centredLeft, true);

        // Sync-problem icon: centred over content, half the panel height, bright flash phase only
        if (isTimerRunning() && m_flashState && m_syncProblemDrawable)
        {
            const int iconSize = getPanelHeight() / 2;
            const auto iconBounds = juce::Rectangle<int>(contentBounds).withSizeKeepingCentre(iconSize, iconSize).toFloat();
            m_syncProblemDrawable->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
        }
    }
    else
    {
        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.45f));
        g.setFont(juce::Font(juce::FontOptions(bodyFont, juce::Font::plain)));
        g.drawText("No project loaded", inner, juce::Justification::centred, true);
    }
}

void UmsciDbprProjectComponent::paintGrabStrip(juce::Graphics& g, juce::Rectangle<int> stripBounds)
{
    // Three small rounded pill shapes centred in the grab strip, like a standard
    // drag-handle indicator.
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

void UmsciDbprProjectComponent::updateButtonImages()
{
    auto syncDrawable = juce::Drawable::createFromSVG(
        *juce::XmlDocument::parse(BinaryData::sync_arrow_up_24dp_svg).get());
    syncDrawable->replaceColour(juce::Colours::black, m_highlightColour);
    m_syncButton->setImages(syncDrawable.get());

    auto loadDrawable = juce::Drawable::createFromSVG(
        *juce::XmlDocument::parse(BinaryData::file_open_24dp_svg).get());
    loadDrawable->replaceColour(juce::Colours::black, m_highlightColour);
    m_loadButton->setImages(loadDrawable.get());

    auto deleteDrawable = juce::Drawable::createFromSVG(
        *juce::XmlDocument::parse(BinaryData::delete_24dp_svg).get());
    deleteDrawable->replaceColour(juce::Colours::black, m_highlightColour);
    m_deleteButton->setImages(deleteDrawable.get());

    m_syncProblemDrawable = juce::Drawable::createFromSVG(
        *juce::XmlDocument::parse(BinaryData::sync_problem_24dp_svg).get());
    m_syncProblemDrawable->replaceColour(juce::Colours::black, m_highlightColour);
}

//==============================================================================

void UmsciDbprProjectComponent::mouseUp(const juce::MouseEvent& e)
{
    // Buttons are click-transparent; the parent handles all interactions via
    // coordinate checks to avoid any child/parent event-routing ambiguity.
    if (m_deleteButton->getBounds().contains(e.getPosition()))
    {
        if (onDeleteRequested) onDeleteRequested();
        return;
    }

    if (m_loadButton->getBounds().contains(e.getPosition()))
    {
        if (onLoadRequested) onLoadRequested();
        return;
    }

    if (m_syncButton->getBounds().contains(e.getPosition()))
    {
        if (onSyncRequested) onSyncRequested();
        return;
    }

    // Only the grab strip (rightmost getGrabStripWidth() pixels) triggers a state toggle.
    if (e.x >= (getWidth() - getGrabStripWidth()) && onStateChangeRequested)
    {
        const auto newState = (m_state == PanelState::Tucked) ? PanelState::Visible : PanelState::Tucked;
        onStateChangeRequested(newState);
    }
}

//==============================================================================

void UmsciDbprProjectComponent::setProjectData(const dbpr::ProjectData& data)
{
    m_projectData = data;
    m_hasData     = true;
    repaint();
}

void UmsciDbprProjectComponent::clearProjectData()
{
    m_projectData = dbpr::ProjectData{};
    m_hasData     = false;
    repaint();
}

void UmsciDbprProjectComponent::setHighlightColour(const juce::Colour& colour)
{
    m_highlightColour = colour;
    updateButtonImages();
    repaint();
}

void UmsciDbprProjectComponent::setMismatchFlashing(bool mismatch)
{
    if (mismatch && !isTimerRunning())
        startTimer(500);
    else if (!mismatch && isTimerRunning())
    {
        stopTimer();
        m_flashState = false;
        repaint();
    }
}

void UmsciDbprProjectComponent::timerCallback()
{
    m_flashState = !m_flashState;
    repaint();
}

void UmsciDbprProjectComponent::setPanelState(PanelState state)
{
    m_state = state;
}

UmsciDbprProjectComponent::PanelState UmsciDbprProjectComponent::getPanelState() const
{
    return m_state;
}
