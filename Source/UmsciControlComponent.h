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


#include "UmsciAppConfiguration.h"
#include "DeviceController.h"


#include "UmsciPaintNControlComponents/UmsciPaintNControlComponentBase.h"
#include "UmsciPaintNControlComponents/UmsciUpmixIndicatorPaintNControlComponent.h"

/*Fwd decls*/
class UmsciLoudspeakersPaintComponent;
class UmsciSoundobjectsPaintComponent;

/**
 * @class UmsciControlComponent
 * @brief The main visualisation and interaction surface for Umsci.
 *
 * ## Composition
 * Three transparent JUCE Components are stacked in z-order, all sharing the same
 * pixel bounds, and all using the same normalised coordinate system managed by
 * `UmsciPaintNControlComponentBase`:
 *
 * | Layer (z-order) | Class | Purpose |
 * |---|---|---|
 * | Bottom | `UmsciLoudspeakersPaintComponent`             | Paints speaker SVG icons at their real-world positions (read-only). |
 * | Middle | `UmsciSoundobjectsPaintComponent`             | Paints and lets the user drag source circles. |
 * | Top    | `UmsciUpmixIndicatorPaintNControlComponent`   | Paints the upmix ring with rotation/scale/stretch handles. |
 *
 * ## Data flow
 * 1. `DeviceController::onRemoteObjectReceived` (message thread) → `setRemoteObject()`.
 * 2. `setRemoteObject()` stores the value in `m_source*` / `m_speaker*` maps and
 *    calls the appropriate setter on the relevant paint component.
 * 3. The paint component marks itself dirty and repaints on the next message-thread
 *    cycle.
 *
 * ## "Database complete" concept
 * On each new connection, `UmsciControlComponent` waits until initial values have
 * been received for every subscribed object before signalling `onDatabaseComplete`.
 * Until then the visualisation may show stale or partial data.
 *
 * ## Coordinate system
 * All source and speaker positions received from the DS100 are in a normalised
 * real-world space (roughly 0.0–1.0 for X/Y, see `UmsciPaintNControlComponentBase`
 * for the exact transform).  The paint components convert these to pixel coordinates
 * for rendering.
 *
 * @note [MANUAL CONTEXT NEEDED] Document the exact real-world coordinate units
 *       used by the DS100 (e.g. metres, normalised 0-1) and how they relate to the
 *       stage/room geometry so that contributors understand what the displayed
 *       numbers mean physically.
 *
 * ## Viewport zoom and sibling synchronisation
 * All three layers share a single zoom state (scale factor + pan offset).  When the
 * user zooms on any layer, that layer fires `UmsciPaintNControlComponentBase::onViewportZoomChanged`;
 * the `syncViewportZoom` lambda (wired up in the constructor) forwards the new state
 * to the other two layers via `setZoom()`, which does not re-fire the callback.
 *
 * On iOS/iPadOS, JUCE 8's touch routing delivers each finger as a separate
 * `MouseEvent` routed to whichever component passes `hitTest()` at that position.
 * Both fingers of a pinch therefore rarely arrive at the same JUCE component, making
 * the JUCE-level `processPinchGesture()` fallback unreliable.  `parentHierarchyChanged()`
 * therefore attaches a native `UIPinchGestureRecognizer` (via
 * `JUCEAppBasics::iOS_utils::registerNativePinchOnView()`) to the JUCE peer UIView
 * the first time a peer becomes available.  The UIKit gesture fires at the gesture-
 * recognizer layer before JUCE's per-component touch routing, so it works regardless
 * of which components the individual fingers hit.  The incremental scale and centre
 * point are forwarded to `simulatePinchZoom()` on one of the paint layers, which
 * fires `onViewportZoomChanged` and thus synchronises all three siblings normally.
 */
class UmsciControlComponent :   public juce::Component, public UmsciAppConfiguration::XmlConfigurableElement
{
public:
    UmsciControlComponent();
    ~UmsciControlComponent() override;

    //==============================================================================
    void resized() override;
    void paint(juce::Graphics& g) override;
    void parentHierarchyChanged() override;

    //==============================================================================
    std::unique_ptr<XmlElement> createStateXml() override;
    bool setStateXml(XmlElement* stateXml) override;

    //==============================================================================
    /**
     * @brief Sets the DS100 input/output channel count for this session.
     *
     * The pair is {inputChannels, outputChannels}.  It is used to limit which
     * channel IDs are subscribed to and rendered.
     *
     * The DS100 supports M / L / XL software licenses that set different maximum
     * input and output channel counts.  The I/O size is configured by the user in
     * Umsci's connection settings and must match (or be within) the device's licensed
     * capacity — it is not auto-detected from the device.
     *
     * @note [MANUAL CONTEXT NEEDED] Confirm whether Umsci validates the configured
     *       size against what the device reports, or simply trusts the user entry.
     */
    const std::pair<int, int>& getOcp1IOSize();
    void setOcp1IOSize(const std::pair<int, int>& ioSize);

