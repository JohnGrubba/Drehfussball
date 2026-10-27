#!/usr/bin/env python3
"""Inline the dashboard sources into dist/index.html and src/site.cpp.

Edit index.html / styles.css / app.js, then run this script (or start
server.py, which runs it automatically).  After that, PlatformIO can flash the
new dashboard with the firmware.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
SRC_CPP = REPO / "src" / "site.cpp"
DIST = ROOT / "dist"


def build(verbose: bool = True) -> int:
    html = (ROOT / "index.html").read_text(encoding="utf-8")
    css = (ROOT / "styles.css").read_text(encoding="utf-8")
    js = (ROOT / "app.js").read_text(encoding="utf-8")

    if '<link rel="stylesheet" href="styles.css">' not in html:
        raise RuntimeError("index.html: stylesheet link not found")
    if '<script src="app.js"></script>' not in html:
        raise RuntimeError("index.html: app.js script tag not found")

    html = html.replace(
        '<link rel="stylesheet" href="styles.css">',
        f"<style>\n{css}\n</style>",
    )
    html = html.replace(
        '<script src="app.js"></script>',
        f"<script>\n{js}\n</script>",
    )

    # The ESP32 embeds the page as a C++ raw string literal.
    if ')rawliteral"' in html:
        raise RuntimeError('dashboard contains the C++ raw-string delimiter')

    DIST.mkdir(parents=True, exist_ok=True)
    (DIST / "index.html").write_text(html, encoding="utf-8")

    cpp = (
        '#include <Arduino.h>\n\n'
        '#include "site.h"\n\n'
        'const char index_html[] PROGMEM = R"rawliteral(\n'
        f"{html}\n"
        ')rawliteral";\n\n'
        'const size_t index_html_len = sizeof(index_html) - 1;\n'
    )
    SRC_CPP.write_text(cpp, encoding="utf-8")

    if verbose:
        print(f"dashboard: {len(html):,} bytes inline")
        print(f"wrote {DIST / 'index.html'}")
        print(f"wrote {SRC_CPP}")
    return len(html)


if __name__ == "__main__":
    build()
