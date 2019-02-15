#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <math.h>
#include <float.h>
#include <pthread.h>
#include <string.h>
#include "turbine_defines.h"
#include "time_functions.h"
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#endif
#define HAVE_STRUCT_TIMESPEC

float T = TFARM_CYCLE_TIME*1000;
float max_values[100];
float current_values[100];
float cycle_target_values[100];
float temp_array[ROWCOUNT][COLCOUNT];
float current_array[ROWCOUNT][COLCOUNT];
float max_array[ROWCOUNT][COLCOUNT];
float avg_Array[ROWCOUNT][COLCOUNT];
int i = 0;
int count = 0;
float target;
char* arr[100];
typedef struct turbine_pos {
  int m;
  int n;
} turbine_pos;
pthread_t thread[ROWCOUNT][COLCOUNT];
pthread_mutex_t mutex;
pthread_cond_t condition;

FILE * fpointer;  // filePointer to open textFile
FILE * fpointer2;

void path();
int average_func(int i, int j);
int adjust_avg(int i, int j);
void *thread_function (void* input);

int main () {

  path();

  //check if text file opens correctly
  if(fpointer == 0) {
      perror("fopen");
      exit(1);
    }
  else printf("file succesfully openned!\n");
  // read entire textFile to the arr array
  while (!(feof(fpointer))) {
    char *line = (char*)malloc(sizeof(char)*500);
    fgets(line, 500, fpointer);
    const char space[2] = " ";
    char* token;
    char* test[100];
    token = strtok(line, space);
    while(token != NULL) {
      test[i] = token;
      arr[i] = test[i];
      token = strtok(NULL, space);
      i++;
    }
  }
  //these 3 for loops put each value from the arr array into their respective values arrays
  for (i = 1 ; i < 16 ; i++) {
    max_values[i-1] = atof(arr[i]);
  }
  for (i = 17 ; i < 32 ; i++) {
    current_values[i-17] = atof(arr[i]);
  }
  for (i = 33 ; i < 39 ; i++) {
    cycle_target_values[i-33] = atof(arr[i]);
  }
  // placing current and max values into their given 2D arrays
    int n = 0;

    for (int i=0 ; i<ROWCOUNT ; i++) {
      for (int j=0 ; j<COLCOUNT ; j++) {
        current_array[i][j] = current_values[n];
        max_array[i][j] = max_values[n];
        n++;
      }
    }

/*    //check to see if arr array was entered correctly
    for (int j = 0; j < 39; j++) {
      printf("%s ", arr[j]);
    }
    //check to make sure the current array was entered correctly
    printf("this is the current array:\n" );
    for (i=0 ; i<ROWCOUNT ; i++) {
      for (j=0 ; j<COLCOUNT ; j++) {
        printf("%f ", current_array[i][j]);
      }
    }
    //check to make sure the max array was entered correctly
    printf("this is the max array:\n" );
    for (i=0 ; i<ROWCOUNT ; i++) {
      for (j=0 ; j<COLCOUNT ; j++) {
        printf("%f ", max_array[i][j]);
      }
    }
    //check specific indecies of each matrix
    printf("the value of the max array: %f\n",max_array[1][4]);
    printf("the value of the current array: %f\n",current_array[2][4]);
*/
  turbine_pos * position;
  for (int a = 0 ; a < 6 ; a++) {
    for (int i=0 ; i<ROWCOUNT ; i++) {
      for (int j=0 ; j<COLCOUNT ; j++) {
        position = malloc(sizeof(struct turbine_pos));
        (*position).m = i;
        (*position).n = j;
        target = cycle_target_values[a];
        pthread_create(&thread[i][j], NULL, thread_function, (void*)position);
        pthread_cond_broadcast(&condition);
      }
    }
    millisleep(TFARM_CYCLE_TIME * 1000);
    pthread_mutex_lock(&mutex);

  while (count < ROWCOUNT*COLCOUNT) {

      pthread_cond_wait(&condition, &mutex);
  }

      for (int i = 0; i < ROWCOUNT; i++) {
        for (int j = 0; j < COLCOUNT; j++) {
          fprintf(fpointer2, "%.2f", temp_array[i][j]);
          fprintf(fpointer2, "%s", " ");
          fflush(fpointer2);
          current_array[i][j] = temp_array[i][j];
        }
        fprintf(fpointer2, "%s", "\n");
        fflush(fpointer2);
      }
      fprintf(fpointer2, "%s\n", "**********");
      fflush(fpointer2);
      pthread_mutex_unlock(&mutex);
    }


  return 0;
}

