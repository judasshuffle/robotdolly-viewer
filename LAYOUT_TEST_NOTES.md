# Workspace layout persistence — 15 October 2026

The Blender-style workspace previously lived only in memory. It now saves the
split tree, divider proportions and panel assignments to
`%APPDATA%/ROBOTDOLLYViewer/user_settings/robotdolly-layout.txt`.

Changes save after panel selection, splitting/joining, divider release and
closing a hosted panel. AREA > SAVE LAYOUT explicitly saves the current whole
workspace and shows LAYOUT SAVED (or SAVE FAILED). This stores one last-used
layout; named presets and floating-window arrangements are outside this change.

The file uses versioned stable panel identifiers, retains exact float ratios,
and is replaced only after its temporary file is closed successfully. Invalid,
oversized or unsupported layouts fall back to the default without overwriting
the original merely on startup. Restore rejects duplicate singleton panels.
Session-dependent panels are restored after login, and shutdown visibility
changes cannot remove saved panels.

## Verification

- RelWithDebInfo viewer build passed; no installer packaging.
- Standalone CMake/CTest layout_roundtrip passed: nested/non-contiguous trees,
  exact ratios, stable identifiers, root-only layout, malformed input, bounds,
  cycles and unchanged destination on failed parse.
- Live automatic save recorded the user's World + Inventory upper row and
  full-width Conversations below, including both divider proportions.
- Clicking AREA > SAVE LAYOUT displayed LAYOUT SAVED and wrote the expected tree.
- Relaunch exposed legacy Inventory creation during the login screen. The
  existing Chat/Inventory hosting path now waits for STATE_STARTED too; rebuilt.
- After that fix, the viewer reopened and logged in successfully, restoring
  World left, Inventory right and Conversations below at the saved proportions.

Backup of previous source/executable and the user's saved three-pane arrangement:
`C:/Projects/robotdolly-layout-backup-20261015-135856`.

Manual follow-up: change panel types and dividers, restart, and check their
restoration after login. Also try People/Groups, Mini-map and Camera panels.
Neural rendering, automatic discovery and sidecar directories are unchanged.
