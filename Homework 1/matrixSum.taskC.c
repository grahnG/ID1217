/* matrix summation using pthreads

   features: uses a barrier; the Worker[0] computes
             the total sum from partial sums computed by Workers
             and prints the total sum to the standard output

   usage under Linux:
     gcc matrixSum.c -lpthread
     a.out size numWorkers

*/
#ifndef _REENTRANT 
#define _REENTRANT 
#endif 
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
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

/* Global Variables */
pthread_mutex_t barrierLock = PTHREAD_MUTEX_INITIALIZER; /* Mutex lock for the barrier */
pthread_cond_t barrierCond = PTHREAD_COND_INITIALIZER;   /* Condition variable for the barrier */
int numWorkers;                                         /* Number of workers */
int numArrived = 0;                                     /* Number of threads that arrived at the barrier */
int size, stripSize;                                    /* Matrix size and strip size */
int matrix[MAXSIZE][MAXSIZE];                           /* Matrix */
int partialSums[MAXWORKERS];                            /* Partial sums */
MatrixElement maxPerWorker[MAXWORKERS];                 /* Per-worker max elements */
MatrixElement minPerWorker[MAXWORKERS];                 /* Per-worker min elements */
int nextRow;
pthread_mutex_t rowLock;


/* Function Prototypes */
void Barrier();
double read_timer();
void initializeMatrix();
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

    /* Initialize matrix */
    initializeMatrix(seed);

    /* Set thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    nextRow = 0;
    /* Start timer */
    double start_time = read_timer();

    /* Create worker threads */
    for (t = 0; t < numWorkers; t++) {
        pthread_create(&workers[t], &attr, Worker, (void *)t);
    }

    /* Wait for all threads to complete */
    for (t = 0; t < numWorkers; t++) {
        pthread_join(workers[t], NULL);
    }

    /* Stop timer */
    double end_time = read_timer();

    /* Print execution time */
    printf("Execution time: %g sec\n", end_time - start_time);

    return 0;
}

/* Worker Function */
void *Worker(void *arg) {
    long id = (long)arg;
    
    /* Local variables */
    int localSum = 0;
    MatrixElement localMax = { .value = INT_MIN };
    MatrixElement localMin = { .value = INT_MAX };
    int workerRow;

    /* Process assigned strip */
    while (1) {
        pthread_mutex_lock(&rowLock);
        if (nextRow >= size) {  
            pthread_mutex_unlock(&rowLock);
            break;
        }
        workerRow = nextRow;
        nextRow++;
        pthread_mutex_unlock(&rowLock);


    }
    for (int j = 0; j < size; j++) {
        localSum += matrix[workerRow][j];

        if (matrix[workerRow][j] > localMax.value) {
            localMax.value = matrix[workerRow][j];
            localMax.row = workerRow;
            localMax.col = j;
        }
        if (matrix[workerRow][j] < localMin.value) {
            localMin.value = matrix[workerRow][j];
            localMin.row = workerRow;
            localMin.col = j;
        }
    }
    /* Save results */
    partialSums[id] = localSum;
    maxPerWorker[id] = localMax;
    minPerWorker[id] = localMin;

    /* Synchronize using the barrier */
    Barrier();

    /* Worker 0 aggregates and prints results */
    if (id == 0) {
        int totalSum = 0;
        MatrixElement globalMax = { .value = INT_MIN };
        MatrixElement globalMin = { .value = INT_MAX };

        for (int i = 0; i < numWorkers; i++) {
            totalSum += partialSums[i];

            if (maxPerWorker[i].value > globalMax.value) {
                globalMax = maxPerWorker[i];
            }

            if (minPerWorker[i].value < globalMin.value) {
                globalMin = minPerWorker[i];
            }
        }

        /* Print results */
        printf("The total sum is: %d\n", totalSum);
        printf("The maximum value is %d at position (%d, %d)\n", globalMax.value, globalMax.row, globalMax.col);
        printf("The minimum value is %d at position (%d, %d)\n", globalMin.value, globalMin.row, globalMin.col);
    }

    return NULL;
}

/* Reusable Barrier Implementation */
void Barrier() {
    pthread_mutex_lock(&barrierLock);
    numArrived++;
    if (numArrived == numWorkers) {
        numArrived = 0;
        pthread_cond_broadcast(&barrierCond);
    } else {
        pthread_cond_wait(&barrierCond, &barrierLock);
    }
    pthread_mutex_unlock(&barrierLock);
}

/* Timer Function */
double read_timer() {
    static struct timeval start;
    static bool initialized = false;
    struct timeval end;

    if (!initialized) {
        gettimeofday(&start, NULL);
        initialized = true;
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