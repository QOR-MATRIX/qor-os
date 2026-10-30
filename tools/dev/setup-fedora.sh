#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#
# Makes a Fedora Workstation installation a QOR OS development machine: the
# build tools and libraries, NVIDIA's driver signed for Secure Boot, Rust,
# the commercial Qt 6.12 kit, QOR OS and its products built, and Prism run
# with them. docs/BUILDING.md ("Linux on a machine of its own") says how to
# install Fedora on an external SSD first.
#
#   tools/dev/setup-fedora.sh            # every step in order, stopping where a reboot is needed
#   tools/dev/setup-fedora.sh check      # what is in place, changing nothing
#   tools/dev/setup-fedora.sh packages   # build tools and libraries
#   tools/dev/setup-fedora.sh nvidia     # NVIDIA's driver from RPM Fusion, signed for Secure Boot
#   tools/dev/setup-fedora.sh rust       # rustup, with the Rust CI uses
#   tools/dev/setup-fedora.sh qt         # the commercial kit in ~/Qt (a Qt Account sign-in first)
#   tools/dev/setup-fedora.sh build      # QOR OS (linux-release) and the products, without their suites
#   tools/dev/setup-fedora.sh play       # Prism in this session, with the products and the QQ samples
#
# Each step can be run again: what is already in place is left as it is. It
# asks for sudo where it installs system packages, and never prints a
# credential. The Secure Boot step asks for a one-time password of your own,
# which the firmware asks for once at the next boot.
set -euo pipefail

REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
QT_ROOT=${QOR_QT_ROOT:-$HOME/Qt}
QT=$QT_ROOT/6.12.0/gcc_64
RUST=1.98.1
BUILD=$REPO/build/linux-release
PRODUCTS=$REPO/build/products-linux

say() { printf '\n== %s\n' "$*"; }
usage() { sed -n '3,22p' "$0" | sed 's/^# \{0,1\}//'; }

fedora() { [ -r /etc/os-release ] && . /etc/os-release && [ "${ID:-}" = fedora ]; }
secure_boot() { command -v mokutil > /dev/null && mokutil --sb-state 2> /dev/null | grep -q 'SecureBoot enabled'; }
nvidia_gpu() { lspci 2> /dev/null | grep -Eiq '(vga|3d).*nvidia'; }
nvidia_built() { modinfo -F version nvidia > /dev/null 2>&1; }
nvidia_loaded() { [ -e /proc/driver/nvidia/version ]; }
key_enrolled() {
    [ -r /etc/pki/akmods/certs/public_key.der ] && mokutil --test-key /etc/pki/akmods/certs/public_key.der 2> /dev/null \
        | grep -q 'already enrolled'
}
key_waiting() { mokutil --list-new 2> /dev/null | grep -qi 'certificate'; }

# The libraries are named by what pkg-config and CMake look for, which dnf
# resolves to Fedora's packages, so a renamed package does not break this.
PACKAGES=(
    gcc-c++ cmake ninja-build git python3 gh weston mokutil
    dbus-daemon python3-dbus python3-gobject
    'pkgconfig(gl)' 'pkgconfig(egl)' 'pkgconfig(glesv2)' 'pkgconfig(gbm)' 'pkgconfig(libdrm)' 'pkgconfig(vulkan)'
    'pkgconfig(xkbcommon)' 'pkgconfig(xkbcommon-x11)' 'pkgconfig(xcb-cursor)'
    'pkgconfig(fontconfig)' 'pkgconfig(freetype2)' 'pkgconfig(dbus-1)'
    'pkgconfig(wayland-client)' 'pkgconfig(wayland-server)' 'pkgconfig(wayland-protocols)'
    'pkgconfig(libinput)' 'pkgconfig(libpulse)' 'pkgconfig(openssl)'
)

step_check() {
    say "What is in place"
    if fedora; then (. /etc/os-release && echo "System: $PRETTY_NAME"); else echo "System: not Fedora"; fi
    echo "Secure Boot: $(secure_boot && echo on || echo 'off, or mokutil missing')"
    if nvidia_gpu; then
        echo "NVIDIA GPU: yes; driver built: $(nvidia_built && modinfo -F version nvidia || echo no);" \
            "loaded: $(nvidia_loaded && echo yes || echo no)"
        secure_boot && echo "Signing key enrolled: $(key_enrolled && echo yes || { key_waiting && echo 'waiting for the next boot' || echo no; })"
    else
        echo "NVIDIA GPU: none seen"
    fi
    echo "Rust: $(command -v rustc > /dev/null && rustc --version || echo none)"
    echo "Qt 6.12: $([ -x "$QT/bin/qmake" ] && echo "$QT" || echo "not in $QT")"
    echo "Prism: $([ -x "$BUILD/prism/compositor/prism" ] && echo built || echo 'not built')"
    local p
    for p in arqade/arqade media/qor-media qq/player/qq-player qq/studio/qq-studio; do
        echo "${p##*/}: $([ -x "$PRODUCTS/$p" ] && echo built || echo 'not built')"
    done
}

step_packages() {
    fedora || { echo "This script is for Fedora; docs/BUILDING.md lists the packages elsewhere." >&2; exit 1; }
    say "Build tools and libraries"
    sudo dnf install -y "${PACKAGES[@]}"
}

