# Menus above workspace — 15 October 2026

Favorites > More uses the standard menu holder inside main_view. The custom
workspace was added as a root-level sibling above main_view, placing the whole
workspace above that menu holder regardless of the menu's own front ordering.

Reparent the existing menu holder and tooltip view to the same root, above the
workspace. Both already occupy the full main-view rectangle and follow all edges.
Their handlers, coordinate origin, ownership by the root tree, and existing menu
contents are preserved. This corrects drawing and input ordering together.

RelWithDebInfo viewer build and git diff --check passed.
Live check after login: Favorites > More and its tooltip draw above the Groups
pane, with the dropdown entries visible. User confirmed the result.
No renderer, saved layout, Sidecar, or installer code changed.
