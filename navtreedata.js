/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Umsci", "index.html", [
    [ "Umsci Changelog", "md_CHANGELOG.html", [
      [ "Table of contents", "index.html#autotoc_md45", null ],
      [ "Overview", "index.html#autotoc_md46", null ],
      [ "Getting started", "index.html#autotoc_md47", [
        [ "Step 1 — Configure the connection to the signal engine", "index.html#autotoc_md49", null ],
        [ "Step 2 — Connect and verify", "index.html#autotoc_md51", null ],
        [ "Step 3 — Read the scene", "index.html#autotoc_md53", null ],
        [ "Step 4 — Set up and adjust the upmix indicator", "index.html#autotoc_md55", [
          [ "4a — Choose the channel format and first soundobject", "index.html#autotoc_md56", null ],
          [ "4b — Align the ring to the room", "index.html#autotoc_md57", null ],
          [ "4c — Commit the positions (Manual mode) or work live", "index.html#autotoc_md58", null ]
        ] ]
      ] ],
      [ "Use cases", "index.html#autotoc_md60", [
        [ "Upmix monitoring and alignment", "index.html#autotoc_md61", null ],
        [ "General soundobject position monitoring and editing", "index.html#autotoc_md62", null ],
        [ "Kiosk / embedded control surface", "index.html#autotoc_md63", null ],
        [ "iOS / iPadOS mobile control", "index.html#autotoc_md64", null ]
      ] ],
      [ "Umsci - app functionality", "index.html#autotoc_md66", [
        [ "Main Umsci UI", "index.html#autotoc_md67", [
          [ "Upmix indicator handles", "index.html#autotoc_md68", null ],
          [ "Zoom and pan", "index.html#autotoc_md69", null ],
          [ "Control modes", "index.html#autotoc_md70", null ]
        ] ],
        [ "Side panels", "index.html#autotoc_md71", [
          [ "dbpr project panel", "index.html#autotoc_md72", null ],
          [ "Upmix spread & delay-mode panel", "index.html#autotoc_md73", null ],
          [ "Snapshot panel", "index.html#autotoc_md74", null ]
        ] ],
        [ "Umsci settings menu", "index.html#autotoc_md76", null ],
        [ "Connection settings", "index.html#autotoc_md77", null ],
        [ "Upmix control settings", "index.html#autotoc_md78", null ],
        [ "External control (MIDI)", "index.html#autotoc_md79", [
          [ "MIDI input device", "index.html#autotoc_md80", null ],
          [ "Parameter assignments", "index.html#autotoc_md81", null ]
        ] ]
      ] ],
      [ "Platform support", "index.html#autotoc_md82", [
        [ "Kiosk and embedded deployment", "index.html#autotoc_md83", null ]
      ] ],
      [ "Command-line parameters", "index.html#autotoc_md84", null ],
      [ "Code architecture", "index.html#autotoc_md85", [
        [ "Component stack", "index.html#autotoc_md86", null ],
        [ "Upmix indicator geometry", "index.html#autotoc_md87", null ],
        [ "Zoom and pan internals", "index.html#autotoc_md88", null ],
        [ "iOS native touch handling", "index.html#autotoc_md89", null ],
        [ "Configuration persistence", "index.html#autotoc_md90", null ],
        [ "MIDI control internals", "index.html#autotoc_md91", null ],
        [ "dbpr project loading internals", "index.html#autotoc_md92", null ]
      ] ],
      [ "[Unreleased]", "md_CHANGELOG.html#autotoc_md94", [
        [ "Added", "md_CHANGELOG.html#autotoc_md95", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md96", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md97", null ]
      ] ],
      [ "[0.4.2] 2026-05-13", "md_CHANGELOG.html#autotoc_md98", [
        [ "Added", "md_CHANGELOG.html#autotoc_md99", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md100", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md101", null ]
      ] ],
      [ "[0.4.0] 2026-05-10", "md_CHANGELOG.html#autotoc_md102", [
        [ "Added", "md_CHANGELOG.html#autotoc_md103", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md104", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md105", null ]
      ] ],
      [ "[0.3.1] 2026-04-26", "md_CHANGELOG.html#autotoc_md106", [
        [ "Added", "md_CHANGELOG.html#autotoc_md107", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md108", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md109", null ]
      ] ],
      [ "[0.3.0] 2026-04-12", "md_CHANGELOG.html#autotoc_md110", [
        [ "Added", "md_CHANGELOG.html#autotoc_md111", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md112", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md113", null ]
      ] ],
      [ "[0.2.4] 2026-03-26", "md_CHANGELOG.html#autotoc_md114", [
        [ "Added", "md_CHANGELOG.html#autotoc_md115", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md116", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md117", null ]
      ] ],
      [ "[0.2.3] 2026-03-22", "md_CHANGELOG.html#autotoc_md118", [
        [ "Added", "md_CHANGELOG.html#autotoc_md119", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md120", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md121", null ]
      ] ],
      [ "[0.2.2] 2026-03-16", "md_CHANGELOG.html#autotoc_md122", [
        [ "Added", "md_CHANGELOG.html#autotoc_md123", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md124", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md125", null ]
      ] ],
      [ "[0.2.1] 2026-03-09", "md_CHANGELOG.html#autotoc_md126", [
        [ "Added", "md_CHANGELOG.html#autotoc_md127", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md128", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md129", null ]
      ] ],
      [ "[0.2.0] 2026-03-06", "md_CHANGELOG.html#autotoc_md130", [
        [ "Added", "md_CHANGELOG.html#autotoc_md131", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md132", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md133", null ]
      ] ],
      [ "[0.1.0] 2026-03-02", "md_CHANGELOG.html#autotoc_md134", [
        [ "Added", "md_CHANGELOG.html#autotoc_md135", null ],
        [ "Changed", "md_CHANGELOG.html#autotoc_md136", null ],
        [ "Fixed", "md_CHANGELOG.html#autotoc_md137", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"AboutComponent_8cpp.html",
"classUmsciAppConfiguration.html#a5e67616c3aac0b239721e86c4837ad61aa782395a7dc1d11e840171a4a7c1ab39",
"classUmsciSnapshotComponent.html#adef65fc55c2bf94f3b71c94509ae1da8",
"structDeviceController_1_1RemObjAddr.html#a7f3c48cc16d21f3c99da5ac530259b0e"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';