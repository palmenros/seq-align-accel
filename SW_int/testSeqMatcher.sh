#!/bin/bash

set -e

NUM_DATABASE_ENTRIES=${1:-40000}

make all

[ ! -f scores.bin ] || mv scores.bin scores.bin.bak
./seqMatcher $NUM_DATABASE_ENTRIES 1000 ../testdata/database.txt ../testdata/specimen.txt scores.bin

printf "\nCHECKING SCORES...\n\n"
 
 ./checkScores.sh $NUM_DATABASE_ENTRIES
