#!/bin/bash
# Run this file from the project root with: sudo ./scripts/dense_runs.sh

sizes=( 64 128 256 512 )

for i in "${sizes[@]}"; do
	echo map_time, mult_time > results/output$i.csv;
	for ((n=0;n<10;n++)); do
		echo iteration $n: quicksort $i;
		./user/quicksort $i;
	done
done