    //==============================================================================
    /** @brief Updates the device name label (shown as a title/overlay in the UI). */
    void setDeviceName(const std::string& name);

    /** @name Sound-object (matrix input) setters
     * @brief Called by `setRemoteObject()` when the corresponding DS100 notification arrives.
     *
     * `sourceId` is 1-based (DS100 channels 1–128).
     * @{ */
    void setSourceName(std::int16_t sourceId, const std::string& name);
    void setSourceMute(std::int16_t sourceId, const std::uint8_t& mute);   ///< 0 = unmuted, nonzero = muted.
    void setSourceGain(std::int16_t sourceId, const std::float_t& gain);   ///< Gain in dB.
    /** @brief Sets the 3-component (X, Y, Z) absolute position in normalised real-world coordinates. */
    void setSourcePosition(std::int16_t sourceId, const std::array<std::float_t, 3>& position);
    /** @brief Delay mode: 0 = off, 1 = compensate, 2 = reflect (DS100-specific enum). */
    void setSourceDelayMode(std::int16_t sourceId, const std::uint16_t& delayMode);
    /** @brief Spread factor 0.0–1.0 (0 = point source, 1 = full spread). */
    void setSourceSpread(std::int16_t sourceId, const std::float_t& spread);
    /** @} */

    /** @name Loudspeaker (matrix output) setters
     * @brief Called by `setRemoteObject()` when the corresponding DS100 notification arrives.
     *
     * `speakerId` is 1-based (DS100 outputs 1–64).
     * @{ */
    void setSpeakerName(std::int16_t speakerId, const std::string& name);
    void setSpeakerMute(std::int16_t speakerId, const std::uint8_t& mute);
    void setSpeakerGain(std::int16_t speakerId, const std::float_t& gain);
    /**
     * @brief Sets the 6-component speaker position: {X, Y, Z, horizontal angle,
     *        vertical angle, rotation}.
     *
     * @note [MANUAL CONTEXT NEEDED] Document the exact units and axis conventions
     *       for each of the 6 components (e.g. metres vs. normalised, degrees vs.
     *       radians, coordinate handedness).
     */
    void setSpeakerPosition(std::int16_t speakerId, const std::array<std::float_t, 6>& position);
    /** @} */

    //==============================================================================
    /**
     * @name Upmix configuration
     * @brief Settings for the upmix indicator overlay (`UmsciUpmixIndicatorPaintNControlComponent`).
     *
     * The upmix indicator shows a ring of virtual speaker positions representing the
     * target upmix channel configuration (e.g. 5.1, 7.1, Atmos).  The ring can be
     * rotated, scaled (radius), stretched per-angle, and shifted (offset) by the user
     * via on-screen handles.
     *
     * `sourceStartId` identifies the first DS100 sound-object channel that is assigned
     * to the upmix renderer inputs; subsequent channels are implicitly consecutive.
     *
     * `liveMode` controls whether the upmix indicator actively follows incoming DS100
     * position updates or shows a static reference layout.
     *
     * @note [MANUAL CONTEXT NEEDED] Describe the upmix workflow: what does the
     *       indicator represent, how does it relate to an external upmix renderer,
     *       and what the user is expected to do with the transform handles.
     * @{
     */
    void setUpmixChannelConfiguration(const juce::AudioChannelSet& upmixChannelConfig);
    const juce::AudioChannelSet getUpmixChannelConfiguration();
    void setUpmixSourceStartId(int startId); ///< 1-based DS100 channel of the first upmix input.
    int  getUpmixSourceStartId() const;
    void setUpmixLiveMode(bool liveMode);    ///< true = follow live DS100 positions.
    bool getUpmixLiveMode() const;
    void setUpmixShape(UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape shape);
    UmsciUpmixIndicatorPaintNControlComponent::IndicatorShape getUpmixShape() const;
    /** @} */

    //==============================================================================
    /**
     * @name Upmix spatial transform
     * @brief Controls the rotation, radial scale, height translation, and angular
     *        stretch of the upmix indicator ring relative to its default position.
     *
     * All values are in normalised units consistent with the component's coordinate
     * system (see `UmsciPaintNControlComponentBase`).  Changing any of these fires
     * the `onUpmixTransformChanged` callback so that the values can be persisted.
     * @{
     */
    void setUpmixTransform(float rot, float trans, float heightTrans, float angleStretch = 1.0f);
    float getUpmixRot() const;          ///< Ring rotation in normalised units (0–1 → 0–360°).
    float getUpmixTrans() const;        ///< Radial scale factor.
    float getUpmixHeightTrans() const;  ///< Height (Z) translation.
    float getUpmixAngleStretch() const; ///< Per-angle stretch factor (1.0 = uniform).
    /** @} */

