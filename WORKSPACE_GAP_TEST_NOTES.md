# Reclaim unused space above workspace — 15 October 2026

The workspace reserved a fixed 110 logical pixels below the window top. Its
rectangle now follows the actual world placeholder beneath the menu/navigation
bars and reserves the compact location bar when visible. This updates with
window and UI layout changes; split proportions and saved panel choices remain
unchanged. The renderer still receives its viewport from the workspace rectangle.

Verification:
- RelWithDebInfo viewer build succeeded; git diff --check passed.
- Logged-in inspection showed the workspace immediately below navigation,
  recovering approximately 57 displayed pixels in the current configuration.
- Saved World/Groups/Conversations arrangement restored.
- Clicked the notification arrow: its floating Notifications panel opened
  successfully. No notification or floating-panel controls were removed.

Backup before editing: C:/Projects/robotdolly-gap-backup-20261015-142653.
Neural processing, Sidecar configuration/discovery, pop-out experiment and
installer packaging were not changed. The sidecar-test directory was untouched.

Manual follow-up: check resizing, maximising and alternate navigation/compact
location bar preferences. Check AI overlay alignment at your usual intensity.
