# ROBOT DOLLY Workspace — experimental foundation

This is a *model-only* prototype for the Second Life viewer fork. It is not wired to LLUI, it does **not** yet render SL inside a docked frame, and it has not been built as part of the viewer.

## Guiding principle
**Any area, any registered editor, as many instances as the editor permits.**

### Separate responsibilities
- **Workspace model**: split tree, sizes, area identity, selected editor type and optional context key. It does not know about Second Life networking.
- **Editor registry**: stable editor type IDs, including world, conversations, inventory and landmarks. Additional editors join the registry, without adding branches to the layout engine.
- **UI adapter (future)**: maps the split tree into LLUI-managed, clipped, resizable areas and provides the Blender-like area-type selector, dragging, joining, maximizing and keyboard focus.
- **SL integration (future)**: embeds the existing world renderer, chat and inventory UI, with SL server-side permissions remaining authoritative.
- **Persistence (future)**: versioned user-defined workspace serialization and safe fallback for missing editor types.

### Chat instance context
The `sl.chat` editor can bind to `nearby`, an IM session ID, or a group session ID. Context belongs to an instance, not to a unique chat window. A saved workspace may display several conversations simultaneously.

### Landmarks
`sl.landmarks` is a separate editor type. It can eventually display location shortcuts horizontally or vertically, independently of the world view, without allocating a permanent toolbar across the screen.

### Current status
- Header-only C++17 workspace model; *not yet compiled or integrated into the viewer*.
- Registry supports editor identification, but multiplicity rules are only metadata for now.
- Root area starts with `sl.world`; split, change editor, resize ratio and close area are supported as data operations.
- A standalone smoke test is in `tests/workspace_model_smoke.cpp`.

### Next steps
1. Run the standalone smoke test under Visual Studio or a C++17 compiler.
2. Create an LLUI adapter using `LLLayoutStack`/`LLLayoutPanel` or custom split hosts.
3. Host nearby chat in its own resizable area using the existing conversation UI.
4. Restrict world drawing and input to its allocated rectangle.
5. Add inventory host, landmark host and editor switching.
6. Add persistence, split/join interactions and independent world cameras later.

**No protected assets should be exported from an SL region; world-edit operations remain constrained by SL permissions.**
