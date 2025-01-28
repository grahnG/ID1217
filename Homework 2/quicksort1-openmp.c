#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <omp.h>

/* Generate a random float between low and high */
double generateRandomFloat(double low, double high) {
    return ((double)rand() * (high - low)) / (double)RAND_MAX + low;
}

/* Insertion Sort for small arrays */
void insertionSort(float array[], int size) {
    for (int i = 1; i < size; i++) {
        float temp = array[i];
        int j = i - 1;
        while (j >= 0 && array[j] > temp) {
            array[j + 1] = array[j];
            j = j - 1;
        }
        array[j + 1] = temp;
    }
}

/* Partition function for quicksort */
int partition(int left, int right, float *array) {
    float pivot = array[left];
    int i = left + 1;
    int j = right;

    while (i <= j) {
        while (i <= right && array[i] <= pivot) i++;
        while (j >= left && array[j] > pivot) j--;
        if (i < j) {
            float temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }

    float temp = array[left];
    array[left] = array[j];
    array[j] = temp;

    return j;
}

/* Parallel Quicksort */
void parallelQuicksort(float *array, int left, int right, int threshold) {
    if (left < right) {
        if ((right - left) < threshold) {
            insertionSort(&array[left], right - left + 1);
        } else {
            int pivotIndex = partition(left, right, array);

#pragma omp task
            parallelQuicksort(array, left, pivotIndex - 1, threshold);

#pragma omp task
            parallelQuicksort(array, pivotIndex + 1, right, threshold);

#pragma omp taskwait
        }
    }
}

/* Perform one iteration of quicksort and measure the time */
double runQuicksort(float *array, int size, int threshold) {
    float *tempArray = (float *)malloc(sizeof(float) * size);
    if (!tempArray) {
        printf("Memory allocation error!\n");
        exit(1);
    }

    // Copy the array for sorting
    for (int i = 0; i < size; i++) {
        tempArray[i] = array[i];
    }

    double startTime = omp_get_wtime();
#pragma omp parallel
    {
#pragma omp single nowait
        parallelQuicksort(tempArray, 0, size - 1, threshold);
    }
    double endTime = omp_get_wtime();

    free(tempArray);
    return endTime - startTime;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <array_size> <num_threads>\n", argv[0]);
        return 1;
    }

    int arraySize = atoi(argv[1]);
    int numThreads = atoi(argv[2]);
    int threshold = 1000; // Threshold for switching to insertion sort

    omp_set_num_threads(numThreads);

    // Initialize random array
    float *array = (float *)malloc(sizeof(float) * arraySize);
    if (!array) {
        printf("Memory allocation error!\n");
        return 1;
    }

    srand(42); // Seed for reproducibility
    for (int i = 0; i < arraySize; i++) {
        array[i] = generateRandomFloat(0.0, 100.0);
    }

    // Measure serial time
    omp_set_num_threads(1);
    double serialTime = runQuicksort(array, arraySize, threshold);

    // Measure parallel time
    omp_set_num_threads(numThreads);
    double parallelTime = runQuicksort(array, arraySize, threshold);

    // Output results
    printf("Array Size: %d\n", arraySize);
    printf("Serial Time: %f seconds\n", serialTime);
    printf("Parallel Time: %f seconds\n", parallelTime);

    free(array);
    return 0;
}
