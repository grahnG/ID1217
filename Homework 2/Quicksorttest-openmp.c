#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#define THRESHOLD 100000  // Threshold for switching to serial sort

/* Swap helper function */
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/* Median-of-Three Pivot Selection */
int medianOfThree(int left, int right, int *array) {
    int mid = left + (right - left) / 2;
    if (array[left] > array[mid]) swap(&array[left], &array[mid]);
    if (array[left] > array[right]) swap(&array[left], &array[right]);
    if (array[mid] > array[right]) swap(&array[mid], &array[right]);
    return mid;
}

/* Partition function for quicksort */
int partition(int left, int right, int *array) {
    int pivotIndex = medianOfThree(left, right, array);
    swap(&array[pivotIndex], &array[right]);
    int pivot = array[right];
    int i = left - 1;

    for (int j = left; j < right; j++) {
        if (array[j] < pivot) {
            i++;
            swap(&array[i], &array[j]);
        }
    }
    swap(&array[i + 1], &array[right]);
    return i + 1;
}

/* Serial Quicksort */
void serialQuicksort(int left, int right, int *array) {
    if (left < right) {
        int pivotIndex = partition(left, right, array);
        serialQuicksort(left, pivotIndex - 1, array);
        serialQuicksort(pivotIndex + 1, right, array);
    }
}

/* Parallel Quicksort */
void parallelQuicksort(int left, int right, int *array) {
    if (left < right) {
        int pivotIndex = partition(left, right, array);

        if ((right - left) > THRESHOLD) {
            #pragma omp task
            parallelQuicksort(left, pivotIndex - 1, array);

            #pragma omp task
            parallelQuicksort(pivotIndex + 1, right, array);

            #pragma omp taskwait
        } else {
            serialQuicksort(left, pivotIndex - 1, array);
            serialQuicksort(pivotIndex + 1, right, array);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <array_size> <num_threads>\n", argv[0]);
        return 1;
    }

    int arraySize = atoi(argv[1]);
    int numThreads = atoi(argv[2]);

    // Allocate and initialize the array
    int *array = (int *)malloc(sizeof(int) * arraySize);
    int *copy = (int *)malloc(sizeof(int) * arraySize);
    if (!array || !copy) {
        printf("Memory allocation error!\n");
        return 1;
    }

    srand(42); // Fixed seed for reproducibility
    for (int i = 0; i < arraySize; i++) {
        array[i] = rand() % (arraySize * 10);
        copy[i] = array[i];
    }

    // Measure serial quicksort
    double start = omp_get_wtime();
    serialQuicksort(0, arraySize - 1, array);
    double serialTime = omp_get_wtime() - start;

    // Measure parallel quicksort
    omp_set_num_threads(numThreads);
    start = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp single nowait
        parallelQuicksort(0, arraySize - 1, copy);
    }
    double parallelTime = omp_get_wtime() - start;

    // Output results
    printf("Array Size: %d\n", arraySize);
    printf("Serial Time: %f seconds\n", serialTime);
    printf("Parallel Time: %f seconds\n", parallelTime);

    free(array);
    free(copy);
    return 0;
}
