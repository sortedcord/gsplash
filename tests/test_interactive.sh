#!/bin/bash

GSPLASH="./build/gsplash"
DUMMY="./build/dummy_game"

if [ ! -f "$GSPLASH" ] || [ ! -f "$DUMMY" ]; then
    echo "Please run 'make' first to build gsplash and dummy_game."
    exit 1
fi

if [ -z "$1" ]; then
    echo "Usage: $0 <path_to_test_image_or_video>"
    exit 1
fi

IMAGE="$1"
if [ ! -f "$IMAGE" ]; then
    echo "Error: File '$IMAGE' not found."
    exit 1
fi

echo "=== Interactive gsplash Visual Test ==="
echo "This will physically display the splash screen using different modes."
echo "You will be asked to confirm if it looked correct after each test."
echo ""

pass=0
fail=0

run_interactive_test() {
    local mode="$1"
    
    echo "------------------------------------------------"
    echo "Testing mode: $mode"
    echo "Launching in 2 seconds (it will stay open for 3 seconds)..."
    sleep 2
    
    # Run gsplash with a 3-second dummy game
    $GSPLASH --mode="$mode" "$IMAGE" "$DUMMY" 3
    
    # Prompt the user interactively
    while true; do
        read -p "Did it display correctly for mode '$mode'? [y/N] " response
        case "$response" in
            [yY][eE][sS]|[yY])
                echo "=> PASS recorded."
                pass=$((pass+1))
                break
                ;;
            [nN][oO]|[nN]|"")
                echo "=> FAIL recorded."
                fail=$((fail+1))
                break
                ;;
            *)
                echo "Please answer y or n."
                ;;
        esac
    done
}

run_interactive_test "stretch"
run_interactive_test "center"
run_interactive_test "crop"

echo "================================================"
echo "Interactive Testing Complete!"
echo "Passed: $pass"
echo "Failed: $fail"
