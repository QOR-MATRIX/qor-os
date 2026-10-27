#!/usr/bin/env bash
# The compositor decision gate (ADR-0003, issue #9) on real hardware.
#
# Runs Prism on Qt's eglfs platform with its KMS integration, one output per
# display on the GPU, with an animating Wayland client on each. It records
# `prism --benchmark` and judges it against the criteria in
# docs/PERFORMANCE.md: under 1% missed frames on every output at its own
# refresh rate, and input to swap within two refresh periods at p95.
#
# Run it from a text console (Ctrl+Alt+F3), not inside a desktop session:
# Prism drives the displays itself. Ctrl+C stops a run.
#
#   prism-gate.sh [--build-dir DIR] [--seconds 20] [--out DIR] [--device /dev/dri/cardN]
#                 [--mode CONNECTOR=WxH@HZ]... [--no-atomic]
#   prism-gate.sh --probe    list the GPUs and the displays on each, then exit
#   prism-gate.sh --try      a trial run inside a desktop session, as two windows
#
# --build-dir defaults to this script's folder in a gate bundle
# (prism-gate-bundle.sh). --mode picks a display's mode, e.g. eDP-1=1920x1080@60
# (connector names as --probe prints them). Writes report.json, summary.md (the
# data for ADR-0011), the logs and a .zip of them all. Exits 0 when every
# criterion passes. See docs/PERFORMANCE.md, "Running the decision gate".

set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
build=""
seconds=20
out="gate-$(date +%Y%m%d-%H%M%S)"
device=""
modes=()
atomic=1
action=gate
while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir) build="$2"; shift 2 ;;
        --seconds) seconds="$2"; shift 2 ;;
        --out) out="$2"; shift 2 ;;
        --device) device="$2"; shift 2 ;;
        --mode) modes+=("$2"); shift 2 ;;
        --no-atomic) atomic=0; shift ;;
        --probe) action=probe; shift ;;
        --try) action=try; out="${out/gate-/try-}"; shift ;;
        -h | --help) sed -n '2,23p' "$0"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

# The GPUs and their connectors. Sets best (the card with the most displays),
# best_count, total and panel (the built-in display's connector on best).
probe() {
    best=""
    best_count=0
    total=0
    panel=""
    local card name driver gpu count connector label status first_panel
    for card in /sys/class/drm/card[0-9]*; do
        [ -e "$card/device" ] || continue
        name=${card##*/}
        case "$name" in *-*) continue ;; esac
        driver=$(basename "$(readlink -f "$card/device/driver")")
        gpu=$(lspci -s "$(basename "$(readlink -f "$card/device")")" 2> /dev/null | cut -d' ' -f2- || true)
        echo "GPU $name ($driver): ${gpu:-unknown}"
        count=0
        first_panel=""
        for connector in "$card"-*; do
            [ -e "$connector/status" ] || continue
            label=${connector##*/"$name"-}
            status=$(cat "$connector/status")
            if [ "$status" = connected ]; then
                count=$((count + 1))
                printf '    %-12s connected     %s\n' "$label" "$(head -n 1 "$connector/modes" 2> /dev/null || true)"
                case "$label" in eDP-* | LVDS-* | DSI-*) first_panel=$label ;; esac
            else
                printf '    %-12s %s\n' "$label" "$status"
            fi
        done
        total=$((total + count))
        if [ "$count" -gt "$best_count" ] || { [ "$count" -eq "$best_count" ] && [ -n "$first_panel" ]; }; then
            best=$name
            best_count=$count
            panel=$first_panel
        fi
    done
}

if [ "$action" = probe ]; then
    probe
    echo
    if [ "$best_count" -ge 2 ]; then
        echo "Ready: $best drives $best_count displays."
    elif [ "$total" -ge 2 ]; then
        echo "The displays are on different GPUs; the gate needs two on one GPU."
    else
        echo "$total display(s) connected; the gate needs two on one GPU."
    fi
    exit 0
fi

if [ -z "$build" ] && [ -x "$here/prism/compositor/prism" ]; then
    build=$here
fi
[ -n "$build" ] || { echo "--build-dir is required" >&2; exit 2; }
prism="$build/prism/compositor/prism"
client="$build/tests/prism-test-client"
for binary in "$prism" "$client"; do
    [ -x "$binary" ] || { echo "missing: $binary (build the linux-release preset first)" >&2; exit 2; }
done
# A gate bundle carries its own Qt.
if [ -d "$build/qt/lib" ]; then
    export LD_LIBRARY_PATH="$build/qt/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export QT_PLUGIN_PATH="$build/qt/plugins"
    export QML_IMPORT_PATH="$build/qt/qml"
