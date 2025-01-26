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
#define MAX_QUEUE_SIZE 10000


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

typedef struct{
  int left, right;
} Task;

typedef struct{ 
  Task tasks[MAX_QUEUE_SIZE];
  int front, rear, count;
  pthread_mutex_t lock;
} TaskQueue;

TaskQueue taskQueue;

/* initialize queue we just created*/

void initQueue(TaskQueue *q){
  q->front = 0;
  q->rear = 0;
  q->count = 0;
  pthread_mutex_init(&q->lock, NULL);
}

/* function to queue tasks*/
void enqueue(TaskQueue *q, Task task){
  pthread_mutex_lock(&q->lock);
  if(q->count < MAX_QUEUE_SIZE){
    q->tasks[q->rear] = task;
    q->rear = (q->rear+1) % MAX_QUEUE_SIZE;
    q->count++;
  }
  pthread_mutex_unlock(&q->lock);

}

/* function to remove from queue*/
int dequeue(TaskQueue *q, Task *task){
  pthread_mutex_lock(&q->lock);
  if(q->count == 0) {
    pthread_mutex_unlock (&q->lock);
    return 0;
  }
  *task = q->tasks[q->front];
  q->front = (q->front + 1 ) % MAX_QUEUE_SIZE;
  q->count--;
  pthread_mutex_unlock(&q->lock);
  return 1;
}

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
printf("Original Unsorted Array: \n");
  printf("[");
  for(i = 0; i <size; i++){
      if(i!= size-1){
      printf("%d, ",serialArr[i]);
    }else { printf("%d", serialArr[i]);
    }
  }
    printf("]\n");

  double end_timeSerial, start_timeSerial;
   start_timeSerial = read_timer();
  serialQuicksort(0, size-1, serialArr);
  end_timeSerial = read_timer();
  double final_timeSerial = end_timeSerial-start_timeSerial;

  initQueue(&taskQueue);

  /* do the parallel work: create the workers */
  start_time = read_timer();
  /* create an inital task, which is to partition the whole array*/
  enqueue(&taskQueue, (Task){0, size-1});
  for (l = 0; l < numWorkers; l++)
    pthread_create(&workerid[l], &attr, Worker, (void *) l);
  
  for( l = 0; l<numWorkers; l++){
    pthread_join(workerid[l],NULL);
  }
    end_time = read_timer();

double final_timeParallel = end_time - start_time;


/*final printout of the sorted arrays*/
  printf("The Array sorter in a serial fashion time in seconds: %lf\n", final_timeSerial );
  /*("\n [");
  for(i = 0; i <size; i++){
      if(i!= size-1){
      printf("%d, ",serialArr[i]);
    }else { printf("%d", serialArr[i]);
    }
  }
    printf("]\n");*/

  printf( "The array sorted in a parallel fashion, time in seconds: %lf\n", final_timeParallel);
      /*printf("\n [");
  for(i = 0; i <size; i++){
      if(i!= size-1){
      printf("%d, ",parallelArr[i]);
    }else { printf("%d", parallelArr[i]);
    }
  }
    printf("]\n");*/

  
  free(serialArr);
  free(parallelArr);
}

/* Each worker picks up  */
void *Worker(void *arg) {
  while (1) {
    Task task;
    /* if the taskQueue is empty exit the while loop */
    if(!dequeue(&taskQueue, &task)){
      break;
    }

    /* partition the array into smaller arrays, putting it on the left or right side of the pivot depending on the number*/
    int pivot= partition(task.left, task.right, parallelArr);
    
    if(task.left < pivot -1){
      enqueue(&taskQueue, (Task) {task.left, pivot -1});

    }
    if(pivot+1 < task.right){
      enqueue(&taskQueue, (Task){pivot + 1, task.right});
    }

  }
  return NULL;
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