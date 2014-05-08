#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "timer.h"

double* x;
double* c;
double** A;
double* y;

int n; //Problem size.
int t; //Iterations.
int threadCount; //Number of Threads.
int type; //Solution type.

int barrierCount = 0;

pthread_mutex_t lock;
pthread_cond_t cond;
pthread_barrier_t barrier;

int* getRange(int myrank, int n, int p) {
  int quotient = n / p;
  int remainder = n % p;
  int myCount;
  int* range = malloc(2 * sizeof(int));
  if (myrank < remainder) {
      myCount = quotient + 1;
      range[0] = myrank * myCount;
  } else {
      myCount = quotient;
      range[0] = myrank * myCount + remainder;
  }
  range[1] = range[0] + myCount;
  return range;
}

double calculateDotProduct(double* a, double* b) {
double total = 0;
int i;
for (i = 0; i < n; i++) {
total += (a[i] * b[i]);
}
return total;
}

void conditionalBarrier() {
pthread_mutex_lock(&lock);
if ((barrierCount + 1) == threadCount) {
while (barrierCount > 0) {
pthread_cond_signal(&cond);
barrierCount--;
}
} else {
barrierCount++;
pthread_cond_wait(&cond, &lock);
}
pthread_mutex_unlock(&lock);
}

void* threadFunction(void* me) {
//Calculate my range:
int id = (int) me;
int* range = getRange(id, n, threadCount);
int start = range[0];
int finish = range[1];
free(range);

//Generate the correct matrix rows:
int i, j;
for (i = start; i < finish; i++) {
A[i] = malloc(n * sizeof(double));
for (j = 0; j < n; j++) {
if (i == j) {
A[i][j] = 0;
} else {
A[i][j] = -1/(double)n;
}
}
}

int currentInteration = 0;
for (currentInteration = 0; currentInteration < t; currentInteration++) {
for (i = start; i < finish; i++) {
y[i] = calculateDotProduct(A[i], x) + c[i];
}

//Wait until everyone has finished using x:
if (type == 1) {
conditionalBarrier();
} else {
pthread_barrier_wait(&barrier);
}

//Update x:
for (i = start; i < finish; i++) {
x[i] = y[i];
}

//Wait until everyone has updated x:
if (type == 1) {
conditionalBarrier();
} else {
pthread_barrier_wait(&barrier);
}

}

return NULL;
}

int main() {
double start, finish, elapsed;
pthread_t* threads;

//Get input:
printf("Enter problem size (n):\n");
scanf("%d", &n);
printf("Enter iterations (t):\n");
scanf("%d", &t);
printf("Enter thread count:\n");
scanf("%d", &threadCount);
printf("Enter synchronization type (1 = cond, 2 = barrier):\n");
scanf("%d", &type);

GET_TIME(start);

//Declare global data structures:
x = malloc(n * sizeof(double));
c = malloc(n * sizeof(double));
A = malloc(n * sizeof(double*));
y = malloc(n * sizeof(double));
if (type == 1) {
pthread_mutex_init(&lock, NULL);
pthread_cond_init(&cond, NULL);
} else {
pthread_barrier_init(&barrier, NULL, threadCount);
}

int i;

//Generate x:
for (i = 0; i < n; i++) {
x[i] = 0;
}

//Generate c:
for (i = 0; i < n; i++) {
c[i] = (double)i / (double)n;
}

//Generate threads:
threads = malloc(threadCount * sizeof(pthread_t));
for (i = 0; i < threadCount; i++) {
pthread_create(&threads[i], NULL, threadFunction, (void*) i);
}

for (i = 0; i < threadCount; i++) {
pthread_join(threads[i], NULL);
}

GET_TIME(finish);
elapsed = finish - start;

printf("Time to complete: %.2f seconds.\n", elapsed);

printf("Up to first 30 elements in x:\n");
for (i = 0; i < n && i < 30; i++) {
printf("%f\n", x[i]);
}

//Clean up:
free(x);
free(c);
for (i = 0; i < n; i++) {
free(A[i]);
}
free(A);
free(y);
if (type == 1) {
pthread_mutex_destroy(&lock);
pthread_cond_destroy(&cond);
} else {
pthread_barrier_destroy(&barrier);
}

return 0;
}