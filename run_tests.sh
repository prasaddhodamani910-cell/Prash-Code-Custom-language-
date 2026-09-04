#!/usr/bin/env bash
set -e

make -s

mkdir -p build/tests
PASS=0
FAIL=0

for file in examples/*.pd; do
    base=$(basename "$file" .pd)
    echo -n "Testing $base... "
    
    # Compile
    ./build/prashc "$file" -o "build/tests/$base"
    
    # Run and capture output
    ./build/tests/$base > "build/tests/$base.out"
    
    # Compare with expected
    if cmp -s "build/tests/$base.out" "examples/$base.expected"; then
        echo -e "\e[32mPASS\e[0m"
        PASS=$((PASS + 1))
    else
        echo -e "\e[31mFAIL\e[0m"
        diff -u "examples/$base.expected" "build/tests/$base.out"
        FAIL=$((FAIL + 1))
    fi
done

echo "Tests: $PASS passed, $FAIL failed."
if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
