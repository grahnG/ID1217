/* matrix summation using OpenMP

   usage with gcc (version 4.2 or higher required):
     gcc -O -fopenmp -o matrixSum-openmp matrixSum-openmp.c 
     ./matrixSum-openmp size numWorkers

*/

#include <omp.h>

double start_time, end_time;

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 8   /* maximum number of workers */

int numWorkers;
int size; 
int matrix[MAXSIZE][MAXSIZE];
void *Worker(void *);
int *parallelArr;
int *serialArr;
int rnd_int;
double serialTime;

void serialQuicksort(int start, int end, int arr[]);
int partition(int start, int end, int arr[]);
void swap(int* a, int* b);
void parallelQuicksort(int start, int end, int arr[]);
void insertSort(int arr[], int n);
int medianOfThree( int start, int end,int* arr);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i, j, total=0;

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;

  omp_set_num_threads(numWorkers);

  /* initialize the matrix */
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
  start_time = omp_get_wtime();
  serialQuicksort(0,size-1, serialArr);
  end_time = omp_get_wtime();
  serialTime = end_time - start_time;


  start_time = omp_get_wtime();
  parallelQuicksort(0, size-1, parallelArr);
    

  end_time = omp_get_wtime();

  printf("Serial Time: %g\n", serialTime);
  printf("Parallel time: %g\n", end_time - start_time);

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
  int  median= medianOfThree(start, end, arr);
  swap(&arr[median], &arr[end]);
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

void parallelQuicksort(int start, int end, int* arr) {
    if (start < end) {
      int pivot = partition( start, end, arr); // Dela upp arrayen

      if((end-start) > 100000){ // Skapa uppgifter för att sortera vänster och höger del parallellt
        #pragma omp task
        parallelQuicksort(start, pivot - 1, arr);  // Sortera vänster del

        #pragma omp task
        parallelQuicksort(pivot + 1, end, arr);    // Sortera höger del

        // Vänta på att båda uppgifterna ska slutföras
        #pragma omp taskwait
        }else{ 
            serialQuicksort(start, pivot -1, arr);
            serialQuicksort(pivot +1, end, arr);

        } 
    }
}
void insertSort(int arr[], int n)
{
    for (int i = 1; i < n; ++i) {
        int key = arr[i];
        int j = i - 1;

        /* Move elements of arr[0..i-1], that are
           greater than key, to one position ahead
           of their current position */
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}
int medianOfThree( int start, int end,int* arr) {
    int mid = start + (end - start) / 2;

      
    int a = arr[start], b = arr[mid], c = arr[end];

    /* checks what part is the median*/
    if ((a < b && b < c) || (c < b && b < a)) return mid;  
    if ((b < a && a < c) || (c < a && a < b)) return start;
    return end; 
}