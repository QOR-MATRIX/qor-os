#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#
# Runs Prism nested in the current Wayland/X11 session (or WSLg), connects real
# clients and checks on the wire that each protocol Prism provides works.
#
#   tools/dev/prism-protocol-check.sh [build-dir]
#
# Needs: a built prism and lumen-widgets-gallery, wayland-info (wayland-utils)
# and weston's demo clients (weston). Exit status is the number of failures.
set -uo pipefail

BUILD=${1:-$HOME/qtos-build/linux-release}
PRISM="$BUILD/prism/compositor/prism"
GALLERY="$BUILD/platform/lumen-widgets-gallery"
SOCKET=wayland-qtos-check
WORK=$(mktemp -d)
trap 'kill $(jobs -p) 2>/dev/null; rm -rf "$WORK"' EXIT

for tool in "$PRISM" "$GALLERY" wayland-info weston-presentation-shm; do
    command -v "$tool" > /dev/null || { echo "missing: $tool"; exit 100; }
done

"$PRISM" --unlocked --socket "$SOCKET" --size 1280x800 > "$WORK/prism.log" 2>&1 &
for _ in $(seq 100); do grep -q "Prism is running" "$WORK/prism.log" && break; sleep 0.1; done
grep -q "Prism is running" "$WORK/prism.log" || { echo "prism did not start"; cat "$WORK/prism.log"; exit 101; }
export WAYLAND_DISPLAY=$SOCKET

wayland-info > "$WORK/globals.txt" 2>&1
WAYLAND_DEBUG=1 QT_QPA_PLATFORM=wayland "$GALLERY" > /dev/null 2> "$WORK/gallery.log" &
WAYLAND_DEBUG=1 weston-presentation-shm > /dev/null 2> "$WORK/presentation.log" &
sleep 5

failures=0
check() { # name, file, pattern
    if grep -qE "$3" "$2"; then
        printf '  ok    %s\n' "$1"
    else
        printf '  FAIL  %s\n' "$1"
        failures=$((failures + 1))
    fi
}

echo "Globals advertised by Prism:"
for global in xdg_wm_base zxdg_decoration_manager_v1 wp_viewporter wp_fractional_scale_manager_v1 \
              wp_cursor_shape_manager_v1 wp_presentation zwp_idle_inhibit_manager_v1 zwp_text_input_manager_v3; do
    check "$global" "$WORK/globals.txt" "interface: '$global'"
done

echo "Protocol traffic:"
check "xdg-decoration: Qt client gets server-side decorations" "$WORK/gallery.log" \
      "zxdg_toplevel_decoration_v1[@#][0-9]+\.configure\(2\)"
check "fractional-scale: preferred_scale sent to a Qt client" "$WORK/gallery.log" \
      "wp_fractional_scale_v1[@#][0-9]+\.preferred_scale\([0-9]+\)"
check "cursor-shape: Qt client obtains a cursor-shape device" "$WORK/gallery.log" \
      "wp_cursor_shape_manager_v1[@#][0-9]+\.get_pointer\("
check "presentation-time: frames reported as presented" "$WORK/presentation.log" \
      "wp_presentation_feedback[@#][0-9]+\.presented\("

echo "$failures failure(s)"
exit "$failures"
