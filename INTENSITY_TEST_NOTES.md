# AI intensity fix — 15 October 2026

## Cause and fix

Apply kept its `std::ifstream` on `sidecar.toml` alive until the function returned.
MSVC opens that stream without delete sharing, so `MoveFileExW` could not replace
the destination. A regression test reproduced Windows error 5 with the same stream
type. Closing the input stream before replacement resolves that self-imposed lock.

The configuration writer is extracted into `robotdolly/ai/intensity_config.h` so
the exact production implementation can be tested without launching the viewer.
It closes the read handle, edits only the intensity value, writes and flushes a
sibling temporary file, closes it, then replaces the destination. A failed save
preserves the original and returns before renderer shutdown/restart. Temporary
file creation refuses to overwrite an existing file. Existing comments, spacing,
BOM and line endings are retained; decimal formatting is locale-independent.

The existing intentional Apply callback and function declaration are retained.
Successful Apply updates the slider's Cancel baseline. The AI tab labels the
value as a percentage and wraps long status messages, with the full text also
available as a tooltip. Bundled renderer discovery and the existing restart path
are unchanged. No neural processing, viewport, zero-motion, or installer code changed.

## Verification

- RelWithDebInfo viewer build: passed, zero warnings/errors.
- `intensity_config` test: passed. Covers 0/5/50/85/100%, CRLF/LF, missing final
  newline, BOM, comments, unrelated intensity keys, decimal-comma locale, invalid
  values, missing/duplicate assignments, locked/read-only destination, temporary
  file collision and cleanup after failure.
- The same writer passed against a copy of the actual bundled `sidecar.toml`.
- Actual AI Graphics UI: Apply wrote `0.00` and `0.50`; Cancel/reopen retained
  the applied 50% value. File comparisons verified that only intensity changed.
- After Firestorm closed, live Apply at the user's chosen 30% replaced renderer
  PID 38620 with PID 52460. The running executable was the bundled
  `ai-renderer/wowsidecar.exe`, not the known-working sidecar-test installation.
- The new process read `intensity = 0.30` and projected `NRIntensity=0.300000`
  into ReShade.ini. Its log reported the Ada-patched runtime, zero-motion texture
  initialization, neural feature creation at frames 60 and 240, and 600-frame
  telemetry for `reshade-hosted DLSS 5 NR x3`. The user confirmed the effect works.
- Final full-file comparison against the pre-test config: only `1.00` to `0.30`
  changed. Intensity was left at the user's selected 30%, with AI running.

Existing capture ambiguity protection correctly refuses to start while another
viewer with the same window class (Firestorm) is open. That safeguard was retained.

## Backup and scope

`C:\Projects\robotdolly-intensity-backup-20261015-134407` contains the initial
intentional edits, their working-tree patch, previous viewer executable, runtime
configuration and runtime AI panel XML. No files in
`C:\Projects\robotdolly-sidecar-test` were edited or overwritten.

The built test viewer is
`C:\Projects\robotdolly-viewer\build-vc170-64\newview\RelWithDebInfo\secondlife-bin.exe`.
The updated AI panel XML was copied into that build's skins folder for UI testing.
No installer or installed shortcut was updated. The old untracked PowerShell patch
was preserved but should not be reapplied over this fix.
