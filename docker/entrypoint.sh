#!/usr/bin/env bash
set -euo pipefail

export DISPLAY="${DISPLAY:-:99}"
export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"

cleanup() {
    jobs -p | xargs -r kill
}

trap cleanup EXIT

Xvfb "$DISPLAY" -screen 0 1600x900x24 -ac +extension GLX +render -noreset &

for _ in $(seq 1 20); do
    if xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
        break
    fi
    sleep 0.5
done

if ! xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
    echo "Xvfb did not become ready on $DISPLAY" >&2
    exit 1
fi

fluxbox >/tmp/fluxbox.log 2>&1 &
x11vnc -display "$DISPLAY" -forever -nopw -shared -rfbport 5900 >/tmp/x11vnc.log 2>&1 &
websockify --web=/usr/share/novnc 6080 localhost:5900 >/tmp/websockify.log 2>&1 &

if [ "$#" -eq 0 ]; then
    set -- ./splinter3D
fi

exec "$@"