    //==============================================================================
    /**
     * @name Upmix ring offset
     * @brief Translates the entire upmix ring in the X/Y plane independently of rotation.
     * @{
     */
    void  setUpmixOffset(float x, float y);
    float getUpmixOffsetX() const;
    float getUpmixOffsetY() const;
    /** @} */

    //==============================================================================
    /**
     * @brief Fires live-mode position callbacks and `onUpmixTransformChanged` after
     *        a programmatic transform update (e.g. from MIDI control).
     *
     * Call this after `setUpmixTransform()` / `setUpmixOffset()` when the change
     * originates outside the UI (MIDI, automation) to ensure DS100 positions are
     * updated in live mode and the config is marked dirty.
     */
    void triggerUpmixTransformApplied();

    /**
     * @brief Starts the flash animation on the upmix indicator if the ideal ring
     *        positions diverge from the current DS100 positions.
     *
     * Call this after a programmatic transform update when live mode is off, so the
     * operator sees a visual cue that the device positions are out of sync.
     */
    void triggerUpmixFlashCheck();

    //==============================================================================
    /**
     * @brief When false, only sound objects that are part of the upmix group
     *        (i.e. channels >= sourceStartId within the upmix channel count) are
     *        rendered; all others are hidden.
     */
    void setShowAllSources(bool showAll);
    bool getShowAllSources() const;

    //==============================================================================
    /** @brief Sets the visual size of source/speaker icons (small / medium / large). */
    void setControlsSize(UmsciPaintNControlComponentBase::ControlsSize size);
    UmsciPaintNControlComponentBase::ControlsSize getControlsSize() const;

    //==============================================================================
    /** @brief Clears all cached source/speaker data and marks the database incomplete. */
    void resetData();

    //==============================================================================
    /**
     * @brief Fired on the message thread when all initially subscribed values have
     *        been received from the DS100 (i.e. the "database" is fully populated).
     * Consumers use this to trigger a first full render or to enable UI controls.
     */
    std::function<void()> onDatabaseComplete;

    /**
     * @brief Fired on the message thread whenever the user changes any upmix transform
     *        parameter via the on-screen handles, so the caller can persist the new values.
     */
    std::function<void()> onUpmixTransformChanged;

private:
    //==============================================================================
    void rebuildOcp1ObjectTree();
    void updatePaintComponents();
    void updateSourceIdFilter();

    void setRemoteObject(const DeviceController::RemoteObject& obj);

    bool checkIsDatabaseComplete();
    bool isDatabaseComplete();
    void setDatabaseComplete(bool complete);

    const juce::Rectangle<float> getRealBoundingRect();
    const std::array<float, 6> getRealBoundingCube();

    //==============================================================================
    std::unique_ptr<UmsciLoudspeakersPaintComponent>            m_loudspeakersInAreaPaintComponent;
    std::unique_ptr<UmsciSoundobjectsPaintComponent>            m_soundobjectsInAreaPaintComponent;
    std::unique_ptr<UmsciUpmixIndicatorPaintNControlComponent>  m_upmixIndicatorPaintAndControlComponent;

    std::pair<int, int> m_ocp1IOSize;

    bool    m_showAllSources = true;

    bool    m_databaseComplete = false;

    juce::Rectangle<float>  m_boundsRealRef;

    std::string                                         m_deviceName;

    std::map<std::int16_t, std::string>                 m_sourceName;
    std::map<std::int16_t, bool>                        m_sourceMute;
    std::map<std::int16_t, std::float_t>                m_sourceGain;
    std::map<std::int16_t, std::array<std::float_t, 3>> m_sourcePosition;
    std::map<std::int16_t, std::uint16_t>               m_sourceDelayMode;
    std::map<std::int16_t, std::float_t>                m_sourceSpread;

    std::map<std::int16_t, std::string>                 m_speakerName;
    std::map<std::int16_t, bool>                        m_speakerMute;
    std::map<std::int16_t, std::float_t>                m_speakerGain;
    std::map<std::int16_t, std::array<std::float_t, 6>> m_speakerPosition;

#if JUCE_IOS
    void* m_nativePinchViewHandle = nullptr; ///< Handle to the peer UIView that has the pinch recognizer registered.
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UmsciControlComponent)
};

