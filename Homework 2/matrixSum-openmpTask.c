/* matrix summation using OpenMP

   usage with gcc (version 4.2 or higher required):
     gcc -O -fopenmp -o matrixSum-openmp matrixSum-openmp.c 
     ./matrixSum-openmp size numWorkers

*/

#include <omp.h>
#include <stdlib.h>
#include <stdio.h>

double start_time, end_time;

#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 8   /* maximum number of workers */

int numWorkers;
int size; 
int matrix[MAXSIZE][MAXSIZE];
void *Worker(void *);

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
  for (i = 0; i < size; i++) {
    //  printf("[ ");
	  for (j = 0; j < size; j++) {
      matrix[i][j] = rand()%99;
      //	  printf(" %d", matrix[i][j]);
	  }
	  //	  printf(" ]\n");
  }

  int globalMaxRow = 0;
  int globalMaxColumn = 0;
  int globalMinRow = 0;
  int globalMinColumn = 0;

  start_time = omp_get_wtime();

#pragma omp parallel shared(globalMaxRow, globalMaxColumn, globalMinRow, globalMinColumn)
{
   int localMaxRow = 0;
   int localMaxColumn = 0;
   int localMinRow = 0;
   int localMinColumn = 0; 

   #pragma omp for reduction (+:total)
   for (i = 0; i < size; i++)
    for (j = 0; j < size; j++) {
        total += matrix[i][j];
        if(matrix[i][j] > matrix[localMaxRow][localMaxColumn]) 
        {
            localMaxRow = i;
            localMaxColumn = j;
        }
        if (matrix[i][j] < matrix[localMinRow][localMinColumn]) 
        {
            localMinRow = i;
            localMinColumn = j;
        }
    } 

    #pragma omp critical 
    {
        if (matrix[localMaxRow][localMaxColumn] > matrix[globalMaxRow][globalMaxColumn]) 
        {
            globalMaxRow = localMaxRow;
            globalMaxColumn = localMaxColumn;
        }
        if (matrix[localMinRow][localMinColumn] < matrix[globalMinRow][globalMinColumn]) 
        {
            globalMinRow = localMinRow;
            globalMinColumn = localMinColumn;
        }
    }
}

// implicit barrier

  end_time = omp_get_wtime();

  printf("the total is %d\n", total);
  printf("The maximum value is %d at (%d, %d)\n", matrix[globalMaxRow][globalMaxColumn], globalMaxRow, globalMaxColumn);
  printf("The minimum value is %d at (%d, %d)\n", matrix[globalMinRow][globalMinColumn], globalMinRow, globalMinColumn);
  printf("it took %g seconds\n", end_time - start_time);

  return 0;
}