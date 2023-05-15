#/bin/bash

echo "Killing any previously spawned processes..."

__dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source ${__dir}/killNodes.sh

CLEAN=false
BUILD=true
NODES=""
REP=""
HELP_MSG="Usage: ./run.sh [--no-build] [--clean] [--help] --nodes n --rep k"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean)
            CLEAN=true
            ;;
        --no-build)
            BUILD=false
            ;;
        --nodes)
            if [[ $2 =~ ^[0-9]+$ ]]; then
                NODES=$((10#$2))
            else
                echo "Error: --nodes argument must be an integer"
                exit 1
            fi
            shift
            ;;
        --rep)
            if [[ $2 =~ ^[0-9]+$ ]]; then
                REP=$((10#$2))
            else
                echo "Error: --rep argument must be an integer"
                exit 1
            fi
            shift
            ;;
        --help)
            echo "$HELP_MSG"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "$HELP_MSG"
            exit 1
            ;;
    esac
    shift
done

if [ -z "$NODES" ]; then
    echo "Error: --nodes argument is required"
    exit 1
fi

if [ -z "$REP" ]; then
    echo "Error: --rep argument is required"
    exit 1
fi

if [ "$BUILD" = true ]; then
__dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if $CLEAN; then
    source ${__dir}/build.sh --clean
else
    source ${__dir}/build.sh
fi

if [ $? -eq 0 ]; then
    echo "Build successful, starting manager and storage nodes..."
else
    exit 1
fi
fi

mkdir -p output

# Launch the GTStore Manager
./bin/manager --nodes "$NODES" --rep "$REP" > output/manager.log 2>&1 &
PID=$!
echo "$PID" >> output/pids_spawned
echo "Manager started with PID $PID, logs printed to output/manager.log"
sleep 1

PORT=50052
for i in $(seq 1 $NODES); do
    echo "Spawning storage node $i..."
    URL="0.0.0.0:$PORT"
    LOGFILE="output/storage_$i.log"
    ./bin/storage --id $i --url $URL > $LOGFILE 2>&1 &
    PID=$!
    echo "$PID" >> output/pids_spawned
    echo "Storage node $i started with PID $PID, listening on $URL and logs printed to $LOGFILE"
    ((PORT++))
done
sleep 1

