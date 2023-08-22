#!/bin/sh

lib="${0%/*}/lib/tenacity"
share="${0%/*}/share/tenacity"

export LD_LIBRARY_PATH="${lib}:${LD_LIBRARY_PATH}"
export TENACITY_MODULES_PATH="${TENACITY_MODULES_PATH}:${lib}/modules"
export TENACITY_PATH="${TENACITY_PATH}:${share}"

exec "${0%/*}/bin/tenacity" "$@"