//find the averages of each index in the current array and store in avg_Array
int average_func(int i, int j) {

      if (i==0 && j==0){
        avg_Array[i][j] = (current_array[i][j+1] + current_array[i+1][j] + current_array[i][j]) / 3;
      }
      else if (i==0 && j==(COLCOUNT-1)) {
        avg_Array[i][j] = (current_array[i][j-1] + current_array[i+1][j] + current_array[i][j]) / 3;
      }
      else if (i==(ROWCOUNT-1) && j==0) {
        avg_Array[i][j] = (current_array[i-1][j] + current_array[i][j+1] + current_array[i][j]) / 3;
      }
      else if (i==(ROWCOUNT-1) && j==(COLCOUNT-1)) {
        avg_Array[i][j] = (current_array[i][j-1] + current_array[i-1][j] + current_array[i][j]) / 3;
      }
      else if (i==0 && (j!=0 || j!=(COLCOUNT-1))){
        avg_Array[i][j] = (current_array[i][j-1] + current_array[i+1][j] + current_array[i][j+1] + current_array[i][j]) / 4;
      }
      else if (i==(ROWCOUNT-1) && (j!=0 || j!=(COLCOUNT-1))){
        avg_Array[i][j] = (current_array[i][j-1] + current_array[i-1][j] + current_array[i][j+1] + current_array[i][j]) / 4;
      }
      else if ((i!=0 || i!=(ROWCOUNT-1)) && j==0) {
        avg_Array[i][j] = (current_array[i-1][j] + current_array[i][j+1] + current_array[i+1][j] + current_array[i][j]) / 4;
      }
      else if ((i!=0 || i!=(ROWCOUNT-1)) && j==(COLCOUNT-1) ){
        avg_Array[i][j] = (current_array[i-1][j] + current_array[i][j-1] + current_array[i+1][j] + current_array[i][j]) / 4;
      }
      else {
        avg_Array[i][j] = (current_array[i][j-1] + current_array[i-1][j] + current_array[i][j+1] + current_array[i+1][j] + current_array[i][j]) / 5;
      }
  return 0;
}

int adjust_avg(int i, int j) {

        if (avg_Array[i][j] < target) {
          temp_array[i][j] = (current_array[i][j] * .3) + avg_Array[i][j];
        }
        if (avg_Array[i][j] >= target) {
          temp_array[i][j] = -(current_array[i][j] * .3) + avg_Array[i][j];
        }
        if (temp_array[i][j] < 0) {
          temp_array[i][j] = 0;
        }
        if (temp_array[i][j] >= max_array[i][j]) {
          temp_array[i][j] = max_array[i][j];
        }
  return 0;
}

void *thread_function (void* input) {

  turbine_pos *position2 = (turbine_pos*)input;
  int i = position2->m;
  int j = position2->n;
  average_func(i, j);
  adjust_avg(i, j);
  pthread_mutex_lock(&mutex);
  count++;
  pthread_cond_signal(&condition);
  pthread_mutex_unlock(&mutex);

  return NULL;
}

void path() {         // setpath and definitions .h files were not working, therefore created this path func inorder to set directories accordingly

  #ifdef _WIN32
    char in_path[]="C:\\temp\\coursein\\p3-in.txt";
    char out_path[]="C:\\temp\\courseout\\p3-out.txt";

  #else
    char in_path[200], out_path[200];
    const char *homedir;
    homedir = getenv("HOME");
    if (homedir!= NULL) {
      homedir = getpwuid(getuid())->pw_dir;
    }
    printf("my home directory is: %s\n", homedir);
    strcpy(in_path,homedir);
    strcpy(out_path,homedir);
    strcat(in_path,"/tmp/coursein/p3-in.txt");
    strcat(out_path,"/tmp/courseout/p3-out.txt");

  #endif
    fpointer = fopen(in_path, "r");
    fpointer2 = fopen(out_path, "w");

    if ((fpointer == NULL) || (fpointer2 == NULL)) {
      printf("Error...r/ The files were unable to be opened\n");
      exit(1);
    }
}
