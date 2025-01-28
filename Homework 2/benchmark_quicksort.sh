#!/bin/bash

# Array sizes and thread counts
ARRAY_SIZES=(1000000 5000000 10000000 50000000)
THREAD_COUNTS=(1 2 4 8)
RUNS=5  # Number of runs for each configuration
RESULT_FILE="quicksort_benchmark.csv"

# Initialize the results file
echo "Array Size,Threads,Serial Time,Parallel Time,Speedup" > $RESULT_FILE

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

# Run benchmarks
for size in "${ARRAY_SIZES[@]}"; do
    # Get serial execution time
    serial_times=()
    for ((i = 0; i < RUNS; i++)); do
        output=$(./quick $size 1 | grep "Serial Time" | awk '{print $3}')
        if [[ -z "$output" ]]; then
            echo "Error: Unable to capture serial time for size=$size"
            output=0
        fi
        serial_times+=("$output")
    done
    SERIAL_MEDIAN=$(calculate_median "${serial_times[@]}")
    echo "Array Size: $size, Serial Time: $SERIAL_MEDIAN seconds"

    for threads in "${THREAD_COUNTS[@]}"; do
        export OMP_NUM_THREADS=$threads
        parallel_times=()
        for ((i = 0; i < RUNS; i++)); do
            output=$(./quick $size $threads | grep "Parallel time" | awk '{print $3}')
            if [[ -z "$output" ]]; then
                echo "Error: Unable to capture parallel time for size=$size, threads=$threads"
                output=0
            fi
            parallel_times+=("$output")
        done
        PARALLEL_MEDIAN=$(calculate_median "${parallel_times[@]}")

        # Calculate speedup
        if (( $(echo "$PARALLEL_MEDIAN == 0" | bc -l) )); then
            SPEEDUP="undefined"
        else
            SPEEDUP=$(echo "$SERIAL_MEDIAN / $PARALLEL_MEDIAN" | bc -l)
        fi

        # Print and save the results
        echo "Array Size: $size, Threads: $threads, Serial Time: $SERIAL_MEDIAN, Parallel Time: $PARALLEL_MEDIAN, Speedup: $SPEEDUP"
        echo "$size,$threads,$SERIAL_MEDIAN,$PARALLEL_MEDIAN,$SPEEDUP" >> $RESULT_FILE
    done
done

echo "Benchmarking completed. Results saved to $RESULT_FILE."
