# Level Drop

By **Ashtonite365**.

Drop level files into a folder and they show up in **My Levels** automatically.

- Watches `%LOCALAPPDATA%\GeometryDash\level-drop\` by default. You can change it in the mod settings.
- Imports `.gmd` files (GDShare format) and `.txt` files that hold a raw level string.
- Each file becomes a **new** local level at the top of the list. Existing levels are never changed or replaced.
- Imported files move to `imported\`. Failed ones move to `failed\` along with a `.log`.
- It scans when the game starts, when you open Create or My Levels, and never while the editor is open.

Requires **GMD API** (hjfod.gmd-api).

Built with Grokest Bot.
