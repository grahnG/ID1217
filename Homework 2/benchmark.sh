#!/bin/bash

# Define matrix sizes and thread counts
MATRIX_SIZES=(500 1000 2000)   # At least 3 different sizes
THREAD_COUNTS=(1 2 4 8)        # At least up to 4 processors
RUNS=5                         # Number of runs for each configuration
RESULT_FILE="benchmark_results.csv"

# Initialize the results file
echo "Matrix Size,Threads,Median Time,Speedup" > $RESULT_FILE

# Function to calculate the median
calculate_median() {
    local times=("$@")
    local count=${#times[@]}
    local middle=$((count / 2))
    # Sort the array
    sorted=($(printf '%s\n' "${times[@]}" | sort -n))
    if ((count % 2 == 0)); then
        # Even number of elements, average the two middle values
        echo "(${sorted[$middle-1]} + ${sorted[$middle]}) / 2" | bc -l
    else
        # Odd number of elements, return the middle value
        echo "${sorted[$middle]}"
    fi
}

# Loop through matrix sizes and thread counts
for size in "${MATRIX_SIZES[@]}"; do
    # Get the sequential execution time
    export OMP_NUM_THREADS=1
    seq_times=()
    for ((i = 0; i < RUNS; i++)); do
        output=$(./matrixSum-openmp $size 1 | grep "it took" | awk '{print $3}')
        if [[ -z "$output" ]]; then
            echo "Error: Unable to capture sequential time for size=$size"
            output=0
        fi
        seq_times+=("$output")
    done
    SEQUENTIAL_MEDIAN=$(calculate_median "${seq_times[@]}")
    echo "Matrix Size: $size, Sequential Time: $SEQUENTIAL_MEDIAN seconds"

    for threads in "${THREAD_COUNTS[@]}"; do
        export OMP_NUM_THREADS=$threads
        parallel_times=()
        for ((i = 0; i < RUNS; i++)); do
            output=$(./matrixSum-openmp $size $threads | grep "it took" | awk '{print $3}')
            if [[ -z "$output" ]]; then
                echo "Error: Unable to capture parallel time for size=$size, threads=$threads"
                output=0
            fi
            parallel_times+=("$output")
        done
        PARALLEL_MEDIAN=$(calculate_median "${parallel_times[@]}")
        
        # Avoid divide-by-zero errors
        if (( $(echo "$PARALLEL_MEDIAN == 0" | bc -l) )); then
            SPEEDUP="undefined"
        else
            SPEEDUP=$(echo "$SEQUENTIAL_MEDIAN / $PARALLEL_MEDIAN" | bc -l)
        fi

        echo "Matrix Size: $size, Threads: $threads, Median Time: $PARALLEL_MEDIAN, Speedup: $SPEEDUP"
        echo "$size,$threads,$PARALLEL_MEDIAN,$SPEEDUP" >> $RESULT_FILE
    done
done

echo "Benchmarking completed. Results saved to $RESULT_FILE."

