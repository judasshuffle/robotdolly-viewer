# ROBOTDOLLY Viewer v1 — private Windows test build

This is an experimental fork of the Second Life viewer, not an official Linden Lab release.

## Identity

- CMake viewer channel: `ROBOTDOLLY Viewer v1`
- Windows installer filename: `ROBOTDOLLY_Viewer_v1_x86_64_Setup.exe`
- Windows installer app display name: `ROBOTDOLLY Viewer v1`
- Viewer settings directory identifier: `ROBOTDOLLYViewer`
- Executable inside the install still uses the upstream name `SecondLifeViewer.exe`.
- The underlying Second Life engine version stays unchanged. The "v1" label is an experimental distribution label, **not** a verified production release.

## Build on the existing developer machine

1. Close ROBOTDOLLY / SecondLifeViewer.exe completely.
2. Pull branch `experiment/workspace-layout`.
3. Reconfigure the existing build (this step is essential because an existing CMake cache can retain the old "Second Life Test" channel):

```bat
cd /d C:\Projects\robotdolly-viewer
git pull
cmake -S C:\Projects\robotdolly-viewer\indra -B C:\Projects\robotdolly-viewer\build-vc170-64 -DVIEWER_CHANNEL="ROBOTDOLLY Viewer v1"
cd build-vc170-64
cmake --build . --config RelWithDebInfo --parallel 2
```

4. Confirm the build creates `ROBOTDOLLY_Viewer_v1_x86_64_Setup.exe`. The generated installer is normally in the build's newview output folder; find the actual file before sharing.
5. Test installation on a separate Windows machine or VM **without** Visual Studio or build tools; check login, 3D world, chat, inventory, split/join, settings location, and uninstallation.

## Before sharing

- Back up regular viewer settings; do not assume installers have been validated merely because compilation succeeded.
- Do not distribute until the installer has been run successfully on a clean machine.
- Check the LGPL and bundled third-party licence/notice obligations; preserve attribution and make required source code available.
- Windows may warn about an unsigned test installer. Do not present it as signed or trusted.
- Keep test releases private until reviewed. A GitHub Releases upload is **not** set up by these source changes.
- Automated updates, dedicated Windows icon, dedicated executable filename, signing, and per-install cache isolation require further work.
