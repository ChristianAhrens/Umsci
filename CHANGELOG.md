# Umsci Changelog
All notable changes to Umsci will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added

### Changed

### Fixed

## [0.3.0] 2026-04-12
### Added
- Added dbpr project panel: load a d&b audiotechnik .dbpr project file via a floating side panel, compare its data against the connected DS100, and sync or clear it with dedicated buttons

### Changed
- Changed upmix indicator snapshot store/recall to a dedicated floating side panel, consistent with the dbpr panel design and accessible at all times without opening a menu

### Fixed
- Fixed connection establishment getting stuck in permanent 'Reading' state: Umsci now retries dropped OCP.1 `getValues` queries so that a connection always reaches the subscribed state
- Fixed stale loudspeaker icon painting after a device change invalidates previously valid speaker positions

## [0.2.4] 2026-03-26
### Added
- Added single upmix indicator state store & recall as dedicated buttons on UI
- Added basic osc external control for upmix rotation, translation, height translation, angle stretch, offset x and offset y

### Changed

### Fixed
- Fixed upmix indicator corruption when using stretch handle to pull >=180deg

## [0.2.3] 2026-03-22
### Added
- Added two finger pinch-zoom support

### Changed

### Fixed
- Fixed 'refit to bounding cube' to scale with Control size settings
- Fixed 'refit to bounding cube' button to not accidentally trigger Upmix modifier rotation
- Fixed iOS native device insets handling (e.g. iPad top-bar overlap) by integrating updated JUCE-AppBasics
- Fixed upmix stretch handle rot/pos in rectangle control mode
- Fixed lower right corner height annotation value display.
- Fixed Connection settings dialog discovered device dropdown cropping

## [0.2.2] 2026-03-16
### Added
- Added commandline parameter --noupdates
- Added commandline parameter --noconfigui
- Added basic midi external control for rot, trans, htrans, anglestretch, offset xy

### Changed
- Changed company website and about dialog link to github pages url
- Changed VS build to statically link vs runtime
- Updated readme

### Fixed

## [0.2.1] 2026-03-09
### Added
- Added upmix control angle stretch parameter
- Added upmix control offset xy parameter
- Added mousewheel / pinch zoom, incl. doubleclick to zoom-and-recenter

### Changed

### Fixed
- Fixed automatic fitting of upmix control indicator to speaker system dimensions

## [0.2.0] 2026-03-06
### Added
- Added support for z-Axis (default 1.2m normal layer, auto-fit to speaker system height for height layer)
- Added support for notarized macOS build
- Added upmix control indicator live update mode
- Added upmix control indicator switching between circular and rectangle shape
- Added button to manually trigger re-fitting the upmix control indicator to speaker system
- Added user setting for control elements size (S, M, L)
- Added filtering for upmix control relevant only or all soundobjects#
- Added support for basic zeroconf discovery of device connection parameters (IP, port, IOsize)
- Added support for persisting upmix control indicator parameters in xml config

### Changed
- Changed upmix control settings to be done in separate dedicated dialog

### Fixed
- Fixed performance when receiving or sending OCP.1 data changes
- Fixed initial startup OCP.1 autoconnect

## [0.1.0] 2026-03-02
### Added
- Added initial v0.1.0 featureset

### Changed

### Fixed
