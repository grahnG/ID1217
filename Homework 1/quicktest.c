#ifndef _REENTRANT 
#define _REENTRANT 
#endif 
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define THRESHOLD 100000 // Threshold for switching to serial sort
#define MAXTHREADS 10

    pthread_t workers[MAXTHREADS];
    pthread_attr_t attr;
    long t;

/* Structure for passing arguments to threads */
typedef struct {
    int left, right;
    int *array;
} Task;

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

/* Worker function for parallel quicksort */
void *parallelQuicksort(void *arg) {
    Task *task = (Task *)arg;
    int left = task->left;
    int right = task->right;
    int *array = task->array;


    if (left < right) {
        int pivotIndex = partition(left, right, array);



        if ((right - left) > THRESHOLD) {
            phtread_create(&workers[t], &attr, parallelQuicksort, (void *)task);
            parallelQuicksort(right);
            pthread_join(workers[t],NULL);

        } else {
            // Fallback to serial quicksort for smaller partitions
            serialQuicksort(left, pivotIndex - 1, array);
            serialQuicksort(pivotIndex + 1, right, array);
        }
    }

    free(task);
    return NULL;
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("Usage: %s <array_size> <num_threads>\n", argv[0]);
        return 1;
    }

    int arraySize = atoi(argv[1]);
    int numThreads = (argc > 2) ? atoi(argv[2]) : MAXTHREADS;

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
    clock_t start = clock();
    serialQuicksort(0, arraySize - 1, array);
    double serialTime = (double)(clock() - start) / CLOCKS_PER_SEC;

    // Measure parallel quicksort
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_t mainThread;
    Task *initialTask = (Task *)malloc(sizeof(Task));
    initialTask->left = 0;
    initialTask->right = arraySize - 1;
    initialTask->array = copy;

    

    start = clock();
    pthread_create(&mainThread, NULL, parallelQuicksort, initialTask);
    pthread_join(mainThread, NULL);
    double parallelTime = (double)(clock() - start) / CLOCKS_PER_SEC;

    // Output results
    printf("Array Size: %d\n", arraySize);
    printf("Serial Time: %f seconds\n", serialTime);
    printf("Parallel Time: %f seconds\n", parallelTime);

    free(array);
    free(copy);
    return 0;
}
