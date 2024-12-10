#!/bin/bash

# Find all QEMU processes
qemu_pids=$(pgrep qemu)

if [ -z "$qemu_pids" ]; then
    echo "No QEMU processes found."
else
    echo "Found QEMU processes: $qemu_pids"
    echo "Killing them now..."
    # Kill all QEMU processes gracefully
    kill $qemu_pids
    # sleep 2

    # # Check if any processes remain and force kill if necessary
    # remaining_pids=$(pgrep qemu)
    # if [ -n "$remaining_pids" ]; then
    #     echo "Forcing kill for remaining processes: $remaining_pids"
    #     kill -9 $remaining_pids
    # else
    #     echo "All QEMU processes terminated."
    # fi
fi
