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

#include "LogComponent.h"

#include <CustomLookAndFeel.h>

//==============================================================================
LoggerImpl::LoggerImpl()
{
    juce::Logger::setCurrentLogger(this);
}

LoggerImpl::~LoggerImpl()
{
    cancelPendingUpdate();
    juce::Logger::setCurrentLogger(nullptr);
}

void LoggerImpl::logMessage(const juce::String& message)
{
    // Keep IDE / platform console output working alongside our buffer.
    juce::Logger::outputDebugString(message);

    auto t = juce::Time::getCurrentTime();
    juce::String stamped = t.formatted("[%H:%M:%S.") + juce::String(t.getMilliseconds()).paddedLeft('0', 3) + "] " + message;

    {
        juce::ScopedLock sl(m_lock);
        if (m_lines.size() >= maxLines)
            m_lines.remove(0);
        m_lines.add(stamped);
        m_pending.add(stamped);
    }
    triggerAsyncUpdate();
}

void LoggerImpl::handleAsyncUpdate()
{
    juce::StringArray toDispatch;
    {
        juce::ScopedLock sl(m_lock);
        toDispatch = m_pending;
        m_pending.clear();
    }

    if (onNewLines && !toDispatch.isEmpty())
        onNewLines(toDispatch);
}

juce::StringArray LoggerImpl::getLines() const
{
    juce::ScopedLock sl(m_lock);
    return m_lines;
}

void LoggerImpl::clearLines()
{
    {
        juce::ScopedLock sl(m_lock);
        m_lines.clear();
        m_pending.clear();
    }
    cancelPendingUpdate();
}


//==============================================================================
LogComponent::LogComponent(LoggerImpl& logger)
    : m_logger(logger)
{
    setMultiLine(true, false);
    setScrollbarsShown(true);
    setReadOnly(true);

    // Populate with any lines already in the buffer before the window opened.
    refreshFromBuffer();

    // Subscribe to future lines arriving on the message thread.
    m_logger.onNewLines = [this](const juce::StringArray&) {
        m_refreshPending = true;
    };

    setOpaque(true);

    startTimer(300);
}

LogComponent::~LogComponent()
{
    m_logger.onNewLines = nullptr;
}

void LogComponent::lookAndFeelChanged()
{
    setColour(juce::TextEditor::backgroundColourId, getLookAndFeel().findColour(juce::Slider::backgroundColourId));
    setColour(juce::TextEditor::outlineColourId, getLookAndFeel().findColour(juce::Slider::trackColourId));
    setColour(juce::TextEditor::focusedOutlineColourId, getLookAndFeel().findColour(juce::Slider::trackColourId));
}

void LogComponent::refreshFromBuffer()
{
    setText(m_logger.getLines().joinIntoString("\n"), false);
    moveCaretToEnd();
}

void LogComponent::timerCallback()
{
    if (m_refreshPending)
    {
        refreshFromBuffer();
        m_refreshPending = false;
    }
}

void LogComponent::userTriedToCloseWindow()
{
    if (onCloseButtonPressed)
        onCloseButtonPressed();
}

