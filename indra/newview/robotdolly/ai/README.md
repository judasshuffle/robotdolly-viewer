# AI window enhancement prototype

This change adds a **managed external renderer** to Graphics preferences. It is
not a native OpenGL neural-rendering pass, and the viewer installer does not
include an AI model, renderer executable or NVIDIA runtime. It cannot enhance
anything by itself. No GPU inference has been tested by this change's CI.

## Current behavior

Choose a configured `wowsidecar.exe` once in Graphics preferences, then use
**AI window enhancement (experimental)** to start or stop it. The viewer owns
that process, stops it when disabled, and uses a Windows job to terminate it
if the viewer closes unexpectedly. It refuses multiple matching viewer windows
so it does not capture the wrong session. It does not launch NeuralScreen.

The renderer still captures the **whole main window**. The viewer now positions
that capture at the visible Windows frame origin and clips its overlay to
`getWorldViewRectRaw()`, the same raw-pixel rectangle used by the yellow
`RD 3D viewport` diagnostic in `llviewerwindow.cpp`. This removes the captured
title bar and surrounding UI from the visible overlay without altering neural
processing. Floating controls inside the viewport are still part of the capture.
The overlay is layered, transparent to mouse input, and does not take focus
([Windows layered-window behavior](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features)).

Geometry follows the viewer UI update loop. Minimize and loss of foreground
mask the overlay; Ctrl+Alt+D remains owned by the sidecar and is not overridden.
The patched backend has fixed-size capture resources, so a window size change
temporarily masks the overlay and relaunches the same configured renderer after
600 ms of stable size. Neural warm-up repeats after this restart. A hidden overlay
is not restarted until the user shows it again. The existing pop-out is not selected.
The external renderer handles GPU detection and capture. A renderer that exits, reports an
error or falls back to passthrough is stopped and the preference resets to off.
The setting is session-only; failed initialization cannot repeat at every login.
Status is shown below the checkbox; hover it for the complete message.

The process reporting frames does not prove that the NVIDIA add-on changed the
image. The UI deliberately says to verify the effect visually. There is no
strength slider in this prototype; the renderer's own saved configuration is
used. Its folder must be writable for its log and ReShade configuration.

## Compatible backend

The control protocol is from `xilla420/dlss5-wow-sidecar`, revision
`296b67ecc93dd2fec3563355a8c4c48d0a23b0d6` (0.5.0 source). `sidecar_control.h`
is an unmodified copy of its `src/common/core/ControlChannel.h`; its MIT notice
is in `SIDECAR-LICENSE.txt` and in the viewer's Windows licence notices.
Other backend revisions need protocol verification before use.

The existing renderer requires ReShade (`dxgi.dll`), the RenoDX DLSS 5 add-on,
and a compatible `nvngx_dlssnr.dll` in its folder. Their licences are separate.
We neither bundle nor download those files. Its RTX 4090 route requires an
Ada-compatible runtime; working in NeuralScreen does **not** establish that
the same DLL works in this backend. The upstream backend currently restricts
its GPU matrix to RTX 40/50; support for lesser cards is **not implemented**.
Building that backend also needs the DLSS and D3D12 Optical Flow SDKs. None of
these requirements is satisfied by compiling this viewer control.

## Validation and remaining work

The Windows control tests use a fake renderer and cover startup with spaces in
paths, target ambiguity, missing dependencies, status exchange, shutdown,
passthrough rejection, frame/viewport alignment, click-through styles, movement,
minimize/restore, resize/maximize relaunch, and preserving emergency hide.
They require an interactive Windows desktop and briefly focus test windows.
They do not test actual mouse delivery through DirectComposition or the GPU pipeline. The Linux
test covers unsupported-platform refusal. The dedicated GitHub workflow also
attempts an open-source Windows viewer build; a passing control test alone is
not a passing viewer build.

Before considering this feature complete:

- Preserve the user's already-working patched backend/runtime on the RTX 4090.
- Verify capture, clicks, resizing, minimizing, Alt-Tab, shutdown and failures
  with the real viewer; examine UI placement at different UI scales.
- Replace external setup with a permitted, packaged backend for a one-install
  experience. The current renderer chooser is only a prototype boundary.
- For world-only enhancement, implement GPU texture exchange before viewer UI
  composition instead of capturing a finished window.
- Add in-viewer effect strength and explicit inference-success telemetry.

The pop-out debug message in the branch's base commit is left unchanged; this
change does not claim to fix that crash.