# RPM Fusion's driver, as its NVIDIA and Secure Boot guides say: akmods
# builds the kernel module for each kernel and signs it with a key of this
# machine's, which the firmware must trust before it loads the module. A
# driver installed before that key is enrolled stays unloaded, and can leave
# the screen blank.
step_nvidia() {
    fedora || { echo "This script is for Fedora." >&2; exit 1; }
    if ! nvidia_gpu; then
        say "No NVIDIA GPU: nothing to do"
        return 0
    fi
    say "RPM Fusion"
    if ! rpm -q rpmfusion-free-release rpmfusion-nonfree-release > /dev/null 2>&1; then
        sudo dnf install -y \
            "https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm" \
            "https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm"
    fi
    if secure_boot && ! key_enrolled; then
        say "Secure Boot: a key to sign the driver with"
        sudo dnf install -y kmodtool akmods mokutil openssl
        [ -r /etc/pki/akmods/certs/public_key.der ] || sudo kmodgenca -a
        if ! key_waiting; then
            echo "mokutil now asks for a one-time password of your choosing. Remember it for the next boot."
            sudo mokutil --import /etc/pki/akmods/certs/public_key.der
        fi
        cat << 'EOF'

Reboot now (systemctl reboot). Before Fedora starts, a blue screen, MOK
management, appears: choose Enroll MOK, Continue, Yes, type the password you
just chose (the keyboard there is US English), then Reboot. Then run this
script again to install the driver.
EOF
        exit 3
    fi
    if ! nvidia_built; then
        say "The NVIDIA driver"
        sudo dnf upgrade --refresh -y
        if [ "$(rpm -q --last kernel-core | head -n1 | awk '{print $1}' | sed 's/^kernel-core-//')" != "$(uname -r)" ]; then
            echo "A newer kernel was installed. Reboot (systemctl reboot) and run this script again."
            exit 3
        fi
        sudo dnf install -y akmod-nvidia xorg-x11-drv-nvidia-cuda
        echo "Waiting while akmods builds the kernel module (up to five minutes)..."
        local _
        for _ in $(seq 60); do
            nvidia_built && break
            sleep 5
        done
        nvidia_built || { echo "The module was not built; see journalctl -u akmods." >&2; exit 1; }
    fi
    if ! nvidia_loaded; then
        echo "NVIDIA $(modinfo -F version nvidia) is built. Reboot (systemctl reboot) to load it."
        exit 3
    fi
    say "NVIDIA $(cat /proc/driver/nvidia/version | head -n1 | grep -o '[0-9]\+\.[0-9.]\+' | head -n1) is loaded"
}

step_rust() {
    say "Rust $RUST"
    if ! command -v rustup > /dev/null && [ ! -x "$HOME/.cargo/bin/rustup" ]; then
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs \
            | sh -s -- -y --profile minimal --default-toolchain "$RUST"
    fi
    "$HOME/.cargo/bin/rustup" toolchain install "$RUST" --profile minimal
    "$HOME/.cargo/bin/rustup" default "$RUST"
}

# tools/ci/install_qt.py signs in as the Qt installer does: with the Qt
# Account this machine signed in to (qtaccount.ini), or the variables CI
# uses. It prints no credential.
step_qt() {
    say "Qt 6.12, the commercial kit, in $QT_ROOT"
    if [ ! -f "$HOME/.local/share/Qt/qtaccount.ini" ] && [ -z "${QT_INSTALLER_JWT_TOKEN:-}" ] \
        && [ -z "${QT_INSTALLER_EMAIL:-}" ]; then
        cat << 'EOF' >&2
No Qt Account sign-in here. Sign in once: download the Qt Online Installer
for Linux from your Qt Account, run it, sign in, and close it at the
component list. Then run this step again.
EOF
        exit 1
    fi
    python3 "$REPO/tools/ci/install_qt.py" --platform linux --root "$QT_ROOT"
}

step_build() {
    say "QOR OS (linux-release)"
    cd "$REPO"
    export PATH=$HOME/.cargo/bin:$PATH QOR_QT_DIR=$QT
    cmake --preset linux-release
    cmake --build --preset linux-release
    say "The products, without their suites"
    "$REPO/tools/dev/products.sh" --qt "$QT" --build-only
}

# Prism in a window of this session (GNOME's Wayland, or X11), with the
# products where Prism finds them and the QQ samples in Home and the Library.
# On a hybrid laptop GNOME runs it on the Intel GPU; QOR_PLAY_GPU=discrete
# runs it on the other, through switcheroo-control.
step_play() {
    local prism=$BUILD/prism/compositor/prism
    [ -x "$prism" ] || { echo "Prism is not built: run the build step." >&2; exit 1; }
    export QOR_ARQADE=$PRODUCTS/arqade/arqade
    export QOR_MEDIA=$PRODUCTS/media/qor-media
    export QOR_QQ_STUDIO=$PRODUCTS/qq/studio/qq-studio
    export QOR_QQ_PLAYER=$PRODUCTS/qq/player/qq-player
    export QOR_GAMES=$REPO/products/qq/samples
    say "Prism: Guide (the PS button) opens Home; Orb Run is among the games"
    if [ "${QOR_PLAY_GPU:-}" = discrete ] && command -v switcherooctl > /dev/null; then
        exec switcherooctl launch "$prism" --unlocked "$@"
    fi
    exec "$prism" --unlocked "$@"
}

STEP=${1:-all}
[ $# -eq 0 ] || shift
case $STEP in
    check | packages | nvidia | rust | qt | build) "step_$STEP" ;;
    play) step_play "$@" ;;
    all)
        step_packages
        step_nvidia
        step_rust
        step_qt
        step_build
        step_check
        say "Done. Start Prism with: tools/dev/setup-fedora.sh play"
        ;;
    -h | --help) usage ;;
    *) echo "Unknown step: $STEP" >&2; usage >&2; exit 2 ;;
esac
