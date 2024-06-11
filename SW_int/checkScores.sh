#!/bin/bash

NUM_DATABASE=${1:-40000}
NUM_BYTES=$(( $NUM_DATABASE * 1000  ))

# Check if score files are the same

RED='\033[0;31m'
GREEN='\033[0;32m'
NO_COLOR='\033[0m' # No Color

if cmp -n $NUM_BYTES scores.bin ../testdata/scores.bin > /dev/null; then
    printf "${GREEN}SUCCESS: Score files are the same${NO_COLOR}\n"
else
    printf "${RED}ERROR: Score files are different${NO_COLOR}\n"
fi
