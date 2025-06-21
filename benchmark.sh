#!/usr/bin/env bash

URL=${1:-http://127.0.0.1:8000/}
TOTAL=${2:-100000}
CONC=${3:-2000}

if ! command -v k6 &>/dev/null; then
    echo "Install k6 first."
    exit 1
fi

SCRIPT="./k6-bench.js"
cat <<'EOF' > "$SCRIPT"
import http from 'k6/http';

export let options = {
    vus: __CONC__,
    iterations: __TOTAL__,
};

export default function () {
    http.get('__URL__');
}
EOF

sed -i "s|__URL__|${URL}|g; s|__TOTAL__|${TOTAL}|g; s|__CONC__|${CONC}|g" "$SCRIPT"

echo "=> Benchmarking target: $URL"
echo "=> Total requests: $TOTAL, Concurrent VUs: $CONC"
echo

k6 run "$SCRIPT"

rm "$SCRIPT"
