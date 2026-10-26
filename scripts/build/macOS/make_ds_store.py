#!/usr/bin/env python3
# Writes Tenacity.DS_Store for the DragNDrop DMG layout, called from
# cmake-modules/tenacity/Package.cmake at configure time. Uses ds_store +
# mac_alias so no Finder scripting is needed (TCC blocks osascript on CI).
#
# Usage: make_ds_store.py <output-path> <dmg-volume-name> <background-image>
#
# Change the layout constants below; the next `cmake` run picks them up.
#
# Exits 2 if ds_store/mac_alias aren't importable so Package.cmake can tell
# "not installed" apart from "installed but blew up".

import sys
from pathlib import Path

try:
    from ds_store import DSStore
    from mac_alias import (Alias, VolumeInfo, TargetInfo, ALIAS_KIND_FILE,
                           ALIAS_EJECTABLE_DISK, mac_epoch)
except ImportError as e:
    print(e, file=sys.stderr)
    sys.exit(2)

BUNDLE_NAME = "Tenacity"
WINDOW_ORIGIN = (400, 100)          # x, y of window top-left
WINDOW_SIZE = (600, 458)            # width, height
ICON_SIZE = 72
TEXT_SIZE = 12
BUNDLE_ICON_POS = (170, 350)
APPLICATIONS_ICON_POS = (430, 350)


def background_alias(volume_name, filename):
    # Alias to /.background/<filename> on the DMG volume, which is where CPack
    # copies CPACK_DMG_BACKGROUND_IMAGE. Built by hand rather than with
    # Alias.for_file() because the file doesn't exist yet at configure time,
    # and pointing at the copy in the source tree was wrong twice over: the
    # alias only resolved on the build machine, and APFS file IDs above 2^32
    # don't fit the alias record's 32-bit CNID fields, so mac_alias dies with
    # struct.error. With no CNIDs Finder falls back to resolving the path
    # relative to the volume, which is all a DMG needs.
    a = Alias()
    a.volume = VolumeInfo(volume_name.encode(), mac_epoch, b"H+",
                          ALIAS_EJECTABLE_DISK, 0, b"\0\0")
    a.volume.posix_path = f"/Volumes/{volume_name}".encode()
    a.target = TargetInfo(ALIAS_KIND_FILE, filename.encode(), 0, 0, mac_epoch,
                          b"\0\0\0\0", b"\0\0\0\0")
    a.target.folder_name = b".background"
    a.target.posix_path = f"/.background/{filename}".encode()
    a.target.carbon_path = f"{volume_name}:.background:\0{filename}".encode()
    return a.to_bytes()


if len(sys.argv) != 4:
    sys.exit("usage: make_ds_store.py <output-path> <dmg-volume-name> "
             "<background-image>")
output = Path(sys.argv[1])
volume_name = sys.argv[2]
# CPack renames the image to background.<ext> when it copies it in
BG_ALIAS = background_alias(volume_name,
                            "background" + Path(sys.argv[3]).suffix)
output.parent.mkdir(parents=True, exist_ok=True)

with DSStore.open(str(output), "w+") as ds:
    # "." keys apply to the container window; filename keys apply per-item.
    # icvp, bwsp and Iloc all have codecs in ds_store, so they take plain
    # Python values. Wrapping them as ("bplist", ...) / ("blob", ...) only
    # works for codes ds_store doesn't know about; for these it serialises
    # the tuple itself and Finder gets garbage.

    # icvp: modern icon-view properties bplist. Covers view mode, arrangement,
    # icon/text size, and the background image in one record. Finder also
    # emits the legacy icvo/icvt/icvl/BKGD/pBBk blobs alongside it, but Finder
    # has preferred icvp since 10.7 and our minimum target is 10.15, so
    # omitting them costs nothing.
    ds["."]["icvp"] = {
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
        # Finder ignores the whole icvp record (default icon size, no
        # background) unless the backgroundColor keys are present, even with
        # a picture background. bwsp and Iloc still apply, so it looks like
        # the alias is broken when it isn't.
        "backgroundColorRed": 1.0,
        "backgroundColorGreen": 1.0,
        "backgroundColorBlue": 1.0,
        "backgroundImageAlias": BG_ALIAS,
    }

    # bwsp: browser window state plist. Bounds string uses NSRect syntax
    # "{{x, y}, {w, h}}" with the origin at the top-left of the window.
    ds["."]["bwsp"] = {
        "WindowBounds": "{{%d, %d}, {%d, %d}}" % (*WINDOW_ORIGIN, *WINDOW_SIZE),
        "ShowSidebar": False,
        "ShowStatusBar": False,
        "ShowPathbar": False,
        "ShowToolbar": False,
        "ShowTabView": False,
        "SidebarWidth": 0,
    }

    # Iloc: icon centre as (x, y). No bplist equivalent for icon positions --
    # format unchanged since HFS+.
    ds[f"{BUNDLE_NAME}.app"]["Iloc"] = BUNDLE_ICON_POS
    ds["Applications"]["Iloc"] = APPLICATIONS_ICON_POS

print(f"wrote {output}")
