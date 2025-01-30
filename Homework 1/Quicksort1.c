#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>  // For gettimeofday()

#define THRESHOLD 100000  // Switch to serial sorting for small partitions
#define MAX_THREADS 8     // Max number of threads

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

/* Parallel Quicksort using Pthreads */
void *parallelQuicksort(void *arg) {
    QuickSortTask *task = (QuickSortTask *)arg;
    int left = task->left;
    int right = task->right;
    int *array = task->array;
    free(task);

    if (left < right) {
        int pivotIndex = partition(left, right, array);

        if ((right - left) > THRESHOLD) {
            pthread_t leftThread, rightThread;
            QuickSortTask *leftTask = malloc(sizeof(QuickSortTask));
            QuickSortTask *rightTask = malloc(sizeof(QuickSortTask));

            if (!leftTask || !rightTask) {
                serialQuicksort(left, pivotIndex - 1, array);
                serialQuicksort(pivotIndex + 1, right, array);
                return NULL;
            }

            leftTask->left = left;
            leftTask->right = pivotIndex - 1;
            leftTask->array = array;

            rightTask->left = pivotIndex + 1;
            rightTask->right = right;
            rightTask->array = array;

            pthread_create(&leftThread, NULL, parallelQuicksort, leftTask);
            pthread_create(&rightThread, NULL, parallelQuicksort, rightTask);

            pthread_join(leftThread, NULL);
            pthread_join(rightThread, NULL);
        } else {
            serialQuicksort(left, pivotIndex - 1, array);
            serialQuicksort(pivotIndex + 1, right, array);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <array_size> <num_threads>\n", argv[0]);
        return 1;
    }

    int arraySize = atoi(argv[1]);
    int numThreads = atoi(argv[2]);
    if (numThreads > MAX_THREADS) numThreads = MAX_THREADS;

    // Allocate and initialize the array
    int *array = (int *)malloc(sizeof(int) * arraySize);
    int *copy = (int *)malloc(sizeof(int) * arraySize);
    if (!array || !copy) {
        printf("Memory allocation error!\n");
        return 1;
    }

    srand(42);
    for (int i = 0; i < arraySize; i++) {
        array[i] = rand() % (arraySize * 10);
        copy[i] = array[i];
    }

    struct timeval startSerial, endSerial, startParallel, endParallel;

    // Measure serial quicksort
    gettimeofday(&startSerial, NULL);
    serialQuicksort(0, arraySize - 1, array);
    gettimeofday(&endSerial, NULL);
    double serialTime = (endSerial.tv_sec - startSerial.tv_sec) + (endSerial.tv_usec - startSerial.tv_usec) / 1e6;

    // Measure parallel quicksort
    gettimeofday(&startParallel, NULL);
    pthread_t mainThread;
    QuickSortTask *mainTask = malloc(sizeof(QuickSortTask));
    mainTask->left = 0;
    mainTask->right = arraySize - 1;
    mainTask->array = copy;
    pthread_create(&mainThread, NULL, parallelQuicksort, mainTask);
    pthread_join(mainThread, NULL);
    gettimeofday(&endParallel, NULL);
    double parallelTime = (endParallel.tv_sec - startParallel.tv_sec) + (endParallel.tv_usec - startParallel.tv_usec) / 1e6;

    // Output results
    printf("Array Size: %d\n", arraySize);
    printf("Serial Time: %f seconds\n", serialTime);
    printf("Parallel Time: %f seconds\n", parallelTime);

    free(array);
    free(copy);
    return 0;
}
