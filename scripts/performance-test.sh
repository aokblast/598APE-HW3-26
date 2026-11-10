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
time=$({ /usr/bin/time -f "%e" ./main.exe 1000 > /dev/null; } 2>&1
echo $time
default_values+=($time)

done

average=$(printf "%s\n" "${default_values[@]}" | awk '{sum+=$1} END {print sum/NR}')
echo "Default Runtime: $average"
