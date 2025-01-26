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


/* function declarations */
void swap(int* a, int* b);
void serialQuicksort(int start, int end, int arr[]);
int partition(int start, int end, int arr[]);



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
int size;
int activeQueue;
  int *parallelArr;
  int *serialArr;

void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i;


  int rnd_int;
  long l;

  pthread_attr_t attr;
  pthread_t workerid[MAXWORKERS];

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex and condition variable */
  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);

  /* read command line args if any */
    size = (argc > 1) ? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc>2) ? atoi(argv[2]) : MAXWORKERS;
  if(size > MAXSIZE) size = MAXSIZE;
  if( numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;




  /* initialize the array */
  parallelArr = (int *)malloc(size * sizeof(int));
  serialArr = (int *)malloc(size * sizeof(int));
  if (parallelArr == NULL){
    printf("Fel vid minnesallokering!\n");
    return 1;
  }
  if (serialArr == NULL){
    printf("Fel vid minnesallokering!\n");
    return 1;
  }

  srand(time(NULL));
  for ( i = 0; i < size; i++) {
    rnd_int = rand() % (size*10);

    serialArr[i] = rnd_int;
    parallelArr[i] = rnd_int;
  }

/* printing the randomized array*/
  printf("[");
  for(i = 0; i <size; i++){
      if(i!= size-1){
      printf("%d, ",serialArr[i]);
    }else { printf("%d", serialArr[i]);
    }
  }
    printf("]\n");


  serialQuicksort(0, size-1, serialArr);

 activeQueue = 0;


  /* do the parallel work: create the workers */
  start_time = read_timer();
  for (l = 0; l < numWorkers; l++)
    pthread_create(&workerid[l], &attr, Worker, (void *) l);
  
  for( l = 0; l<numWorkers; l++){
    pthread_join(workerid[l],NULL);
  }
    end_time = read_timer();



/*final printout of the sorted arrays*/
  printf("[");
  for(i = 0; i <size; i++){
      if(i!= size-1){
      printf("%d, ",serialArr[i]);
    }else { printf("%d", serialArr[i]);
    }
  }
    printf("]\n");

      printf("[");
  for(i = 0; i <size; i++){
      if(i!= size-1){
      printf("%d, ",parallelArr[i]);
    }else { printf("%d", parallelArr[i]);
    }
  }
    printf("]\n");
  free(serialArr);
  free(parallelArr);
}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
  long id = (long)arg;
  if(id == 0) {
    serialQuicksort(0, size-1, parallelArr);
    activeQueue++;
  }
    
    
    
    
    /* get end time */
    

  
}
 
void serialQuicksort(int start, int end,int arr[]){

  if(start < end){

    int pivot = partition(start, end, arr);

    /* recursive calls, what will be parrallelized later(?)*/
    serialQuicksort(start, pivot-1, arr);
    serialQuicksort(pivot + 1, end, arr);
  }
}

/* serial partition function, low and high is indices */
/* partitions the array i.e splits the array into lower and higher values arround the pivot*/
int partition(int start, int end, int arr[]){
  //lägg till median of three för optimerad metod
  int mid = start + (end - start) / 2;
  
  /* Flytta pivot till slutet */
  int pivotIndex = mid;
  swap(&arr[pivotIndex], &arr[end]);
  int pivot = arr[end];

  int i = start - 1;

  /* sorting the arrays */
  for (int j = start; j < end; j++){
    if(arr[j] < pivot){
      i++;
      swap(&arr[i], &arr[j]);
    }
  }

  swap(&arr[i+1], &arr[end]);
  return i+1;
}

void swap(int* a, int* b ){
  int t = *a;
  *a = *b;
  *b = t ;
}