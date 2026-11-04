#!/bin/sh
# Fails if a source file includes a header from a higher layer.
# Layers, lowest first (a layer may include its own and anything lower):
#   util, platform, model, scan, db, core, ui, app, main.c
# version.h may be included from anywhere.
cd "$(dirname "$0")/../src" || exit 1

rank() {
    case "$1" in
        util) echo 0 ;; platform) echo 1 ;; model) echo 2 ;; scan) echo 3 ;;
        db) echo 4 ;; core) echo 5 ;; ui) echo 6 ;; app) echo 7 ;; main) echo 8 ;;
        *) echo "" ;;
    esac
}

status=0
for f in $(find . -name '*.[ch]' | sed 's|^\./||'); do
    case "$f" in
        main.c) layer=main ;;
        version.h) continue ;;
        */*) layer=${f%%/*} ;;
        *) echo "unknown layer: $f"; status=1; continue ;;
    esac
    mine=$(rank "$layer")
    [ -z "$mine" ] && { echo "unknown layer: $f"; status=1; continue; }

    for inc in $(sed -n 's/^#include "\([a-z_]*\)\/.*"/\1/p' "$f"); do
        theirs=$(rank "$inc")
        [ -z "$theirs" ] && continue # third-party or same-dir header
        if [ "$theirs" -gt "$mine" ]; then
            echo "layer violation: $f ($layer) includes $inc/"
            status=1
        fi
    done
done

[ $status -eq 0 ] && echo "layers ok"
exit $status
