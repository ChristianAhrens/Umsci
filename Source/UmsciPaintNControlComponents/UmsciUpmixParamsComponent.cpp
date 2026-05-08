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

#include "UmsciUpmixParamsComponent.h"

UmsciUpmixParamsComponent::UmsciUpmixParamsComponent()
{
    setOpaque(false);
    setInterceptsMouseClicks(true, true);

    m_spreadSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                    juce::Slider::NoTextBox);
    m_spreadSlider->setRange(0.0, 1.0, 0.0);
    m_spreadSlider->setValue(static_cast<double>(m_spread), juce::dontSendNotification);
    m_spreadSlider->setColour(juce::Slider::thumbColourId, m_highlightColour);
    m_spreadSlider->setColour(juce::Slider::trackColourId, m_highlightColour.withAlpha(0.5f));
    m_spreadSlider->onValueChange = [this] {
        if (m_suppressCallbacks) return;
        m_spread = static_cast<float>(m_spreadSlider->getValue());
        checkMismatch();
        if (onSpreadChanged) onSpreadChanged(m_spread);
    };
    addAndMakeVisible(m_spreadSlider.get());

    auto initButton = [this](std::unique_ptr<juce::TextButton>& btn, const juce::String& label) {
        btn = std::make_unique<juce::TextButton>(label);
        btn->setClickingTogglesState(false);
        btn->setColour(juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        btn->setColour(juce::TextButton::buttonOnColourId, m_highlightColour);
        addAndMakeVisible(btn.get());
    };
    initButton(m_delayModeOff,   "Off");
    initButton(m_delayModeTight, "Tight");
    initButton(m_delayModeFull,  "Full");

    m_delayModeOff->onClick   = [this] {
        if (m_suppressCallbacks) return;
        m_delayMode = 0;
        updateDelayModeButtonStates();
        checkMismatch();
        if (onDelayModeChanged) onDelayModeChanged(0);
    };
    m_delayModeTight->onClick = [this] {
        if (m_suppressCallbacks) return;
        m_delayMode = 1;
        updateDelayModeButtonStates();
        checkMismatch();
        if (onDelayModeChanged) onDelayModeChanged(1);
    };
    m_delayModeFull->onClick  = [this] {
        if (m_suppressCallbacks) return;
        m_delayMode = 2;
        updateDelayModeButtonStates();
        checkMismatch();
        if (onDelayModeChanged) onDelayModeChanged(2);
    };
    updateDelayModeButtonStates();

    if (auto xml = juce::XmlDocument::parse(BinaryData::sync_problem_24dp_svg))
    {
        m_syncProblemDrawable = juce::Drawable::createFromSVG(*xml);
        m_syncProblemDrawable->replaceColour(juce::Colours::black, m_highlightColour);
    }
}

UmsciUpmixParamsComponent::~UmsciUpmixParamsComponent() = default;

//==============================================================================

void UmsciUpmixParamsComponent::setControlSize(int sizeLevel)
{
    m_sizeLevel = juce::jlimit(0, 2, sizeLevel);
    resized();
    repaint();
}

int UmsciUpmixParamsComponent::getPanelWidth() const
{
    static constexpr int v[] = { 260, 325, 390 };
    return v[m_sizeLevel];
}

int UmsciUpmixParamsComponent::getPanelHeight() const
{
    // 2*getPadding() + getTitleRowHeight() + 2*getContentRowHeight() + 2*(getContentRowHeight()/4)
    static constexpr int v[] = { 111, 130, 152 };
    return v[m_sizeLevel];
}

int UmsciUpmixParamsComponent::getGrabStripWidth() const
{
    static constexpr int v[] = { 21, 27, 32 };
    return v[m_sizeLevel];
}

int UmsciUpmixParamsComponent::getPadding() const
{
    static constexpr int v[] = { 12, 14, 16 };
    return v[m_sizeLevel];
}

int UmsciUpmixParamsComponent::getTitleRowHeight() const
{
    static constexpr int v[] = { 27, 32, 38 };
    return v[m_sizeLevel];
}

int UmsciUpmixParamsComponent::getContentRowHeight() const
{
    static constexpr int v[] = { 24, 28, 33 };
    return v[m_sizeLevel];
}

float UmsciUpmixParamsComponent::getTitleFontSize() const
{
    static constexpr float v[] = { 16.5f, 20.0f, 24.0f };
    return v[m_sizeLevel];
}

float UmsciUpmixParamsComponent::getContentFontSize() const
{
    static constexpr float v[] = { 16.5f, 18.0f, 22.0f };
    return v[m_sizeLevel];
}

//==============================================================================

void UmsciUpmixParamsComponent::setPanelState(PanelState state) { m_state = state; }
UmsciUpmixParamsComponent::PanelState UmsciUpmixParamsComponent::getPanelState() const { return m_state; }

//==============================================================================

