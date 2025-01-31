#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>  // For gettimeofday()

#define DEFAULT_ARRAY_SIZE         100000
#define DEFAULT_PARALLEL_THRESHOLD   5000

int g_parallel_threshold;

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

/* Struct for passing data to pthread */
typedef struct {
    int left;
    int right;
    int *array;
} QuickSortTask;

void *parallelQuicksortWorker(void *arg);

void parallelQuicksort(int left, int right, int *array) {
    int size = right - left + 1;
    if (size >= g_parallel_threshold) {
        int pivotIndex = partition(left, right, array);
        pthread_t leftThread;
        QuickSortTask task = { left, pivotIndex - 1, array };
        pthread_create(&leftThread, NULL, parallelQuicksortWorker, &task);
        parallelQuicksort(pivotIndex + 1, right, array);
        pthread_join(leftThread, NULL);
    } else {
        serialQuicksort(left, right, array);
    }
}

void *parallelQuicksortWorker(void *arg) {
    QuickSortTask *task = (QuickSortTask *)arg;
    parallelQuicksort(task->left, task->right, task->array);
    return NULL;
}

void array_print(int *array, size_t size) {
    printf("[");
    if (size > 0) {
        printf(" %i", array[0]);
        for (size_t i = 1; i < size; i++) {
            printf(", %i", array[i]);
        }
    }
    printf(" ]\n");
}

bool array_equality(int *a, int *b, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

double timeDiff(struct timeval start, struct timeval end) {
	return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Usage: %s <array_size> <parallel_threshold> <print_array>\n", argv[0]);
    }

    int arraySize        = (argc > 1) ? atoi(argv[1]) : DEFAULT_ARRAY_SIZE;
    g_parallel_threshold = (argc > 2) ? atoi(argv[2]) : DEFAULT_PARALLEL_THRESHOLD;
	bool print_array     = (argc > 3) ? atoi(argv[3]) : false;

    printf("Array Size         : %d\n", arraySize);
    printf("Parallel Threshold : %d\n", g_parallel_threshold);

    // Allocate and initialize the array
    int *array = (int *)malloc(sizeof(int) * arraySize);
    int *copy  = (int *)malloc(sizeof(int) * arraySize);
    if (!array || !copy) {
        printf("Memory allocation error!\n");
        return 1;
    }

    srand(time(NULL));
    for (int i = 0; i < arraySize; i++) {
        array[i] = rand() % (arraySize * 10);
        copy[i] = array[i];
    }

    if (print_array) {
        array_print(array, arraySize);
        array_print(copy, arraySize);
    }

    struct timeval startSerial, endSerial, startParallel, endParallel;

    // Measure serial quicksort
    gettimeofday(&startSerial, NULL);
    serialQuicksort(0, arraySize - 1, array);
    gettimeofday(&endSerial, NULL);
    double serialTime = timeDiff(startSerial, endSerial);

    // Measure parallel quicksort
    gettimeofday(&startParallel, NULL);
    parallelQuicksort(0, arraySize - 1, copy);
    gettimeofday(&endParallel, NULL);
    double parallelTime = timeDiff(startParallel, endParallel);

    // Output results
    printf("Serial Time        : %f seconds\n", serialTime);
    printf("Parallel Time      : %f seconds\n", parallelTime);
    printf("Array Equality?    : %s\n", array_equality(array, copy, arraySize) ? "True" : "False");

    if (serialTime < parallelTime) {
        printf("Parallel quicksort was %.2fx slower.\n", (float)parallelTime / (float)serialTime);
    } else {
        printf("Parallel quicksort was %.2fx faster.\n", (float)serialTime / (float)parallelTime);
    }

    if (print_array) {
        array_print(array, arraySize);
        array_print(copy, arraySize);
    }

    free(array);
    free(copy);

    return 0;
}



