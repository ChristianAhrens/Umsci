![Showreel.001.png](Resources/Documentation/Showreel/Showreel.001.png "Umsci Headline Icons")

See [LATEST RELEASE](https://github.com/ChristianAhrens/Umsci/releases/latest) for available installer packages or join iOS TestFlight Beta:

<img src="Resources/AppStore/TestFlightQRCode.png" alt="TestFlight QR Code" width="15%">

Full code documentation available at [![Documentation](https://img.shields.io/badge/docs-doxygen-blue)](https://ChristianAhrens.github.io/Umsci/)

<a name="toc" />

## Table of contents

* [Overview](#overview)
* [Umsci - app functionality](#Umsci-app-functionality)
  * [Main Umsci UI](#Umsci-ui)
  * [Umsci settings menu](#Umsci-settings-menu)
  * [Connection settings](#connection-settings)
  * [Upmix control settings](#upmix-control-settings)
  * [External control (MIDI)](#external-control-midi)
* [Command-line parameters](#commandlineparameters)
* [Code architecture](#code-architecture)
  * [Component stack](#component-stack)
  * [Configuration persistence](#configuration-persistence)
  * [MIDI control internals](#midi-control-internals)


<a name="overview" />

## Overview

Umsci is a small utility to connect to a d&b Soundscape signal engine via OCA/OCP.1 to control soundobject positions in accordance to standard immersive output formats.
The app subscribes to soundobject and loudspeaker position values and displays them on a 2D surface, blended with a dedicated *Upmix indicator* control element. This allows user interaction to place a visual representation of said immersive output formats on the 2D surface and move the associated soundobject positions to match the immersive output format.

Its sourcecode and prebuilt binaries are made publicly available to enable interested users to experiment, extend and create own adaptations.

Use what is provided here at your own risk!


<a name="Umsci-app-functionality" />

## Umsci - app functionality

<a name="Umsci-UI" />

### Main Umsci UI

![Showreel.002.png](Resources/Documentation/Showreel/Showreel.002.png "Umsci main")

The main view shows three transparent layers stacked on top of each other — all sharing the same coordinate space:

| Layer | Content |
|:------|:--------|
| **Bottom** | Loudspeaker positions read from the DS100 (SVG icons). |
| **Middle** | Soundobject positions, updated live via OCP.1. Draggable in live mode. |
| **Top** | Upmix indicator ring with interactive transform handles (see below). |

#### Upmix indicator handles

| Handle | Gesture | Effect |
|:-------|:--------|:-------|
| Ring arc (floor) | Drag tangentially | Rotates the ring (±180°). |
| Ring arc (height, inner) | Drag radially | Scales the height ring relative to the floor ring. |
| Centre cross | Drag | Shifts the ring centre in XY. |
| Stretch arrow | Drag along arrow | Compresses or expands the front/rear angular spread. |
| Refit button (top-right) | Click | Snaps the transform so the ring fits the loudspeaker bounding box. |

Zoom and pan is available on the main view via mouse wheel / pinch gesture.

<a name="Umsci-settings-menu" />

### Umsci settings menu

![Showreel.003.png](Resources/Documentation/Showreel/Showreel.003.png "Umsci settings")

The gear button (top-left) opens the settings menu. Available options:

| Group | Options |
|:------|:--------|
| **Look & feel** | Follow host / Dark / Light. |
| **Control colour** | Green / Red / Blue / Pink / Laser. |
| **Control format** | Stereo, LRS, LCRS, 5.0, 5.1, 5.1.2, 7.0, 7.1, 7.1.4, 9.1.6. |
| **Control size** | S / M / L — scales the soundobject and speaker icons. |
| **Connection settings…** | OCP.1 IP/port configuration (see below). |
| **Upmix control settings…** | Upmix mode, shape, and channel-start configuration (see below). |
| **External control…** | MIDI assignment for the six upmix transform parameters (see below). |
| **Fullscreen** | Toggles fullscreen / windowed mode (also F key or Escape). |

<a name="connection-settings" />

### Connection settings

Opens the *Control connection settings* dialog. Configure:

- **OCP.1 IP** — the network address of the d&b Soundscape signal engine.
- **OCP.1 port** — default `50014`.
- **OCP.1 IOsize** — IO size corresponding to the soundscape signal engine license M, L, XL that is expected.

Changes take effect on **Ok**; the connection toggle button in the top-right corner shows the live connection state.

<a name="upmix-control-settings" />

### Upmix control settings

Opens the *Upmix control settings* dialog. Configure:

- **Control mode** — Manual or Live (see [Control modes](#control-modes) above).
- **Indicator shape** — Circle or Rectangle path for the upmix ring.
- **Picture to apply** — which format geometry to use when applying positions.
- **First soundobject** — the 1-based DS100 channel index where the upmix renderer's output starts.
- **Visible soundobjects** — limits which soundobject numbers are shown in the main view.

<a name="external-control-midi" />

### External control (MIDI)

Opens the *External control* dialog. Allows each of the six upmix transform parameters to be driven by a MIDI continuous controller.

#### MIDI input device

Select the MIDI input device from the combo box at the top of the dialog. The selection is persisted across sessions.

#### Parameter assignments

Each parameter has a **MidiLearner** row that supports both value-range and command-range assignment:

| Parameter | Default range | Description |
|:----------|:-------------|:------------|
| **Rotation** | −180° … +180° | Rotates the entire ring. CC mid-point = 0° (front). |
| **Translation (scale)** | 0 … 3 | Radial scale of the floor ring. 1.0 = nominal radius. |
| **Height translation** | 0 … 2 | Height ring radius as a fraction of the floor ring. |
| **Angle stretch** | 0 … 2 | Compresses/expands front–rear angular spread. 1.0 = uniform. |
| **Offset X** | −2 … +2 | Shifts the ring centre left/right in units of base radius. |
| **Offset Y** | −2 … +2 | Shifts the ring centre front/back in units of base radius. |

Click **Learn** on a row and move a MIDI controller to capture its CC number and value range automatically. The assignment maps the learned CC range linearly to the full parameter range.

Changes are applied only when **Ok** is pressed; **Cancel** discards any edits. Assignments are persisted in the XML configuration file.

> **Note:** In *Live* mode, MIDI parameter changes are immediately forwarded to the DS100 as OCP.1 position updates. DS100 echo-backs are automatically suppressed so that they do not cause spurious indicator flashing.


<a name="commandlineparameters" />

## Command-line parameters

Umsci has a set of command-line parameters. Parameters are passed directly when launching the executable from a terminal or as part of a launch script.

| Parameter | Description |
|:----------|:------------|
| `--noupdates` | Disables the automatic online update check performed by `JUCEAppBasics::WebUpdateDetector` at startup. Useful in network-restricted environments, automated deployments, or kiosk setups where outbound HTTP requests to GitHub should be avoided. |
| `--noconfigui` | Hides the three configuration buttons (About, Settings, Connection) in the upper-left corner of the UI. Intended for kiosk or embedded deployments where the user should not be able to change settings or disconnect from the soundscape signal engine. |


<a name="code-architecture" />

## Code architecture

This section is aimed at developers who want to understand, extend, or adapt the codebase.

<a name="component-stack" />

### Component stack

The visual output is produced by three `UmsciPaintNControlComponentBase`-derived components held by `UmsciControlComponent`, all given identical pixel bounds:

```
UmsciControlComponent
  ├── UmsciLoudspeakersPaintComponent          (bottom — display only)
  ├── UmsciSoundobjectsPaintComponent          (middle — draggable source circles)
  └── UmsciUpmixIndicatorPaintNControlComponent (top — ring + transform handles)
```

All three inherit `UmsciPaintNControlComponentBase` which provides:
- Coordinate transforms between pixel space and real-world DS100 coordinates (`GetPointForRealCoordinate` / `GetRealCoordinateForPoint`).
- Zoom/pan state (`m_zoomFactor`, `m_zoomPanOffset`) driven by mouse wheel and pinch gesture.
- A `setZoom()` / `resetZoom()` API so that `UmsciControlComponent` can keep all three layers synchronised via the `onViewportZoomChanged` callback.
- A virtual `onZoomChanged()` hook that derived classes override to re-trigger their prerender pass.

`UmsciUpmixIndicatorPaintNControlComponent` also inherits `JUCEAppBasics::TwoDFieldBase`, which supplies per-channel angle and label data for any `juce::AudioChannelSet` (the chosen surround format).

### Upmix indicator geometry

The ring geometry is prerendered into `juce::Path` objects by `PrerenderUpmixIndicatorInBounds()` whenever the bounds, zoom, or any transform parameter changes. The five transform parameters are:

| Member | Default | Meaning |
|:-------|:--------|:--------|
| `m_upmixRot` | `0.0` rad | Ring rotation around Z. 0 = front, positive = clockwise. |
| `m_upmixTrans` | `1.0` | Floor ring radial scale factor. |
| `m_upmixHeightTrans` | `0.6` | Height ring radius as a fraction of floor ring radius. |
| `m_upmixAngleStretch` | `1.0` | Front/rear angular spread compression (1.0 = uniform). |
| `m_upmixOffsetX/Y` | `0.0` | Ring centre offset in units of base radius. |

<a name="configuration-persistence" />

### Configuration persistence

`UmsciAppConfiguration` wraps JUCE's `XmlDocument`-based config file. Classes that need to read/write settings inherit either `UmsciAppConfiguration::Dumper` (write) or `UmsciAppConfiguration::Watcher` (read-on-change). `MainComponent` implements both.

Calling `m_config->triggerConfigurationDump()` serialises the current state; any change on disk automatically calls `onConfigUpdated()` on all registered Watchers.

<a name="midi-control-internals" />

### MIDI control internals

```
MIDI hardware
  │  (MIDI thread)
  ▼
MainComponent::handleIncomingMidiMessage()
  │  juce::MessageManager::callAsync → message thread
  ▼
MainComponent::applyUpmixMidiValue()
  │  maps normalised [0,1] → parameter range
  ▼
UmsciControlComponent::setUpmixTransform() / setUpmixOffset()
  │  updates ring geometry + repaints
  ▼
UmsciControlComponent::triggerUpmixTransformApplied()
  ▼
UmsciUpmixIndicatorPaintNControlComponent::notifyTransformChanged()
  ├── (live mode) increments m_inhibitFlashCount by channel count
  ├── (live mode) fires onSourcePositionChanged for each channel → OCP.1 SetObjectValue
  └── fires onTransformChanged → config dump
```

The `m_inhibitFlashCount` counter absorbs OCP.1 echo-backs (the DS100 reflects each `SetObjectValue` back as a notification). Without it, each echo would call `updateFlashState()`, which detects a mismatch between rendered and live positions and starts the flash animation — producing visible flicker during MIDI control.

MIDI assignments are stored as `JUCEAppBasics::MidiCommandRangeAssignment` objects and serialised to hex strings in the XML config under the `<ExternalControlConfig>` element.
