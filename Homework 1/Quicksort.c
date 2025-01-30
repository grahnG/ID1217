#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#define MAXSIZE 10000
#define MAXWORKERS 10
#define MAX_QUEUE_SIZE 10000

pthread_mutex_t barrier;
pthread_cond_t go;
int numWorkers;
int numArrived = 0;

void swap(int* a, int* b);
void serialQuicksort(int start, int end, int arr[]);
int partition(int start, int end, int arr[]);

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

int size;
int *parallelArr;
int *serialArr;
pthread_t workers[MAXWORKERS];
volatile bool work_done = false;
pthread_mutex_t work_done_mutex;
pthread_cond_t work_done_cond;

typedef struct {
  int left, right;
} Task;

typedef struct {
  Task tasks[MAX_QUEUE_SIZE];
  int front, rear, count;
  pthread_mutex_t lock;
  pthread_cond_t not_empty;
} TaskQueue;

TaskQueue taskQueue;

volatile int active_threads = 0; // Counter for active threads
pthread_mutex_t active_threads_mutex; // Mutex for active_threads counter

void initQueue(TaskQueue *q) {
  q->front = 0;
  q->rear = 0;
  q->count = 0;
  pthread_mutex_init(&q->lock, NULL);
  pthread_cond_init(&q->not_empty, NULL);
}

void enqueue(TaskQueue *q, Task task) {
  pthread_mutex_lock(&q->lock);
  if (q->count < MAX_QUEUE_SIZE) {
    q->tasks[q->rear] = task;
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    q->count++;
    pthread_cond_signal(&q->not_empty);
  }
  pthread_mutex_unlock(&q->lock);
}

int dequeue(TaskQueue *q, Task *task) {
  pthread_mutex_lock(&q->lock);
  while (q->count == 0 && !work_done) {
    pthread_cond_wait(&q->not_empty, &q->lock);
  }
  if (q->count == 0) {
    pthread_mutex_unlock(&q->lock);
    return 0;
  }
  *task = q->tasks[q->front];
  q->front = (q->front + 1) % MAX_QUEUE_SIZE;
  q->count--;
  pthread_mutex_unlock(&q->lock);
  return 1;
}

void *Worker(void *arg) {
  while (1) {
    Task task;
    if (dequeue(&taskQueue, &task)) {
      // Increment active_threads counter
      pthread_mutex_lock(&active_threads_mutex);
      active_threads++;
      pthread_mutex_unlock(&active_threads_mutex);

      int pivot = partition(task.left, task.right, parallelArr);
      if (task.left < pivot - 1) enqueue(&taskQueue, (Task){task.left, pivot - 1});
      if (pivot + 1 < task.right) enqueue(&taskQueue, (Task){pivot + 1, task.right});

      // Decrement active_threads counter
      pthread_mutex_lock(&active_threads_mutex);
      active_threads--;
      if (active_threads == 0 && taskQueue.count == 0) {
        // Signal that all work is done
        pthread_cond_signal(&work_done_cond);
      }
      pthread_mutex_unlock(&active_threads_mutex);
    } else {
      break;
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int i, rnd_int;
  long l;
  pthread_attr_t attr;
  pthread_t workerid[MAXWORKERS];

  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);
  pthread_mutex_init(&work_done_mutex, NULL);
  pthread_cond_init(&work_done_cond, NULL);
  pthread_mutex_init(&active_threads_mutex, NULL);

  size = (argc > 1) ? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2) ? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;

  parallelArr = (int *)malloc(size * sizeof(int));
  serialArr = (int *)malloc(size * sizeof(int));
  if (parallelArr == NULL || serialArr == NULL) {
    printf("Fel vid minnesallokering!\n");
    return 1;
  }

  srand(time(NULL));
  for (i = 0; i < size; i++) {
    rnd_int = rand() % (size * 10);
    serialArr[i] = rnd_int;
    parallelArr[i] = rnd_int;
  }

  printf("Original Unsorted Array: \n");
  printf("[");
  for (i = 0; i < size; i++) {
    if (i != size - 1) {
      printf("%d, ", serialArr[i]);
    } else {
      printf("%d", serialArr[i]);
    }
  }
  printf("]\n");

  // Serial Quicksort
  clock_t start_timeSerial = clock();
  serialQuicksort(0, size - 1, serialArr);
  double final_timeSerial = (double)(clock() - start_timeSerial) / CLOCKS_PER_SEC;

  initQueue(&taskQueue);

  // Parallel Quicksort
  clock_t start_timeParallel = clock();
  for (l = 0; l < numWorkers; l++) {
    pthread_create(&workerid[l], &attr, Worker, (void *)l);
  }
  enqueue(&taskQueue, (Task){0, size - 1});

  // Wait for all threads to finish
  pthread_mutex_lock(&active_threads_mutex);
  while (active_threads > 0 || taskQueue.count > 0) {
    pthread_cond_wait(&work_done_cond, &active_threads_mutex);
  }
  pthread_mutex_unlock(&active_threads_mutex);

  // Signal workers to exit
  work_done = true;
  pthread_cond_broadcast(&taskQueue.not_empty);

  for (l = 0; l < numWorkers; l++) {
    pthread_join(workerid[l], NULL);
  }
  double final_timeParallel = (double)(clock() - start_timeParallel) / CLOCKS_PER_SEC;

  printf("The Array sorted in a serial fashion, time in seconds: %lf\n", final_timeSerial);
  printf("The array sorted in a parallel fashion, time in seconds: %lf\n", final_timeParallel);

  free(serialArr);
  free(parallelArr);
}

void serialQuicksort(int start, int end, int arr[]) {
  if (start < end) {
    int pivot = partition(start, end, arr);
    serialQuicksort(start, pivot - 1, arr);
    serialQuicksort(pivot + 1, end, arr);
  }
}

int partition(int start, int end, int arr[]) {
  int mid = start + (end - start) / 2;
  int pivotIndex = mid;
  swap(&arr[pivotIndex], &arr[end]);
  int pivot = arr[end];
  int i = start - 1;

  for (int j = start; j < end; j++) {
    if (arr[j] < pivot) {
      i++;
      swap(&arr[i], &arr[j]);
    }
  }

  swap(&arr[i + 1], &arr[end]);
  return i + 1;
}

void swap(int* a, int* b) {
  int t = *a;
  *a = *b;
  *b = t;
}