void UmsciUpmixParamsComponent::setSpread(float spread)
{
    m_spread = juce::jlimit(0.0f, 1.0f, spread);
    m_suppressCallbacks = true;
    if (m_spreadSlider)
        m_spreadSlider->setValue(static_cast<double>(m_spread), juce::dontSendNotification);
    m_suppressCallbacks = false;
}

float UmsciUpmixParamsComponent::getSpread() const { return m_spread; }

void UmsciUpmixParamsComponent::setDelayMode(int mode)
{
    m_delayMode = juce::jlimit(0, 2, mode);
    m_suppressCallbacks = true;
    updateDelayModeButtonStates();
    m_suppressCallbacks = false;
}

int UmsciUpmixParamsComponent::getDelayMode() const { return m_delayMode; }

//==============================================================================

void UmsciUpmixParamsComponent::setSourceSpreadDeviceValue(std::int16_t sourceId, float spread)
{
    m_deviceSpread[sourceId] = spread;
    checkMismatch();
}

void UmsciUpmixParamsComponent::setSourceDelayModeDeviceValue(std::int16_t sourceId, std::uint16_t mode)
{
    m_deviceDelayMode[sourceId] = mode;
    checkMismatch();
}

void UmsciUpmixParamsComponent::setMonitoredSourceIds(const std::vector<std::int16_t>& ids)
{
    m_monitoredSourceIds = ids;
    // Re-run mismatch check if we already have device data (e.g. IDs resolved after database complete).
    if (!m_deviceSpread.empty() || !m_deviceDelayMode.empty())
        checkMismatch();
}

void UmsciUpmixParamsComponent::clearDeviceValues()
{
    m_deviceSpread.clear();
    m_deviceDelayMode.clear();
    stopTimer();
    m_flashState = false;
    setAlpha(1.0f);
    setChildrenMouseInterception(true);
    repaint();
}

//==============================================================================

void UmsciUpmixParamsComponent::setHighlightColour(const juce::Colour& colour)
{
    m_highlightColour = colour;

    if (m_spreadSlider)
    {
        m_spreadSlider->setColour(juce::Slider::thumbColourId, colour);
        m_spreadSlider->setColour(juce::Slider::trackColourId, colour.withAlpha(0.5f));
    }
    for (auto* btn : { m_delayModeOff.get(), m_delayModeTight.get(), m_delayModeFull.get() })
        if (btn) btn->setColour(juce::TextButton::buttonOnColourId, colour);

    if (auto xml = juce::XmlDocument::parse(BinaryData::sync_problem_24dp_svg))
    {
        m_syncProblemDrawable = juce::Drawable::createFromSVG(*xml);
        m_syncProblemDrawable->replaceColour(juce::Colours::black, colour);
    }

    repaint();
}

//==============================================================================

void UmsciUpmixParamsComponent::paint(juce::Graphics& g)
{
    const auto bounds        = getLocalBounds();
    const int  contentWidth  = getWidth() - getGrabStripWidth();
    const auto contentBounds = bounds.withWidth(contentWidth);
    const auto stripBounds   = bounds.withLeft(contentWidth);

    g.setColour(findColour(juce::ResizableWindow::backgroundColourId).withAlpha(0.93f));
    g.fillRoundedRectangle(bounds.toFloat().reduced(1.0f), 5.0f);

    g.setColour(m_highlightColour);
    g.drawRoundedRectangle(bounds.toFloat().reduced(1.0f), 5.0f, 1.5f);

    g.setColour(m_highlightColour.withAlpha(0.35f));
    g.drawVerticalLine(contentWidth, 5.0f, static_cast<float>(getHeight()) - 5.0f);

    paintContent(g, contentBounds);
    paintGrabStrip(g, stripBounds);
}

void UmsciUpmixParamsComponent::paintOverChildren(juce::Graphics& g)
{
    if (isTimerRunning() && m_flashState && m_syncProblemDrawable)
    {
        const int  contentWidth  = getWidth() - getGrabStripWidth();
        const auto contentBounds = getLocalBounds().withWidth(contentWidth);
        auto iconSize = static_cast<float>(std::min(contentBounds.getWidth(), contentBounds.getHeight())) * 0.4f;
        auto iconBounds = juce::Rectangle<float>(iconSize, iconSize)
            .withCentre(contentBounds.getCentre().toFloat());
        m_syncProblemDrawable->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
    }
}

void UmsciUpmixParamsComponent::resized()
{
    const int pad      = getPadding();
    const int titleH   = getTitleRowHeight();
    const int rowH     = getContentRowHeight();
    const int gap      = rowH / 4;
    const int contentW = getWidth() - getGrabStripWidth();
    const int innerW   = contentW - 2 * pad;
    const int innerX   = pad;
    const int labelW   = innerW / 4;

    // Row 1: "Spread" label (painted) + slider — gap below title
    m_spreadSlider->setBounds(innerX + labelW, pad + titleH + gap, innerW - labelW, rowH);

    // Row 2: "Delay:" label (painted) + three equal-width buttons — gap below row 1
    const int btnAreaW = innerW - labelW;
    const int btnW     = btnAreaW / 3;
    const int btnX     = innerX + labelW;
    const int btnY     = pad + titleH + gap + rowH + gap;
    m_delayModeOff->setBounds(btnX,            btnY, btnW,                rowH);
    m_delayModeTight->setBounds(btnX + btnW,    btnY, btnW,                rowH);
    m_delayModeFull->setBounds(btnX + 2 * btnW, btnY, btnAreaW - 2 * btnW, rowH);
}

