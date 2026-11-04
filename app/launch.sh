#!/bin/sh
# RHSM launcher for OnionOS (/mnt/SDCARD/App/RHSM/launch.sh)
cd "$(dirname "$0")" || exit 1

LOG=log.txt
[ -f "$LOG" ] && mv -f "$LOG" log.prev.txt

log() { echo "[launch] $*" >> "$LOG"; }

# Debug mode: `make debug-on` creates the file `debug` and copies gdbserver to
# bin/. The app then waits for a debugger on port 2345 before starting.
if [ -f debug ]; then
    if [ ! -f bin/gdbserver ]; then
        log "debug flag set but bin/gdbserver missing; running normally"
    else
        if ! ./bin/gdbserver --version >> "$LOG" 2>&1; then
            log "gdbserver failed to start (see above); running normally"
        else
            log "debug mode: waiting for gdb on :2345"
            exec ./bin/gdbserver :2345 ./bin/rhsm >> "$LOG" 2>&1
        fi
    fi
else
    log "normal mode (no debug flag)"
fi

exec ./bin/rhsm >> "$LOG" 2>&1
