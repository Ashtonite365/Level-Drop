# Level Drop

By **Ashtonite365**.

Drop level files into a folder and they show up in **My Levels** automatically.

- Watches `%LOCALAPPDATA%\GeometryDash\level-drop\` by default. You can change it in the mod settings.
- Imports `.gmd` files (GDShare format) and `.txt` files that hold a raw level string.
- Each file becomes a new local level at the top of the list.
- With **Replace same-name levels** on (the default), older local levels with the same name are deleted. Turn it off to keep every copy.
- Levels named from the file name (`.txt`) drop a trailing ` vN` (e.g. `Orbit Lullaby v3`). `.gmd` files keep their own level name.
- Imported files move to `imported\`. Failed ones move to `failed\` along with a `.log`.
- It scans when the game starts, when you open Create or My Levels, and never while the editor is open.

- **Save** button on the My Levels screen (under the search button) writes your levels to disk immediately without closing the game.
- **Auto-save after import** (on by default) saves right after each import, so imports aren't lost if the game crashes.

Requires **GMD API** (hjfod.gmd-api).

Built with Grokest Bot.
