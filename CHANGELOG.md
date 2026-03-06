# Umsci Changelog
All notable changes to Umsci will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added

### Changed

### Fixed

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
