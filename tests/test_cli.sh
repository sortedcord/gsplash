#!/bin/bash

# A test suite for testing CLI argument permutations of gsplash

GSPLASH="./build/gsplash"
GAME="/bin/true"
IMAGE="nonexistent.png" 

if [ ! -f "$GSPLASH" ]; then
    echo "Please run 'make' first to build gsplash."
    exit 1
fi

export SDL_VIDEODRIVER=dummy

pass=0
fail=0

run_test() {
    local expected_status=$1
    shift
    local expected_log="$1"
    shift
    local desc="$1"
    shift
    
    printf "Test: %-45s " "$desc"
    
    # Run the command and capture output
    local output
    output=$($GSPLASH "$@" 2>&1)
    local status=$?
    
    local passed=true
    local error_msg=""

    if [ $status -ne $expected_status ]; then
        passed=false
        error_msg="Expected status $expected_status, got $status"
    elif [ -n "$expected_log" ]; then
        if ! echo "$output" | grep -q "$expected_log"; then
            passed=false
            error_msg="Expected log missing: $expected_log"
        fi
    fi

    if $passed; then
        echo "PASS"
        pass=$((pass+1))
    else
        echo "FAIL ($error_msg)"
        fail=$((fail+1))
    fi
}

echo "=== Running CLI Argument Tests ==="

# Valid combinations (0=STRETCH, 1=CENTER, 2=CROP)
run_test 0 "mode=1" "Basic invocation" $IMAGE $GAME
run_test 0 "mode=0" "Long mode: stretch" --mode=stretch $IMAGE $GAME
run_test 0 "mode=1" "Long mode: center" --mode=center $IMAGE $GAME
run_test 0 "mode=2" "Long mode: crop" --mode=crop $IMAGE $GAME
run_test 0 "mode=0" "Long mode: fallback to stretch" --mode=invalid $IMAGE $GAME
run_test 0 "mode=0" "Short mode: stretch" -m stretch $IMAGE $GAME
run_test 0 "mode=1" "Short mode: center" -m center $IMAGE $GAME
run_test 0 "mode=2" "Short mode: crop" -m crop $IMAGE $GAME
run_test 0 "mode=0" "Short mode: fallback to stretch" -m whatever $IMAGE $GAME
run_test 0 "mode=1" "With game arguments" $IMAGE $GAME arg1 arg2
run_test 0 "mode=2" "Mode and game arguments" -m crop $IMAGE $GAME arg1 arg2

# Invalid combinations (Usage errors, so we just check for status 1, no specific log needed)
run_test 1 "" "No arguments"
run_test 1 "" "Only image" $IMAGE
run_test 1 "" "Missing arg for short mode (-m only)" -m $IMAGE
run_test 1 "" "Missing game for short mode" -m stretch $IMAGE

echo "================================================="
echo "Tests completed: $pass passed, $fail failed."

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
