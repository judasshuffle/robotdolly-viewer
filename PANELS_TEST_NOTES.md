# Panels menu batch — 15 October 2026

Built on `experiment/ai-viewport`. Only `indra/newview/llviewerwindow.cpp`
was changed for this batch. AI overlay code and both sidecar folders are unchanged.

The shared panel catalogue now drives labels, category menus and floater lookup.
New choices: People, Friends, Groups, Mini-map, World Map and Camera.
Chat is labelled Conversations and opens the existing conversation container,
including group chats. Snapshot is excluded at the user's request.

People/Friends/Groups share one native floater; choosing one moves that panel
instead of creating duplicate instances. New docked panels restore their original
parent and rectangle when replaced or joined away. Camera gets its normal close
cleanup. The existing world rectangle and AI integration are unchanged.

## Validation

RelWithDebInfo viewer build passed with zero warnings/errors; `git diff --check`
passed. No installer was built. Live logged-in docking behavior needs manual testing.

Launch `build-vc170-64/newview/RelWithDebInfo/secondlife-bin.exe` from this project
after closing the old viewer. Existing installed shortcuts are not updated.

- Split an area and choose each new panel from its category.
- Resize areas; check controls remain usable, especially in narrow areas.
- Switch People/Friends/Groups and confirm the requested tab opens.
- Open a group chat through Groups and use it in Conversations.
- Close, replace and join away new panels; reopen from the regular viewer menus.
- Test camera controls and maps while the AI overlay is enabled.

## Rollback

The immediately preceding working overlay build and original `llviewerwindow.cpp`
are saved in `C:\Projects\robotdolly-panels-backup-20261015-111334`.
Close the viewer, then restore that source file to `indra/newview/` and the backed-up
`secondlife-bin.exe` to `build-vc170-64/newview/RelWithDebInfo/` to undo this batch.
The backup also contains the pre-existing overlay working-tree patch.
