#!/usr/bin/env bash
# The compositor decision gate (ADR-0003, issue #9) on real hardware.
#
# Runs Prism on Qt's eglfs platform with its KMS integration, one output per
# connected display, with an animating Wayland client on each. It records
# `prism --benchmark` and judges it against the criteria in
# docs/PERFORMANCE.md: under 1% missed frames on every output at its own
# refresh rate, and input to swap within two refresh periods at p95.
#
# Run it from a text console (Ctrl+Alt+F3), not inside a desktop session:
# Prism must be the DRM master. Your user needs the video, render and input
# groups. See docs/PERFORMANCE.md, "Running the decision gate".
#
#   tools/dev/prism-gate.sh --build-dir ~/qtos-build/linux-release [--seconds 20] [--out DIR]
#
# Writes report.json, summary.md (the data for ADR-0011) and the logs to the
# output folder. Exits 0 when every criterion passes.

set -euo pipefail

build=""
seconds=20
out="gate-$(date +%Y%m%d-%H%M%S)"
while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir) build="$2"; shift 2 ;;
        --seconds) seconds="$2"; shift 2 ;;
        --out) out="$2"; shift 2 ;;
        -h | --help) sed -n '2,17p' "$0"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done
[ -n "$build" ] || { echo "--build-dir is required" >&2; exit 2; }
prism="$build/prism/compositor/prism"
client="$build/tests/prism-test-client"
for binary in "$prism" "$client"; do
    [ -x "$binary" ] || { echo "missing: $binary (build the linux-release preset first)" >&2; exit 2; }
done
if [ -n "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ]; then
    echo "A desktop session is running here. Switch to a text console (Ctrl+Alt+F3) and run this again." >&2
    exit 3
fi
compgen -G "/dev/dri/card*" > /dev/null || { echo "No DRM device (/dev/dri/card*)." >&2; exit 3; }

mkdir -p "$out"
out=$(cd "$out" && pwd)
# Every connected display is a screen, side by side, with the hardware cursor.
printf '{ "hwcursor": true, "virtualDesktopLayout": "horizontal" }\n' > "$out/kms.json"

export QT_QPA_PLATFORM=eglfs
export QT_QPA_EGLFS_INTEGRATION=eglfs_kms
export QT_QPA_EGLFS_KMS_CONFIG="$out/kms.json"
export QT_QPA_EGLFS_KMS_ATOMIC=1
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"

socket="wayland-gate-$$"
"$prism" --socket "$socket" --spread-clients --benchmark "$seconds" --benchmark-out "$out/report.json" \
    > "$out/prism.log" 2>&1 &
prism_pid=$!
for _ in $(seq 1 150); do
    grep -q "Prism is running" "$out/prism.log" 2> /dev/null && break
    kill -0 "$prism_pid" 2> /dev/null || break
    sleep 0.1
done
outputs=$(grep -c '^Output ' "$out/prism.log" || true)
if ! grep -q "Prism is running" "$out/prism.log" || [ "$outputs" -lt 1 ]; then
    echo "Prism did not start:" >&2
    cat "$out/prism.log" >&2
    exit 4
fi
grep '^Output ' "$out/prism.log"

# One animating client per output (--spread-clients places them).
clients=()
for i in $(seq 1 "$outputs"); do
    WAYLAND_DISPLAY="$socket" QT_QPA_PLATFORM=wayland "$client" --title "Gate client $i" \
        --app-id org.qtos.GateClient --size 900x560 --animate > "$out/client-$i.log" 2>&1 &
    clients+=($!)
    sleep 0.5
done
echo "Measuring for ${seconds} s…"
wait "$prism_pid" || true
kill "${clients[@]}" 2> /dev/null || true
[ -f "$out/report.json" ] || { echo "No report; see $out/prism.log" >&2; exit 4; }

python3 - "$out" << 'PY'
import json
import os
import platform
import subprocess
import sys
from datetime import date

out = sys.argv[1]
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

summary = f"""# Compositor decision gate: measurement ({date.today().isoformat()})

- Machine: {platform.node()}, kernel {platform.release()}
- GPU: {gpu or "unknown"}
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
print(f"Saved {out}")
sys.exit(0 if passed and enough else 1)
PY
