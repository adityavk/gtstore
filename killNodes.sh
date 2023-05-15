#!/bin/bash

while read -r pid; do
    echo "Killing process $pid..."
    kill "$pid"
done < output/pids_spawned

echo "All processes killed."
rm output/pids_spawned
