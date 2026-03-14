![Showreel.001.png](Resources/Documentation/Showreel/Showreel.001.png "Umsci Headline Icons")

See [LATEST RELEASE](https://github.com/ChristianAhrens/Umsci/releases/latest) for available installer packages.

Full code documentation available at [![Documentation](https://img.shields.io/badge/docs-doxygen-blue)](https://ChristianAhrens.github.io/Umsci/)

<a name="toc" />

## Table of contents

* [Overview](#overview)
* [Umsci - app functionality](#Umsci-app-functionality)
  * [Main Umsci UI](#Umsci-ui)
  * [Umsci settings menu](#Umsci-settings-menu)
* [Command-line parameters](#commandlineparameters)


<a name="overview" />

## Overview

Umsci is a small utility to connect to a d&b Soundscape signal engine via OCA/OCP.1 to control soundobject positions in accordance to standard immersive output formats.
The app subscribes to soundobject and loudspeaker position values and displays them on a 2D surface, blended with a dedicated 'Upmix indicator' control element. This allows user interaction to place a visual representation of said immersive output formats on the 2D surface and move the associated soundobject positions to match the immersive output format.

Its sourcecode and prebuilt binaries are made publicly available to enable interested users to experiment, extend and create own adaptations.

Use what is provided here at your own risk!

<a name="Umsci-app-functionality" />

## Umsci - app functionality

<a name="Umsci-UI" />

### Main Umsci UI

![Showreel.002.png](Resources/Documentation/Showreel/Showreel.002.png "Umsci main")

<a name="Umsci-settings-menu" />

### Umsci settings menu

![Showreel.003.png](Resources/Documentation/Showreel/Showreel.003.png "Umsci settings")

<a name="commandlineparameters" />

## Command-line parameters

Umsci has a set of command-line parameters. Parameters are passed directly when launching the executable from a terminal or as part of a launch script.

| Parameter | Description |
|:----------|:------------|
| `--noupdates` |  Disables the automatic online update check performed by `JUCEAppBasics::WebUpdateDetector` at startup. Useful in network-restricted environments, automated deployments, or kiosk setups where outbound HTTP requests to GitHub should be avoided. |
| `--noconfigui` | Hides the three configuration buttons (About, Settings, Connection) in the upper-left corner of the UI. Intended for kiosk or embedded deployments where the user should not be able to change settings or disconnect from the soundscape signal engine. |
