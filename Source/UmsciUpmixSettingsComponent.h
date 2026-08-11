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


/**
 * @class UmsciUpmixSettingsComponent
 * @brief Body of the "Upmix control settings" dialog, presented inside a
 *        juce::AlertWindow via addCustomComponent().
 *
 * juce::AlertWindow's own addComboBox()/addTextEditor() rows stack a name label
 * (18px) above each 22px control, and separately position the OK/Cancel buttons
 * at a proportion of the dialog's overall height — a combination that does not
 * reliably grow to fit an increasing row count (observed: the last rows started
 * overlapping the buttons once a 9th row was added). This component lays out all
 * rows itself with a fixed, known height and side-by-side label+control rows
 * (more compact than AlertWindow's stacked layout), and reports that height via
 * getPreferredHeight() so the caller can rely on addCustomComponent()'s simple
 * "just add my height" sizing instead of AlertWindow's per-row heuristic.
 *
 * Usage: construct, call the setters to populate current values, addCustomComponent()
 * it to a juce::AlertWindow, and after the user presses OK read back the getters.
 */
class UmsciUpmixSettingsComponent : public juce::Component
{
public:
    UmsciUpmixSettingsComponent();
    ~UmsciUpmixSettingsComponent() override;

    //==============================================================================
    void resized() override;

    /** @brief Total height needed to show all rows without clipping/overlap. */
    int getPreferredHeight() const;

    //==============================================================================
    void setControlFormatItems(const juce::StringArray& items, int selectedIndex);
    int  getControlFormatIndex() const;

    void setLiveMode(bool liveMode);
    bool getLiveMode() const;

    void setShapeIsRectangle(bool isRectangle);
    bool getShapeIsRectangle() const;

    void setVisualizationIsSolidBar(bool isSolidBar);
    bool getVisualizationIsSolidBar() const;

    void setStartSoundobjectId(int startId);
    int  getStartSoundobjectId() const;

    void setShowAllSources(bool showAll);
    bool getShowAllSources() const;

    void setShowLfeChannel(bool show);
    bool getShowLfeChannel() const;

    void setShowLevelMeter(bool show);
    bool getShowLevelMeter() const;

    void setShowChannelLabels(bool show);
    bool getShowChannelLabels() const;

private:
    //==============================================================================
    static constexpr int kRowH      = 26;
    static constexpr int kRowGap    = 8;
    static constexpr int kMargin    = 8;
    static constexpr int kLabelW    = 170;
    static constexpr int kDialogWidth = 420;
    static constexpr int kRowCount  = 9;

    juce::Label    m_controlFormatLabel   { {}, "Channel format" };
    juce::ComboBox m_controlFormatCombo;
    juce::Label    m_liveModeLabel        { {}, "Control mode" };
    juce::ComboBox m_liveModeCombo;
    juce::Label    m_shapeLabel           { {}, "Indicator shape" };
    juce::ComboBox m_shapeCombo;
    juce::Label    m_visualizationLabel   { {}, "Indicator visualization" };
    juce::ComboBox m_visualizationCombo;
    juce::Label    m_startIdLabel         { {}, "First soundobject" };
    juce::TextEditor m_startIdEditor;
    juce::Label    m_showSourcesLabel     { {}, "Visible soundobjects" };
    juce::ComboBox m_showSourcesCombo;
    juce::Label    m_lfeLabel             { {}, "LFE / positionless channel" };
    juce::ComboBox m_lfeCombo;
    juce::Label    m_levelMeterLabel      { {}, "Level metering" };
    juce::ComboBox m_levelMeterCombo;
    juce::Label    m_channelLabelsLabel   { {}, "Channel labels" };
    juce::ComboBox m_channelLabelsCombo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UmsciUpmixSettingsComponent)
};
