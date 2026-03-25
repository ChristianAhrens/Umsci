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


//==============================================================================
/**
 * @class Logger
 * @brief Captures all JUCE DBG() / Logger output and buffers up to maxLines
 *        messages in a thread-safe circular buffer.
 *
 * Installs itself as the current JUCE Logger on construction and removes itself
 * on destruction. Platform debug output (IDE console, logcat, NSLog…) is
 * preserved by forwarding each message to outputDebugString().
 * The onNewLines callback is always invoked on the message thread.
 */
class LoggerImpl :  public juce::Logger,
                    private juce::AsyncUpdater
{
public:
    static constexpr int maxLines = 1000;

    LoggerImpl();
    ~LoggerImpl() override;

    /** Returns a snapshot of the current line buffer. Thread-safe. */
    juce::StringArray getLines() const;

    /** Clears the buffer. Thread-safe. */
    void clearLines();

    /** Called on the message thread whenever new lines have been added. */
    std::function<void(const juce::StringArray&)> onNewLines;

private:
    void logMessage(const juce::String& message) override;
    void handleAsyncUpdate() override;

    mutable juce::CriticalSection m_lock;
    juce::StringArray m_lines;
    juce::StringArray m_pending;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoggerImpl)
};


//==============================================================================
/**
 * @class LogComponent
 * @brief Content component for the debug log window.
 *        Displays the LoggerImpl buffer in a scrollable monospace editor.
 */
class LogComponent : public juce::TextEditor, public juce::Timer
{
public:
    explicit LogComponent(LoggerImpl& logger);
    ~LogComponent() override;

    void lookAndFeelChanged() override;

    void userTriedToCloseWindow() override;

    void timerCallback() override;

    std::function<void()> onCloseButtonPressed;

private:
    void refreshFromBuffer();

    LoggerImpl& m_logger;
    bool m_refreshPending = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogComponent)
};


