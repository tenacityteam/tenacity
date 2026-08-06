#!/usr/bin/env python3
# Writes Tenacity.DS_Store for the DragNDrop DMG layout, called from
# cmake-modules/tenacity/Package.cmake at configure time. Uses ds_store +
# mac_alias so no Finder scripting is needed (TCC blocks osascript on CI).
#
# Usage: make_ds_store.py <output-path>
#
# Change the layout constants below; the next `cmake` run picks them up.

import struct
import sys
from pathlib import Path

from ds_store import DSStore
from mac_alias import Alias

REPO_ROOT = Path(__file__).resolve().parents[3]
BACKGROUND = REPO_ROOT / "mac/Resources/Tenacity-DMG-background.tiff"
BG_ALIAS = Alias.for_file(str(BACKGROUND)).to_bytes()

BUNDLE_NAME = "Tenacity"
WINDOW_ORIGIN = (400, 100)          # x, y of window top-left
WINDOW_SIZE = (600, 458)            # width, height
ICON_SIZE = 72
TEXT_SIZE = 12
BUNDLE_ICON_POS = (170, 350)
APPLICATIONS_ICON_POS = (430, 350)

if len(sys.argv) != 2:
    sys.exit("usage: make_ds_store.py <output-path>")
output = Path(sys.argv[1])
output.parent.mkdir(parents=True, exist_ok=True)

with DSStore.open(str(output), "w+") as ds:
    # "." keys apply to the container window; filename keys apply per-item.

    # icvp: modern icon-view properties bplist. Covers view mode, arrangement,
    # icon/text size, and the background image in one record. Finder also
    # emits the legacy icvo/icvt/icvl/BKGD/pBBk blobs alongside it, but Finder
    # has preferred icvp since 10.7 and our minimum target is 10.15, so
    # omitting them costs nothing.
    ds["."]["icvp"] = ("bplist", {
        "viewOptionsVersion": 1,
        "arrangeBy": "none",
        "iconSize": float(ICON_SIZE),
        "textSize": float(TEXT_SIZE),
        "labelOnBottom": True,
        "showItemInfo": False,
        "showIconPreview": True,
        "gridSpacing": 100.0,
        "gridOffsetX": 0.0,
        "gridOffsetY": 0.0,
        "backgroundType": 2,        # 2 = picture (0 = default, 1 = color)
        "backgroundImageAlias": BG_ALIAS,
    })

    # bwsp: browser window state plist. Bounds string uses NSRect syntax
    # "{{x, y}, {w, h}}" with the origin at the top-left of the window.
    ds["."]["bwsp"] = ("bplist", {
        "WindowBounds": "{{%d, %d}, {%d, %d}}" % (*WINDOW_ORIGIN, *WINDOW_SIZE),
        "ShowSidebar": False,
        "ShowStatusBar": False,
        "ShowPathbar": False,
        "ShowToolbar": False,
        "ShowTabView": False,
        "SidebarWidth": 0,
    })

    # Iloc: uint32 x, uint32 y, 8 * 0xFF (no label offset -> Finder centers).
    # No bplist equivalent for icon positions -- format unchanged since HFS+.
    def iloc(pos_tuple):
        return ("blob", struct.pack(">II", *pos_tuple) + b"\xff" * 8)

    ds[f"{BUNDLE_NAME}.app"]["Iloc"] = iloc(BUNDLE_ICON_POS)
    ds["Applications"]["Iloc"] = iloc(APPLICATIONS_ICON_POS)

print(f"wrote {output}")
