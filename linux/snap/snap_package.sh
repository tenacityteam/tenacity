#!/usr/bin/env bash
# build the tenacity snap. .snap ends up next to this script.
set -euo pipefail

cd "$(dirname "$(readlink -f "$0")")"

# snapcraft usually lives in /snap/bin, which isn't always in $PATH
# (sudo, cron, non-login shells). add it before checking.
case ":$PATH:" in
   *:/snap/bin:*) ;;
   *) PATH="/snap/bin:$PATH" ;;
esac

if ! command -v snapcraft >/dev/null 2>&1; then
   echo "snapcraft not found. install it with: sudo snap install snapcraft --classic" >&2
   exit 1
fi

sudo snapcraft pack "$@"
