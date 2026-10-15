# AI overlay checkpoint — 15 October 2026

Branch: `experiment/ai-viewport`. Existing changes were preserved. Neither
`C:\Projects\robotdolly-sidecar` nor `C:\Projects\robotdolly-sidecar-test`
was edited or used to run these tests. Neural processing and the zero-motion
fix are unchanged. No sliders or installer packaging were started.

## Backup

`C:\Projects\robotdolly-overlay-backup-20261015-085147`

Contains the initial HEAD, status, binary working-tree patch, existing modified
and untracked files, originals of files edited here, and both existing viewer
executables. To undo this task, close the viewer and restore the seven edited
files from their matching paths in this backup (see `git diff --stat`). Restore
`build-vc170-64\newview\RelWithDebInfo\secondlife-bin.exe` from the backup
if needed. Do not use a blanket git reset: the original viewport.cpp edit and
untracked workspace files belong to the pre-existing state.

## Changes

- Uses `getWorldViewRectRaw()`, also used by the yellow diagnostic at
  `llviewerwindow.cpp:3427`. Converts bottom-left viewer coordinates to top-left
  Windows client pixels; UI scale is not applied a second time.
- Places the captured whole window at the visible Windows frame origin and
  clips to the 3D viewport. The duplicate captured title bar is outside the clip.
- Applies layered, mouse-transparent, non-activating styles only to the
  `SidecarOverlay` window owned by the renderer this viewer launched.
- Updates geometry on the viewer UI loop and masks the overlay while minimized,
  in the background, or waiting for a resized capture.
- Relaunches the same configured renderer after the new window size is stable
  for 600 ms. The existing renderer's capture and GPU resources are fixed-size;
  relaunching avoids changing that working implementation. Neural warm-up repeats.
- Preserves Ctrl+Alt+D hide. If resized while hidden, capture restarts when shown.
- Preserves the existing C++20 UTF-8 path fix and supports the C++17 test build.

## Validation completed

- Viewer `secondlife-bin` RelWithDebInfo build: success, zero warnings/errors.
  Used the existing built dependencies; no installer target was built.
- Windows fake-renderer tests: passed. Includes lifecycle, dependency failures,
  passthrough rejection, geometry/clip offsets, click-through styles, movement,
  minimize/restore, resize, maximize/restore, and emergency hide preservation.
- `git diff --check`: passed.

## Manual test

1. Close other viewer instances. Launch this newly built executable:
   `C:\Projects\robotdolly-viewer\build-vc170-64\newview\RelWithDebInfo\secondlife-bin.exe`.
   The existing `SecondLifeViewer.exe` and installed shortcuts were not updated.
2. Enable AI with your already-configured renderer. Verify the neural effect
   still appears after warm-up, fills the actual 3D viewport, and has one title bar.
3. Click menus, floating controls, camera controls, and Graphics preferences while
   AI is visible. Check hover, dragging, wheel input, and keyboard focus.
4. Move, resize, maximize, restore, minimize, and Alt-Tab. Resizing briefly exposes
   the normal viewer, then repeats renderer startup/warm-up automatically.
5. Toggle Ctrl+Alt+D both ways; also resize while hidden, then show again.
6. Change workspace panel sizes and UI scale. If possible, move between monitors
   with different Windows scaling. Check viewport edges and image registration.
7. Disable AI and close the viewer; confirm its renderer exits.

Real NVIDIA output, DirectComposition mouse delivery, mixed-DPI monitors, and
live dragging still require these manual checks. This remains a clipped external
overlay: floating UI inside the viewport is still present in the captured image.
Full neural composition before viewer UI is a later integration step.
