#!/bin/bash
# Run this file from the project root with: ./scripts/dense_runs.sh

sizes=( 64 128 256 512 )

for i in "${sizes[@]}"; do
	for ((n=0;n<10;n++)); do
		echo iteration $n: dense_mm $i;
		./user/dense_mm $i;
	done
done
