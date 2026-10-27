#!/usr/bin/env bash
# Packs Prism, the Prism test client, the Qt runtime they need and
# prism-gate.sh into one archive that runs on a stock Ubuntu desktop, such as
# a live USB session: the decision gate then needs no Qt kit and no build on
# the machine under test. See docs/PERFORMANCE.md, "Running the decision
# gate".
#
#   tools/dev/prism-gate-bundle.sh --build-dir ~/qor-build/linux-release \
#       --manifest ubuntu-26.04.1-desktop-amd64.manifest [--qt DIR] [--out qor-gate.tar.gz]
#
# Build on the Ubuntu release the bundle targets. Libraries from the Qt kit
# always go in. System libraries go in only when the target's package
# manifest (the .manifest next to an Ubuntu ISO) lacks their package; the
# graphics driver stack and the C/C++ runtime must come from the target.

set -euo pipefail

build=""
qt=""
manifest=""
out="qor-gate.tar.gz"
while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir) build="$2"; shift 2 ;;
        --qt) qt="$2"; shift 2 ;;
        --manifest) manifest="$2"; shift 2 ;;
        --out) out="$2"; shift 2 ;;
        -h | --help) sed -n '2,15p' "$0"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done
[ -n "$build" ] && [ -n "$manifest" ] || { echo "--build-dir and --manifest are required" >&2; exit 2; }
[ -n "$qt" ] || qt=$(sed -n 's|^Qt6_DIR:PATH=\(.*\)/lib/cmake/Qt6$|\1|p' "$build/CMakeCache.txt")
here=$(cd "$(dirname "$0")" && pwd)
for file in "$build/prism/compositor/prism" "$build/tests/prism-test-client" "$qt/lib/libQt6Core.so.6" "$manifest"; do
    [ -e "$file" ] || { echo "missing: $file" >&2; exit 2; }
done

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
root="$stage/qor-gate"
mkdir -p "$root/prism/compositor" "$root/tests" "$root/qt/lib" "$root/qt/plugins" "$root/qt/qml"
cp "$build/prism/compositor/prism" "$root/prism/compositor/"
cp "$build/tests/prism-test-client" "$root/tests/"
cp "$here/prism-gate.sh" "$root/"

# The platforms Prism and its clients use, nested and on bare metal.
for plugin in platforms/libqeglfs.so platforms/libqwayland.so platforms/libqminimal.so platforms/libqoffscreen.so \
    egldeviceintegrations wayland-graphics-integration-server wayland-graphics-integration-client \
    wayland-shell-integration wayland-decoration-client imageformats platforminputcontexts; do
    mkdir -p "$root/qt/plugins/$(dirname "$plugin")"
    cp -r "$qt/plugins/$plugin" "$root/qt/plugins/$(dirname "$plugin")/"
done
# The QML modules Prism imports from Qt (its own are compiled in).
for module in QtCore QtQml QtQuick QtWayland QtGraphs QtMultimedia QtQuick3D; do
    cp -r "$qt/qml/$module" "$root/qt/qml/"
done
rm -rf "$root/qt/qml/QtQuick/Controls/FluentWinUI3" "$root/qt/qml/QtQuick/Controls/designer" "$root/qt/qml/QtQuick/tooling"

# Every library those need. ldd lists each file's whole dependency tree.
declare -A target=()
while read -r name _; do
    target[${name%:amd64}]=1
done < "$manifest"
mapfile -t binaries < <(find "$root" -type f \( -name '*.so*' -o -perm -u+x \) ! -name '*.sh')
declare -A packages=()
missing=()
while read -r soname path; do
    if [ "$path" = "not" ]; then
        missing+=("$soname (not found here either)")
        continue
    fi
    if [[ "$path" == "$qt/lib/"* ]]; then
        cp -L "$path" "$root/qt/lib/$soname"
        continue
    fi
    package=$(dpkg -S "$path" 2> /dev/null || dpkg -S "$(readlink -f "$path")" 2> /dev/null || dpkg -S "/usr$path" 2> /dev/null || true)
    package=${package%%:*}
    if [ -z "$package" ]; then
        missing+=("$soname (no package)")
    elif [ -z "${target[$package]:-}" ]; then
        # glvnd's libOpenGL only dispatches to the target's driver, so it may
        # come along; the driver stack itself may not.
        case "$soname" in
            libc.so* | libm.so* | libstdc++* | libgcc_s* | libGL* | libEGL* | libgbm* | libdrm* | libvulkan*)
                missing+=("$soname ($package)") ;;
            *)
                cp -L "$path" "$root/qt/lib/$soname"
                packages[$package]=1 ;;
        esac
    fi
done < <(LD_LIBRARY_PATH="$qt/lib" ldd "${binaries[@]}" 2> /dev/null | awk '$2 == "=>" && ($3 ~ /^\// || $3 == "not") { print $1, $3 }' | sort -u)
if [ ${#missing[@]} -gt 0 ]; then
    printf 'The target lacks these, and they cannot be bundled:\n' >&2
    printf '  %s\n' "${missing[@]}" >&2
    exit 4
fi

commit=$(git -C "$here" -c safe.directory='*' rev-parse --short HEAD 2> /dev/null || echo unknown)
bundled="${!packages[*]}"
{
    echo "QOR OS decision-gate bundle"
    echo "Commit: $commit"
    echo "Built: $(date -u +%Y-%m-%dT%H:%MZ) on $(sed -n 's/^PRETTY_NAME="\(.*\)"$/\1/p' /etc/os-release)"
    echo "Qt: $qt"
    echo "Bundled system packages: ${bundled:-none}"
    echo
    echo "Run ./prism-gate.sh --help for usage."
} > "$root/BUNDLE.txt"

tar -C "$stage" -czf "$out" qor-gate
echo "Wrote $out ($(du -h "$out" | cut -f1); $(du -sh "$root" | cut -f1) unpacked)"
echo "Bundled system packages: ${bundled:-none}"