fi
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"

prism_args=(--spread-clients --benchmark "$seconds")
if [ "$action" = try ]; then
    if [ -z "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ]; then
        echo "--try runs inside a desktop session; without one, run the gate itself." >&2
        exit 3
    fi
    prism_args+=(--outputs 2 --size 1280x720)
else
    if [ -n "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ]; then
        echo "A desktop session is running here. Switch to a text console (Ctrl+Alt+F3), log in and run this again." >&2
        echo "(prism-gate.sh --try runs a trial in this session.)" >&2
        exit 3
    fi
    probe
    if [ -z "$device" ]; then
        if [ "$best_count" -lt 2 ]; then
            echo >&2
            if [ "$total" -ge 2 ]; then
                echo "The displays are on different GPUs, and Prism drives one GPU. Connect the external display to a" >&2
                echo "port on the same GPU as the other: try the laptop's other video port (USB-C instead of HDMI, or" >&2
                echo "the reverse), or two external displays on one GPU. The list above shows which GPU has which." >&2
            else
                echo "Only $total display is connected; the gate needs two on one GPU, at different refresh rates." >&2
            fi
            echo "(--device /dev/dri/CARD runs anyway, on that GPU's displays.)" >&2
            exit 5
        fi
        device=/dev/dri/$best
    elif [ "$device" != "/dev/dri/$best" ]; then
        panel=""
    fi
    if [ ! -r "$device" ] || [ ! -w "$device" ]; then
        echo "No access to $device. Log in on this console itself, or join the video and render groups:" >&2
        echo "  sudo usermod -aG video,render,input $USER    (then log out and back in)" >&2
        exit 3
    fi
    if [[ " $(id -nG) " != *" input "* ]]; then
        echo "Note: without the input group Prism gets no keyboard or mouse. The measurement does not need them."
    fi
fi

mkdir -p "$out"
out=$(cd "$out" && pwd)
if [ "$action" = gate ]; then
    # Every display on the GPU is a screen, side by side, the built-in one first.
    python3 - "$out/kms.json" "$device" "$panel" ${modes[@]+"${modes[@]}"} << 'PY'
import json
import re
import sys

path, device, panel, *modes = sys.argv[1:]


def qt_name(connector):  # The kernel's "HDMI-A-1" is Qt's "HDMI1".
    return re.sub(r"^HDMI-[AB]-", "HDMI-", connector).replace("-", "")


outputs = {}
if panel:
    outputs[panel] = {"name": qt_name(panel), "primary": True}
for mode in modes:
    connector, _, value = mode.partition("=")
    outputs.setdefault(connector, {"name": qt_name(connector)})["mode"] = value
config = {"device": device, "hwcursor": True, "virtualDesktopLayout": "horizontal"}
if outputs:
    config["outputs"] = list(outputs.values())
json.dump(config, open(path, "w"), indent=1)
PY
    export QT_QPA_PLATFORM=eglfs
    export QT_QPA_EGLFS_INTEGRATION=eglfs_kms
    export QT_QPA_EGLFS_KMS_CONFIG="$out/kms.json"
    export QT_QPA_EGLFS_KMS_ATOMIC=$atomic
    # The modes each display offers, in prism.log.
    export QT_LOGGING_RULES="qt.qpa.eglfs.kms=true"
fi

pids=()
cleanup() {
    if [ ${#pids[@]} -gt 0 ]; then
        kill "${pids[@]}" 2> /dev/null || true
    fi
}
# The results, or the logs of a run that failed, packed for upload. A .zip:
# GitHub issues refuse .tgz attachments.
# shellcheck disable=SC2329 # called from the EXIT trap
pack() {
    python3 - "$out" << 'PY' && echo "Saved $out and $out.zip"
import os
import sys
import zipfile

folder = sys.argv[1]
with zipfile.ZipFile(folder + ".zip", "w", zipfile.ZIP_DEFLATED) as archive:
    for root, _, files in os.walk(folder):
        for name in files:
            path = os.path.join(root, name)
            archive.write(path, os.path.relpath(path, os.path.dirname(folder)))
PY
}
trap 'cleanup; pack' EXIT
trap 'echo "Stopped." >&2; exit 130' INT TERM

socket="wayland-gate-$$"
# The timeout is a watchdog, in case Prism hangs holding the displays.
timeout --kill-after=10 $((seconds + 120)) "$prism" --socket "$socket" "${prism_args[@]}" \
    --benchmark-out "$out/report.json" > "$out/prism.log" 2>&1 < /dev/null &
prism_pid=$!
pids+=("$prism_pid")
for _ in $(seq 1 300); do
    grep -q "Prism is running" "$out/prism.log" 2> /dev/null && break
    kill -0 "$prism_pid" 2> /dev/null || break
    sleep 0.1
done
if [ "$action" = try ]; then
    outputs=2
else
    outputs=$(grep -c '^Output ' "$out/prism.log" || true)
fi
if ! grep -q "Prism is running" "$out/prism.log" || [ "$outputs" -lt 1 ]; then
    echo "Prism did not start; the end of $out/prism.log:" >&2
    tail -n 25 "$out/prism.log" >&2
    exit 4
fi
grep '^Output ' "$out/prism.log" || true

# One animating client per output (--spread-clients places them).
for i in $(seq 1 "$outputs"); do
    WAYLAND_DISPLAY="$socket" QT_QPA_PLATFORM=wayland "$client" --title "Gate client $i" \
        --app-id org.qtos.GateClient --size 900x560 --animate > "$out/client-$i.log" 2>&1 < /dev/null &
    pids+=($!)
    sleep 0.5
done
echo "Measuring for about $((seconds + 15)) s. The screens return when it is done."
wait "$prism_pid" || true
cleanup
pids=()
[ -f "$out/report.json" ] || { echo "No report; the end of $out/prism.log:" >&2; tail -n 25 "$out/prism.log" >&2; exit 4; }

status=0
GATE_TRIAL=$([ "$action" = try ] && echo 1 || echo 0) GATE_DEVICE="$device" python3 - "$out" << 'PY' || status=$?
import json
import os
import platform
import subprocess
import sys
from datetime import date

out = sys.argv[1]
trial = os.environ.get("GATE_TRIAL") == "1"
report = json.load(open(os.path.join(out, "report.json")))
outputs = report.get("outputs", [])
latency = report.get("inputToSwap", {})


def run(command):
    try:
        return subprocess.run(command, capture_output=True, text=True, check=False).stdout.strip()
    except OSError:
        return ""


gpu = "; ".join(line for line in run(["lspci"]).splitlines() if "VGA" in line or "3D controller" in line)
rows = []
passed = True
for index, o in enumerate(outputs):
    ok = o.get("frames", 0) > 0 and o.get("missedPercent", 100) < 1.0
    passed &= ok
    rows.append(f"| {index + 1} | {o.get('screen', '?')} | {o.get('size', '?')} | {o.get('refreshHz', 0):.1f} Hz "
                f"| {o.get('frames', 0)} | {o.get('p95Ms', 0):.2f} ms | {o.get('missedPercent', 0):.2f}% "
                f"| {'pass' if ok else 'FAIL'} |")
refresh = outputs[0].get("refreshHz", 60) if outputs else 60
budget = 2 * 1000.0 / refresh
latency_ok = latency.get("samples", 0) > 0 and latency.get("p95Ms", 1e9) <= budget
passed &= latency_ok
distinct = len({round(o.get("refreshHz", 0)) for o in outputs}) > 1
enough = len(outputs) >= 2

title = "Trial inside a desktop session (not gate data)" if trial else "Compositor decision gate: measurement"
summary = f"""# {title} ({date.today().isoformat()})

- Machine: {platform.node()}, kernel {platform.release()}
- GPU: {gpu or "unknown"}
- DRM device: {os.environ.get("GATE_DEVICE") or "none (nested)"}
- Graphics API: {report.get("graphicsApi", "?")}
- Outputs: {len(outputs)}{"" if enough else " (the gate asks for two)"}, refresh rates \
{"differ" if distinct else "are the same (the gate asks for different ones)"}

| Output | Screen | Size | Refresh | Frames | p95 interval | Missed | Under 1% missed |
|--------|--------|------|---------|--------|--------------|--------|-----------------|
""" + "\n".join(rows) + f"""

Input to swap: p50 {latency.get("p50Ms", 0):.1f} ms, p95 {latency.get("p95Ms", 0):.1f} ms \
({latency.get("samples", 0)} samples); two refresh periods of output 1 are {budget:.1f} ms: \
{"pass" if latency_ok else "FAIL"}.

**Result: {"every criterion passes" if passed else "a criterion fails"}.** \
{"" if enough and distinct else "The setup does not yet match the gate (two outputs at different refresh rates). "}\
Record the decision in ADR-0011 (docs/adr/).
"""
open(os.path.join(out, "summary.md"), "w").write(summary)
print(summary)
sys.exit(0 if trial or (passed and enough) else 1)
PY
exit $status
