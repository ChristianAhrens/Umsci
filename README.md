![Showreel.001.png](Resources/Documentation/Showreel/Showreel.001.png "Umsci Headline Icons")

See [LATEST RELEASE](https://github.com/ChristianAhrens/Umsci/releases/latest) for available installer packages or join iOS TestFlight Beta:

<img src="Resources/AppStore/TestFlightQRCode.png" alt="TestFlight QR Code" width="15%">

Full code documentation available at [![Documentation](https://img.shields.io/badge/docs-doxygen-blue)](https://ChristianAhrens.github.io/Umsci/)

<a name="toc" />

## Table of contents

* [Overview](#overview)
* [Use cases](#use-cases)
* [Umsci - app functionality](#Umsci-app-functionality)
  * [Main Umsci UI](#Umsci-ui)
    * [Upmix indicator handles](#upmix-indicator-handles)
    * [Zoom and pan](#zoom-and-pan)
    * [Control modes](#control-modes)
  * [Umsci settings menu](#Umsci-settings-menu)
  * [Connection settings](#connection-settings)
  * [Upmix control settings](#upmix-control-settings)
  * [External control (MIDI)](#external-control-midi)
* [Platform support](#platform-support)
* [Command-line parameters](#commandlineparameters)
* [Code architecture](#code-architecture)
  * [Component stack](#component-stack)
  * [Zoom and pan internals](#zoom-and-pan-internals)
  * [iOS native touch handling](#ios-native-touch-handling)
  * [Configuration persistence](#configuration-persistence)
  * [MIDI control internals](#midi-control-internals)


<a name="overview" />

## Overview

Umsci is a utility that connects to a **d&b Soundscape** signal processing engine (DS100) via the OCA/OCP.1 network protocol and lets an operator visualise and control soundobject positions in real time.

Its primary focus is the specific workflow of mapping an external **upmix renderer's** virtual output channels onto the physical room layout managed by the DS100.  An upmix renderer (e.g. a DAW plug-in or hardware processor) takes a stereo or surround bus and produces a set of spatialised output channels that should be fed into consecutive DS100 sound objects.  Umsci gives the operator a graphical handle on that block of sound objects, letting them rotate, scale, stretch and shift the entire virtual speaker ring to best match the physical loudspeaker array — while watching how the actual DS100 positions track the intended geometry.

The application subscribes to all soundobject and loudspeaker position values published by the DS100 and renders them on a shared 2D view that blends three overlaid layers: the physical loudspeaker layout (read-only), the live soundobject positions (interactive), and the upmix indicator control ring.

Its source code and prebuilt binaries are made publicly available to enable interested users to experiment, extend, and create their own adaptations.

Use what is provided here at your own risk!


<a name="use-cases" />

## Use cases

### Upmix monitoring and alignment

The primary use case: an external upmix renderer outputs N channels (e.g. 16 channels for a 7.1.4 Atmos bed) into DS100 sound objects starting at a configurable channel index.  Umsci shows the idealised ring geometry for the chosen channel format (Stereo through 9.1.6) superimposed over the physical loudspeaker layout.  The operator can interactively adjust rotation, scale, height and front/rear stretch to align the virtual ring with the available speakers, and in *Live* mode the DS100 positions are updated in real time as the handles are dragged.

### General soundobject position monitoring and editing

All DS100 sound objects (up to 64 / 128 depending on the M / L / XL license installed in the signal engine) are rendered as draggable circles.  Even without the upmix focus, Umsci provides a quick spatial overview of the full object set and allows individual objects to be repositioned from any connected device.

### Kiosk / embedded control surface

Using the `--noconfigui` and `--noupdates` command-line flags, Umsci can be locked into a minimal, touch-friendly control surface where settings dialogs and update notifications are hidden.  This is suited for fixed installations where the operator should only move objects and adjust the upmix transform, without access to connection or configuration options.

### iOS / iPadOS mobile control

Umsci runs natively on iOS/iPadOS.  The full touch interaction model — including two-finger pinch zoom on the scene view — is supported.  An iPad can act as a wireless remote control surface for a DS100 on the same network, using Zeroconf/mDNS auto-discovery to find the device without entering an IP address manually.


<a name="Umsci-app-functionality" />

## Umsci - app functionality

<a name="Umsci-UI" />

### Main Umsci UI

![Showreel.002.png](Resources/Documentation/Showreel/Showreel.002.png "Umsci main")

The main view shows three transparent layers stacked on top of each other — all sharing the same coordinate space and zoom/pan state:

| Layer | Content |
|:------|:--------|
| **Bottom** | Loudspeaker positions read from the DS100 (SVG icons, display only). |
| **Middle** | Soundobject positions, updated live via OCP.1. Draggable in Live mode. |
| **Top** | Upmix indicator ring with interactive transform handles (see below). |

The connection toggle button (top-right) shows the live OCP.1 connection state and can be clicked to connect or disconnect.

<a name="upmix-indicator-handles" />

#### Upmix indicator handles

The top layer renders a ring (circle or rectangle path) representing the ideal speaker positions for the chosen immersive format.  Five handles allow the ring geometry to be adjusted:

| Handle | Gesture | Effect |
|:-------|:--------|:-------|
| Ring arc (floor) | Drag tangentially | Rotates the ring (±180°). |
| Ring arc (height, inner) | Drag radially | Scales the height ring relative to the floor ring. |
| Centre cross | Drag | Shifts the ring centre in XY. |
| Stretch arrow | Drag along arrow | Compresses or expands the front/rear angular spread. |
| Refit button (top-right) | Click | Snaps the transform so the ring fits inside the loudspeaker bounding box. |

Double-clicking a handle resets that parameter to its default value.

When *Live mode* is active, coloured dots show the actual DS100 source positions for the upmix channels overlaid on the ideal ring, allowing the operator to see alignment errors and oscillations in real time.  A flashing dot indicates that the DS100 is echoing back position updates (normal behaviour during active control).

<a name="zoom-and-pan" />

#### Zoom and pan

The scene view supports zoom and pan on all platforms:

| Input | Action |
|:------|:-------|
| Mouse wheel (scroll up/down) | Zoom in/out centred on cursor position. |
| Trackpad two-finger pinch | Zoom in/out centred on gesture midpoint. |
| Two-finger touch pinch (iOS/iPadOS) | Zoom in/out centred on gesture midpoint (native UIKit gesture recognizer). |
| Mouse wheel while pressing a modifier key | Pan horizontally / vertically. |

The zoom level is shared across all three layers so they stay aligned.

<a name="control-modes" />

#### Control modes

The *Control mode* setting (in *Upmix control settings*) governs how soundobject position changes originating from Umsci are sent to the DS100:

| Mode | Behaviour |
|:-----|:----------|
| **Manual (double-click to apply)** | Dragging or adjusting handles updates the on-screen geometry locally. The new positions are only sent to the DS100 when the user double-clicks the ring or handle. Use this mode when you want to preview a new layout before committing it. |
| **Live (apply changes immediately)** | Every position change is sent to the DS100 in real time as the user drags. DS100 echo-backs are automatically absorbed so they do not produce spurious visual feedback. |

<a name="Umsci-settings-menu" />

### Umsci settings menu

![Showreel.003.png](Resources/Documentation/Showreel/Showreel.003.png "Umsci settings")

The gear button (top-left) opens the settings menu.  Available options:

| Group | Options |
|:------|:--------|
| **Look & feel** | Follow host / Dark / Light. |
| **Control colour** | Green / Red / Blue / Anni Pink / Laser. |
| **Control format** | Stereo, LRS, LCRS, 5.0, 5.1, 5.1.2, 7.0, 7.1, 7.1.4, 9.1.6 — sets the upmix ring channel geometry. |
| **Control size** | S / M / L — scales the soundobject and speaker icons. |
| **Connection settings…** | OCP.1 IP/port/IO-size configuration (see below). |
| **Upmix control settings…** | Upmix mode, shape, channel-start and visibility configuration (see below). |
| **External control…** | MIDI assignment for the six upmix transform parameters (see below). |
| **Fullscreen** | Toggles fullscreen / windowed mode (also F key or Escape). |

The control format determines how many channels the upmix ring has and which speaker labels are shown (L, C, R, Ls, Rs, Ltf, Rtf, etc.).  Supported formats span from simple Stereo (2 channels) up to 9.1.6 Atmos (16 channels).

<a name="connection-settings" />

### Connection settings

Opens the *Control connection settings* dialog.  Configure:

- **OCP.1 IP** — the network address of the d&b Soundscape signal engine (DS100).  On iOS/iPadOS the *Discovered devices* combo box automatically lists DS100 devices found via Zeroconf/mDNS on the local network; selecting one fills in the IP and port fields automatically.
- **OCP.1 port** — default `50014`.
- **OCP.1 IOsize** — the input × output channel count matching the software license installed in the DS100 signal engine.  Enter as `<inputs>x<outputs>`, for example `64x64` for an L license or `128x64` for an XL license.  This controls how many soundobject and loudspeaker subscriptions Umsci creates.

> **Beware:** The IOsize defines how many channels are expected to be available on the signal engine, and Umsci will not finish the connection and subscription cycle if this expectation is not met by the actual available device!

Changes take effect on **Ok**; the connection toggle button in the top-right corner shows the live connection state.

<a name="upmix-control-settings" />

### Upmix control settings

Opens the *Upmix control settings* dialog.  Configure:

- **Channel format** — the channel format whose geometry is used when applying positions (Stereo through 9.1.6). This takes into account the actual immersive format geometry definitions, esp. angles.
See e.g.
  - [ITU-R BS.775-4](https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.775-4-202212-I!!PDF-E.pdf)
  - [ITU-R BS.2159-9](https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-BS.2159-9-2022-PDF-E.pdf)
  - [Dolby Atmos speaker setup guides](https://www.dolby.com/about/support/guide/speaker-setup-guides/)
- **Control mode** — see [Control modes](#control-modes) above.
- **Indicator shape** — *Circle* (default) draws the upmix ring as a circular arc; *Rectangle* draws it as a rounded rectangle path.  Choose based on which shape better matches the physical loudspeaker layout of the venue.
- **First soundobject** — the 1-based DS100 input channel index where the upmix renderer's output starts.  If the upmix renderer occupies channels 17–32, set this to `17`.  Subsequent channels are implied consecutively up to the channel count of the selected format.
- **Visible soundobjects** — *All* shows every soundobject in the scene; *Upmix controlled only* hides all soundobjects that are not part of the current upmix channel block, reducing visual clutter when the DS100 carries many other sources.

<a name="external-control-midi" />

### External control (MIDI)

Opens the *External control* dialog.  Allows each of the six upmix transform parameters to be driven by a MIDI continuous controller, enabling control from a hardware surface, DAW automation, or any other MIDI source.

#### MIDI input device

Select the MIDI input device from the combo box at the top of the dialog.  The selection is persisted across sessions.

#### Parameter assignments

Each parameter has a **MidiLearner** row that supports both value-range and command-range assignment:

| Parameter | Default range | Description |
|:----------|:-------------|:------------|
| **Rotation** | −180° … +180° | Rotates the entire ring. CC mid-point = 0° (front). |
| **Translation (scale)** | 0 … 3 | Radial scale of the floor ring. 1.0 = nominal radius. |
| **Height translation** | 0 … 2 | Height ring radius as a fraction of the floor ring. 0.6 = default (40 % smaller). |
| **Angle stretch** | 0 … 2 | Compresses/expands front–rear angular spread. 1.0 = uniform. |
| **Offset X** | −2 … +2 | Shifts the ring centre left/right in units of base radius. |
| **Offset Y** | −2 … +2 | Shifts the ring centre front/back in units of base radius. |

Click **Learn** on a row and move a MIDI controller to capture its CC number and value range automatically.  The assignment maps the learned CC range linearly to the full parameter range.

Changes are applied only when **Ok** is pressed; **Cancel** discards any edits.  Assignments are persisted in the XML configuration file.

> **Note:** In *Live* mode, MIDI parameter changes are immediately forwarded to the DS100 as OCP.1 position updates.  DS100 echo-backs are automatically suppressed so that they do not cause spurious indicator flashing.


<a name="platform-support" />

## Platform support

| Platform | Notes |
|:---------|:------|
| **macOS** | Desktop app; mouse wheel and trackpad pinch zoom supported. |
| **Windows** | Desktop app; mouse wheel zoom supported. |
| **iOS / iPadOS** | Native app; full touch support including two-finger pinch zoom.  Safe-area insets (notch, Dynamic Island, home indicator, Stage Manager) are handled automatically.  Zeroconf device discovery is available in connection settings. |

All platforms share the same XML configuration format.  The configuration file is stored in the platform's standard application data directory.

### Kiosk and embedded deployment

Two command-line flags simplify locked-down installations (see [Command-line parameters](#commandlineparameters)):

- `--noconfigui` hides the About, Settings, and Connection buttons so the user can only interact with the scene view.
- `--noupdates` disables the startup update check, required in network-restricted environments.

On iOS, the app can be distributed via TestFlight or an MDM solution and configured at first launch; subsequent launches restore the persisted XML config automatically.


<a name="commandlineparameters" />

## Command-line parameters

Umsci has a set of command-line parameters.  Parameters are passed directly when launching the executable from a terminal or as part of a launch script.

| Parameter | Description |
|:----------|:------------|
| `--noupdates` | Disables the automatic online update check performed by `JUCEAppBasics::WebUpdateDetector` at startup.  Useful in network-restricted environments, automated deployments, or kiosk setups where outbound HTTP requests to GitHub should be avoided. |
| `--noconfigui` | Hides the three configuration buttons (About, Settings, Connection) in the upper-left corner of the UI.  Intended for kiosk or embedded deployments where the user should not be able to change settings or disconnect from the soundscape signal engine. |


<a name="code-architecture" />

## Code architecture

This section is aimed at developers who want to understand, extend, or adapt the codebase.

<a name="component-stack" />

### Component stack

The visual output is produced by three `UmsciPaintNControlComponentBase`-derived components held by `UmsciControlComponent`, all given identical pixel bounds:

```
UmsciControlComponent
  ├── UmsciLoudspeakersPaintComponent           (bottom — display only)
  ├── UmsciSoundobjectsPaintComponent           (middle — draggable source circles)
  └── UmsciUpmixIndicatorPaintNControlComponent (top — ring + transform handles)
```

All three inherit `UmsciPaintNControlComponentBase` which provides:
- Coordinate transforms between pixel space and real-world DS100 coordinates (`GetPointForRealCoordinate` / `GetRealCoordinateForPoint`).
- Zoom/pan state (`m_zoomFactor`, `m_zoomPanOffset`) driven by mouse wheel, trackpad pinch, and on iOS a native `UIPinchGestureRecognizer` (see below).
- A `setZoom()` / `resetZoom()` API so that `UmsciControlComponent` can keep all three layers synchronised via the `onViewportZoomChanged` callback.
- A virtual `onZoomChanged()` hook that derived classes override to re-trigger their prerender pass before repaint.

`UmsciUpmixIndicatorPaintNControlComponent` also inherits `JUCEAppBasics::TwoDFieldBase`, which supplies per-channel angle and label data for any `juce::AudioChannelSet` (the chosen surround format).

`hitTest()` is overridden on the sound-objects and upmix-indicator layers to return `true` only over interactive elements (source circles, ring arcs, handles).  The large empty area of both layers is transparent to mouse/touch events, which pass through to the layer below.

### Upmix indicator geometry

The ring geometry is prerendered into `juce::Path` objects by `PrerenderUpmixIndicatorInBounds()` whenever the bounds, zoom, or any transform parameter changes.  The five transform parameters are:

| Member | Default | Meaning |
|:-------|:--------|:--------|
| `m_upmixRot` | `0.0` | Ring rotation around Z.  0 = front, positive = clockwise (normalised 0–1 = 0–360°). |
| `m_upmixTrans` | `1.0` | Floor ring radial scale factor. |
| `m_upmixHeightTrans` | `0.6` | Height ring radius as a fraction of floor ring radius. |
| `m_upmixAngleStretch` | `1.0` | Front/rear angular spread compression (1.0 = uniform). |
| `m_upmixOffsetX/Y` | `0.0` | Ring centre offset in units of base radius. |

<a name="zoom-and-pan-internals" />

### Zoom and pan internals

Zoom state lives in `UmsciPaintNControlComponentBase`:

- `m_zoomFactor` — float, 1.0 = no zoom, clamped to [0.1, 10.0].
- `m_zoomPanOffset` — `juce::Point<float>`, normalised fraction of base content width/height; resize-stable.

`getContentBounds()` applies the current zoom and pan to produce the pixel rectangle that content is rendered into.  `mouseWheelMove()` and `mouseMagnify()` handle desktop input.  Both call `onZoomChanged()` on the component then fire the `onViewportZoomChanged` callback which `UmsciControlComponent` uses to propagate the new zoom state to the other two sibling layers via `setZoom()` (which is idempotent and does not re-fire the callback).

A `simulatePinchZoom(float scaleFactor, juce::Point<float> centre)` public method provides the same zoom path for programmatic callers (used by the iOS native pinch path described below).

<a name="ios-native-touch-handling" />

### iOS native touch handling

JUCE 8 routes each finger touch independently to whichever component passes `hitTest()` at that touch position.  Because both the sound-objects and upmix-indicator layers have large `hitTest()` dead zones, the two fingers of a pinch gesture often land on different components (or no component), preventing JUCE's `mouseMagnify()` synthesis from firing.

`UmsciControlComponent::parentHierarchyChanged()` therefore attaches a `UIPinchGestureRecognizer` (via `JUCEAppBasics::iOS_utils::registerNativePinchOnView()`) directly to the JUCE peer's UIView the first time a peer becomes available.  This gesture recognizer operates at the UIKit layer before JUCE's per-component touch routing, so it fires regardless of which JUCE components the individual fingers hit.  The incremental scale and centre point are forwarded to `simulatePinchZoom()` on the loudspeakers layer, which fires `onViewportZoomChanged` and synchronises all three sibling layers normally.

Safe-area insets (status bar, home indicator, Dynamic Island, Stage Manager) are managed by `JUCEAppBasics::iOS_utils::initialise()` / `getDeviceSafetyMargins()`.  The implementation uses `UIWindowScene.windows.firstObject` (the main app window) rather than the key window, so safe-area queries remain correct when a modal dialog (Alert, connection settings) is open and temporarily becomes the key window.

<a name="configuration-persistence" />

### Configuration persistence

`UmsciAppConfiguration` wraps JUCE's `XmlDocument`-based config file.  Classes that need to read/write settings inherit either `UmsciAppConfiguration::Dumper` (write) or `UmsciAppConfiguration::Watcher` (read-on-change).  `MainComponent` implements both.

Calling `m_config->triggerConfigurationDump()` serialises the current state; any change on disk automatically calls `onConfigUpdated()` on all registered Watchers.

Persisted settings include: OCP.1 connection parameters, look-and-feel, control colour, control format, control size, upmix transform (rotation, translation, height, stretch, offset X/Y), upmix shape, upmix source start ID, upmix live mode, show-all-sources flag, and MIDI assignments.

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

The `m_inhibitFlashCount` counter absorbs OCP.1 echo-backs (the DS100 reflects each `SetObjectValue` back as a notification).  Without it, each echo would call `updateFlashState()`, which detects a mismatch between rendered and live positions and starts the flash animation — producing visible flicker during MIDI control.

MIDI assignments are stored as `JUCEAppBasics::MidiCommandRangeAssignment` objects and serialised to hex strings in the XML config under the `<ExternalControlConfig>` element.
