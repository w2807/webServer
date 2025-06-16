#!/usr/bin/env bash

URL=${1:-http://127.0.0.1:8000/}
TOTAL=${2:-10000}
CONC=${3:-100}

if ! command -v ab &>/dev/null; then
    echo "Install Apache Benchmark (ab) first."
    exit 1
fi

echo "=> Benchmarking target: $URL"
echo "=> Total requests: $TOTAL, Concurrent requests: $CONC"
echo

ab -n $TOTAL -c $CONC $URL
