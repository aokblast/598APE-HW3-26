#!/usr/bin/env bash

make

default_values=()

NROUNDS="${NROUNDS:-3}"

tmp=$(mktemp)

echo "Start running, will run for ${NROUNDS} rounds"

for ((i=0; i<NROUNDS; i++))
do

echo "Round $i"

echo "Running on Default:"
time=$(/usr/bin/time -f "%e" ./sandpile.exe 1000 2>&1 >/dev/null)
echo $time
default_values+=($time)

done

average=$(printf "%s\n" "${default_values[@]}" | awk '{sum+=$1} END {print sum/NR}')
echo "Default Runtime: $average"
