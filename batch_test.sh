#!/bin/bash
cd "$(dirname "$0")"

if [[ $# -eq 4 ]]; then
    echo "Running with custom clients: $@"
    ALGOS="$@"
else
    echo "Invalid number of arguments. Must specify 4 client names (with .py extension)."
    exit 1
fi

# Clean ALL debug logs (including renamed _run* ones)
rm -f logs/*_debug_*.log
RESULTS=""
for i in 1 2 3; do
    echo "=== RUN $i ==="
    ./game.sh -n "Run $i" $ALGOS
    sleep 3
    # Find only non-renamed logs (no _run in name) to avoid picking up old renamed files
    LOGS=$(ls -t logs/*_debug_*.log 2>/dev/null | grep -v '_run' || true)
    LOG=$(echo "$LOGS" | head -1)
    if [ -n "$LOG" ]; then
        echo "Log: $LOG"
        LAST=$(grep 'STATUS' "$LOG" | tail -1)
        PEAK=$(grep 'STATUS' "$LOG" | sort -t'=' -k3 -rn | head -1)
        echo "Peak: $PEAK"
        echo "Final: $LAST"
        RESULTS="$RESULTS\nRun $i Peak: $PEAK\nRun $i Final: $LAST\n"
        # Rename ALL logs from this run to prevent re-picking
        for L in $LOGS; do
            mv "$L" "${L%.log}_run${i}.log"
        done
    fi
    echo ""
done
echo ""
echo "=== SUMMARY ==="
echo -e "$RESULTS"
