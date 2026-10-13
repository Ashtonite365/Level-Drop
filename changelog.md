# v1.2.0
- Added **Replace same-name levels** (on by default). An imported level replaces any existing local levels with exactly the same name. The old copies are deleted with GD's own delete, and the notification says how many were removed.
- `.txt` imports (named from the file name) drop a trailing ` vN`, so `Orbit Lullaby v3.txt` becomes "Orbit Lullaby". `.gmd` files keep the name stored in the file.
- Added **Auto-save after import** (on by default) and a **Save** button on My Levels.
- Folder watching is unchanged.

# v1.0.0
- Initial release (as Level Drop): drop-folder auto-import of `.gmd` and level-string `.txt` files as new local levels, with imported/failed sorting and notifications.
