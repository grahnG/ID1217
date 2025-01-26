#ifndef _REENTRANT
#define _REENTRANT
#endif
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>

#define MAXSIZE 10000 /* Maximum matrix size */
#define MAXWORKERS 10 /* Maximum number of workers */

/* Struct to store a matrix element's value and position */
typedef struct {
    int row;
    int col;
    int value;
} MatrixElement;

/* Struct to store thread-local results */
typedef struct {
    int localSum;
    MatrixElement localMax;
    MatrixElement localMin;
} ThreadResult;

/* Global Variables */
int size, numWorkers, stripSize;  /* Matrix size, number of workers, strip size */
int matrix[MAXSIZE][MAXSIZE];     /* Matrix */
int nextRow = 0;                  /* Shared counter for "bag of tasks" */
pthread_mutex_t rowLock;          /* Mutex Lock for shared counter */

/* Function Prototypes */
double read_timer();
void initializeMatrix(int seed);
void *Worker(void *);

/* Main Function */
int main(int argc, char *argv[]) {
    pthread_t workers[MAXWORKERS];
    pthread_attr_t attr;
    long t;

    /* Read command-line arguments */
    size = (argc > 1) ? atoi(argv[1]) : MAXSIZE;
    numWorkers = (argc > 2) ? atoi(argv[2]) : MAXWORKERS;
    int seed = (argc > 3) ? atoi(argv[3]) : -1; // Default to -1 for no specific seed
    if (size > MAXSIZE) size = MAXSIZE;
    if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
    stripSize = size / numWorkers;

    /* Initialize matrix and mutex */
    initializeMatrix(seed);
    pthread_mutex_init(&rowLock, NULL);

    /* Set thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    /* Start timer */
    double start_time = read_timer();

    /* Create worker threads and collect their results */
    ThreadResult *results[MAXWORKERS];
    for (t = 0; t < numWorkers; t++) {
        pthread_create(&workers[t], &attr, Worker, (void *)t);
    }

    /* Aggregate results */
    int totalSum = 0;
    MatrixElement globalMax = { .value = INT_MIN };
    MatrixElement globalMin = { .value = INT_MAX };

    for (t = 0; t < numWorkers; t++) {
        ThreadResult *result;
        pthread_join(workers[t], (void **)&result);

        totalSum += result->localSum;

        if (result->localMax.value > globalMax.value) {
            globalMax = result->localMax;
        }
        if (result->localMin.value < globalMin.value) {
            globalMin = result->localMin;
        }

        /* Free the memory allocated by the thread */
        free(result);
    }

    /* Stop timer */
    double end_time = read_timer();

    /* Destroy the mutex */
    pthread_mutex_destroy(&rowLock);

    /* Print results */
    printf("The total sum is: %d\n", totalSum);
    printf("The maximum value is %d at position (%d, %d)\n", globalMax.value, globalMax.row, globalMax.col);
    printf("The minimum value is %d at position (%d, %d)\n", globalMin.value, globalMin.row, globalMin.col);
    printf("Execution time: %g sec\n", end_time - start_time);

    return 0;
}

/* Worker Function */
void *Worker(void *arg) {
    int row;
    long id = (long)arg;
    int firstRow = id * stripSize;
    int lastRow = (id == numWorkers - 1) ? size - 1 : (firstRow + stripSize - 1);

    /* Allocate memory for the thread's result */
    ThreadResult *result = (ThreadResult *)malloc(sizeof(ThreadResult));
    result->localSum = 0;
    result->localMax = (MatrixElement){ .value = INT_MIN };
    result->localMin = (MatrixElement){ .value = INT_MAX };

    while (1) {
        /* Critical section: Get a row from the shared counter */
        phtread_mutex_lock(&rowLock);
        row = nextRow;
        nextRow++;
        pthread_mutex_unlock(&rowLock);

        /* Check if all rows are processed */
        if (row >= size) {
            break;
        }
    }

    /* Process assigned strip */
    for (int i = firstRow; i <= lastRow; i++) {
        for (int j = 0; j < size; j++) {
            result->localSum += matrix[i][j];
            if (matrix[i][j] > result->localMax.value) {
                result->localMax = (MatrixElement){ .row = i, .col = j, .value = matrix[i][j] };
            }
            if (matrix[i][j] < result->localMin.value) {
                result->localMin = (MatrixElement){ .row = i, .col = j, .value = matrix[i][j] };
            }
        }
    }

    /* Return the result */
    return (void *)result;
}

/* Timer Function */
double read_timer() {
    static struct timeval start;
    static int initialized = 0;
    struct timeval end;

    if (!initialized) {
        gettimeofday(&start, NULL);
        initialized = 1;
    }

    gettimeofday(&end, NULL);
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

/* Initialize Matrix */
void initializeMatrix(int seed) {
    if (seed >= 0) {
        srand(seed); // Use the provided seed for reproducibility
    } else {
        srand(time(NULL)); // Use the current time for randomness
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i][j] = rand() % 100; /* Random values [0, 99] */
        }
    }
}