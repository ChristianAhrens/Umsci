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

#include "CustomParameterConfig.h"


/**
 * @class CustomParameterControlConfigDialog
 * @brief Settings panel for user-defined OSC parameter control.
 *
 * Presented inside a juce::AlertWindow via addCustomComponent().
 * Shows:
 *  - Connection section: remote host IP, send port, receive port.
 *  - Parameter list: scrollable rows, one per parameter entry.
 *    Each row has: type selector, name, min, max, step, step-names (Discrete),
 *    OSC address, and a delete button.
 *  - "Add parameter" button below the list.
 *
 * Call setConfig() before showing the dialog, then getConfig() after the user
 * presses OK to retrieve the edited state.
 */
class CustomParameterControlConfigDialog : public juce::Component
{
public:
    CustomParameterControlConfigDialog();
    ~CustomParameterControlConfigDialog() override;

    //==============================================================================
    void resized() override;
    void paint(juce::Graphics&) override;

    //==============================================================================
    void setConfig(const CustomParameterConfig& config);
    CustomParameterConfig getConfig() const;

private:
    //==============================================================================
    static constexpr int kRowH         = 28;  ///< Height of a single parameter row.
    static constexpr int kHeaderH      = 80;  ///< Height of the connection section (kMargin + 18 + kRowH + kRowH).
    static constexpr int kMaxViewportH = 300; ///< Maximum height of the scrollable parameter list.
    static constexpr int kAddBtnH      = 30;  ///< Height of the "Add parameter" button.
    static constexpr int kMargin       = 6;
    static constexpr int kDialogWidth  = 700; ///< Fixed width used when sizing the component.

    //==============================================================================
    void rebuildParameterRows();
    void addParameterRow(int rowIndex);
    void removeParameterRow(int rowIndex);
    void updateRowVisibility(int rowIndex);
    int  computePreferredHeight() const;

    //==============================================================================
    // Connection section widgets
    std::unique_ptr<juce::Label>      m_hostLabel;
    std::unique_ptr<juce::TextEditor> m_hostEditor;
    std::unique_ptr<juce::Label>      m_sendPortLabel;
    std::unique_ptr<juce::TextEditor> m_sendPortEditor;
    std::unique_ptr<juce::Label>      m_recvPortLabel;
    std::unique_ptr<juce::TextEditor> m_recvPortEditor;

    // Scrollable parameter list
    std::unique_ptr<juce::Viewport>   m_viewport;
    std::unique_ptr<juce::Component>  m_listContent;

    std::unique_ptr<juce::TextButton> m_addButton;

    //==============================================================================
    struct ParamRow
    {
        std::unique_ptr<juce::ComboBox>    typeCombo;
        std::unique_ptr<juce::TextEditor>  nameEditor;
        std::unique_ptr<juce::TextEditor>  minEditor;
        std::unique_ptr<juce::TextEditor>  maxEditor;
        std::unique_ptr<juce::TextEditor>  stepEditor;
        std::unique_ptr<juce::TextEditor>  stepNamesEditor;
        std::unique_ptr<juce::TextEditor>  oscAddrEditor;
        std::unique_ptr<juce::TextButton>  deleteButton;
    };
    std::vector<ParamRow> m_paramRows;

    CustomParameterConfig m_config;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomParameterControlConfigDialog)
};
