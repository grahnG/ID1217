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
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numWorkers;           /* number of workers */ 
int numArrived = 0;       /* number who have arrived */

typedef struct {
    int total;       /* total sum of all matrix elements */    
    int maxVal;      /* maximum value found in the matrix */
    int minVal;      /* minimum value found in the matrix */
    int maxRow;      /* row index of the maximum value */
    int minRow;      /* row index of the minimum value */
    int maxCol;      /* column index of the maximum value */
    int minCol;      /* column index of the minimum value */
} result;

result globalResult; /* shared variable to store the global results of computations */
pthread_mutex_t resultLock; /* mutex lock to ensure thread-safe updates to the shared variable */

/* a reusable counter barrier */
void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == numWorkers) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else
    pthread_cond_wait(&go, &barrier);
  pthread_mutex_unlock(&barrier);
}

/* timer */
double read_timer() {
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }
    gettimeofday( &end, NULL );
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

double start_time, end_time; /* start and end times */
int size, stripSize;  /* assume size is multiple of numWorkers */
int sums[MAXWORKERS]; /* partial sums */
int matrix[MAXSIZE][MAXSIZE]; /* matrix */

void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i, j;
  long l; /* use long in case of a 64-bit system */
  pthread_attr_t attr;
  pthread_t workerid[MAXWORKERS];

  /* initialize global result */
  globalResult.total = 0;
  globalResult.maxVal = INT_MIN;
  globalResult.minVal = INT_MAX;

  int serialMax = INT_MIN, serialMin = INT_MAX;
  int serialMaxRow = -1, serialMaxCol = -1;
  int serialMinRow = -1, serialMinCol = -1;

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex and condition variable */
  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);
  pthread_mutex_init(&resultLock, NULL);

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
  stripSize = size/numWorkers;

  /* initialize the matrix */
  for (i = 0; i < size; i++) {
	  for (j = 0; j < size; j++) {
          matrix[i][j] = rand()%100;
	  }
  }

  /* serial computation of sum */
 int serialSum = 0;
for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
        serialSum += matrix[i][j];
    }
} 
printf("Serial sum is: %d\n", serialSum); 

/* Serial computation of min, max, and their positions */
for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
        if (matrix[i][j] > serialMax) {
            serialMax = matrix[i][j];
            serialMaxRow = i;
            serialMaxCol = j;
        }
        if (matrix[i][j] < serialMin) {
            serialMin = matrix[i][j];
            serialMinRow = i;
            serialMinCol = j;
        }
    }
}

/* Print serial results */
printf("Serial Max: %d at (%d, %d)\n", serialMax, serialMaxRow, serialMaxCol);
printf("Serial Min: %d at (%d, %d)\n", serialMin, serialMinRow, serialMinCol);

  /* print the matrix */
#ifdef DEBUG
  for (i = 0; i < size; i++) {
	  printf("[ ");
	  for (j = 0; j < size; j++) {
	    printf(" %d", matrix[i][j]);
	  }
	  printf(" ]\n");
  }
#endif

  /* do the parallel work: create the workers */
  start_time = read_timer();

  for (l = 0; l < numWorkers; l++)
    pthread_create(&workerid[l], &attr, Worker, (void *) l);

    /* wait for all workers to finish */
    for (l = 0; l < numWorkers; l++) {
        pthread_join(workerid[l], NULL);
    }

    /* stop timer */
    double end_time = read_timer();

    /* print final results */
    printf("The total is %d\n", globalResult.total);
    printf("The maximum value is %d at postion (%d, %d)\n" ,
               globalResult.maxVal, globalResult.maxRow, globalResult.maxCol);
    printf("The minimum value is %d at position (%d, %d)\n",
               globalResult.minVal, globalResult.minRow, globalResult.minCol);
    printf("The execution time is %g sec\n", end_time - start_time);

    pthread_exit(NULL);

}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
  long myid = (long) arg;
  int total, i, j, first, last, localSum;
  int localMax, localMin;
  int localMaxRow, localMaxCol, localMinRow, localMinCol;

#ifdef DEBUG
  printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif

  /* determine first and last rows of my strip */
  first = myid*stripSize;
  last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);

  /* initialize local variables */
  localSum = 0;
  localMax = matrix[first][0];
  localMin = matrix[first][0];
  localMaxRow = first;
  localMaxCol = 0;
  localMinRow = first;
  localMinCol = 0;


  /* computing the local sum, max and min */
  for (i = first; i <= last; i++) {
    for (j = 0; j < size; j++) {
        localSum += matrix[i][j];
        if (matrix[i][j] > localMax) {
            localMax = matrix[i][j];
            localMaxRow = i;
            localMaxCol = j;
        }
        if (matrix[i][j] < localMin) {
            localMin = matrix[i][j];
            localMinRow = i;
            localMinCol = j;
        }
    }
  }

  /* update the global result using mutex */
  pthread_mutex_lock(&resultLock);
  globalResult.total += localSum;

  if (localMax > globalResult.maxVal) {
    globalResult.maxVal = localMax;
    globalResult.maxRow = localMaxRow;
    globalResult.maxCol = localMaxCol;
  }

  if (localMin < globalResult.minVal) {
    globalResult.minVal = localMin;
    globalResult.minRow = localMinRow;
    globalResult.minCol = localMinCol;
  }
  pthread_mutex_unlock(&resultLock);

  return NULL;
}