void UmsciUpmixParamsComponent::paintContent(juce::Graphics& g, juce::Rectangle<int> contentBounds)
{
    const int   pad       = getPadding();
    const int   titleH    = getTitleRowHeight();
    const int   rowH      = getContentRowHeight();
    const int   gap       = rowH / 4;
    const float titleFont = getTitleFontSize();
    const float bodyFont  = getContentFontSize();

    auto inner   = contentBounds.reduced(pad, pad);
    const int labelW = inner.getWidth() / 4;

    g.setColour(findColour(juce::Label::textColourId));
    g.setFont(juce::Font(juce::FontOptions(titleFont, juce::Font::bold)));
    g.drawText("Upmix ch. parameters", inner.removeFromTop(titleH), juce::Justification::centredLeft, true);

    inner.removeFromTop(gap);

    g.setFont(juce::Font(juce::FontOptions(bodyFont, juce::Font::plain)));

    auto spreadRow = inner.removeFromTop(rowH);
    g.drawText("Spread", spreadRow.removeFromLeft(labelW), juce::Justification::centredLeft, true);

    inner.removeFromTop(gap);

    auto delayRow = inner.removeFromTop(rowH);
    g.drawText("Delay:", delayRow.removeFromLeft(labelW), juce::Justification::centredLeft, true);
}

void UmsciUpmixParamsComponent::paintGrabStrip(juce::Graphics& g, juce::Rectangle<int> stripBounds)
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

void UmsciUpmixParamsComponent::mouseUp(const juce::MouseEvent& e)
{
    const int grabX = getWidth() - getGrabStripWidth();

    if (e.x >= grabX)
    {
        // Grab strip toggles panel state (always reachable).
        if (onStateChangeRequested)
        {
            const auto newState = (m_state == PanelState::Tucked) ? PanelState::Visible : PanelState::Tucked;
            onStateChangeRequested(newState);
        }
    }
    else if (isTimerRunning())
    {
        // Content area click while out of sync: push current aggregate values to device.
        // Children are blocked (setInterceptsMouseClicks false) so every click lands here.
        if (onSpreadChanged)    onSpreadChanged(m_spread);
        if (onDelayModeChanged) onDelayModeChanged(m_delayMode);
    }
}

void UmsciUpmixParamsComponent::timerCallback()
{
    m_flashState = !m_flashState;
    setAlpha(m_flashState ? 1.0f : 0.25f);
    repaint();
}

//==============================================================================

void UmsciUpmixParamsComponent::checkMismatch()
{
    constexpr float kSpreadTol = 0.01f;
    bool mismatch = false;

    for (auto id : m_monitoredSourceIds)
    {
        auto spreadIt = m_deviceSpread.find(id);
        if (spreadIt == m_deviceSpread.end()
                || std::abs(spreadIt->second - m_spread) > kSpreadTol)
        {
            mismatch = true;
            break;
        }
        auto delayIt = m_deviceDelayMode.find(id);
        if (delayIt == m_deviceDelayMode.end()
                || static_cast<int>(delayIt->second) != m_delayMode)
        {
            mismatch = true;
            break;
        }
    }

    if (mismatch && !isTimerRunning())
    {
        startTimer(500);
        setChildrenMouseInterception(false);
    }
    else if (!mismatch && isTimerRunning())
    {
        stopTimer();
        m_flashState = false;
        setAlpha(1.0f);
        setChildrenMouseInterception(true);
        repaint();
    }
}

void UmsciUpmixParamsComponent::updateDelayModeButtonStates()
{
    if (m_delayModeOff)   m_delayModeOff->setToggleState(m_delayMode == 0, juce::dontSendNotification);
    if (m_delayModeTight) m_delayModeTight->setToggleState(m_delayMode == 1, juce::dontSendNotification);
    if (m_delayModeFull)  m_delayModeFull->setToggleState(m_delayMode == 2, juce::dontSendNotification);
}

void UmsciUpmixParamsComponent::setChildrenMouseInterception(bool intercepts)
{
    if (m_spreadSlider)   m_spreadSlider->setInterceptsMouseClicks(intercepts, intercepts);
    if (m_delayModeOff)   m_delayModeOff->setInterceptsMouseClicks(intercepts, false);
    if (m_delayModeTight) m_delayModeTight->setInterceptsMouseClicks(intercepts, false);
    if (m_delayModeFull)  m_delayModeFull->setInterceptsMouseClicks(intercepts, false);
}
