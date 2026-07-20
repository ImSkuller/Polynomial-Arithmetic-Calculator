# Assets

This top-level folder holds real screenshots of the app in use (the main
layout, an arithmetic result, the Statistics panel, the Help modal, etc.)
for the project README and any written report. None are checked in yet -
drop PNG/JPG captures here and link them from the top-level
[README.md](../README.md) once available.

[`web/`](web/) is a different thing entirely: it's the frontend's actual
source (`index.html`, `styles.css`, `app.js`), baked into `polycalc.exe` at
build time by `cmake/EmbedWebAssets.cmake` - see the top-level README's
[Building](../README.md#building) section.
