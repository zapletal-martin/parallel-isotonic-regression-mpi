#include <stdio.h>  /* printf and BUFSIZ defined there */
#include <stdlib.h> /* exit defined there */
#include <mpi.h>    /* all MPI-2 functions defined there */

struct LabeledPoint
{
   double label;
   double feature;
} labeledPoint; 


int sum(struct LabeledPoint *input, int length) {
   int sum = 0;
   int i = 0;

   for(i = 0; i < length; i++) {
      sum += input[i].label;
   }  

   return sum;
}

void pool(struct LabeledPoint *input, int length) {
   int i;

   double sumCache = sum(input, length);

   for(i = 0; i < length; i++) {
      input[i].label = sumCache / length;
   }
}

struct LabeledPoint *pava(struct LabeledPoint *input, int length) {
	int i;

   for(i = 0; i < length; i++) {
      int j = 0;

      for(j = i; j > 0; j--) {
         if(input[j].label < input[j-1].label) {
            pool(&input[j-1], i-j+2);
         }
      }
   }

   return input;
}

void printArray(struct LabeledPoint *input, int length) {
   printf("\r\n-------------------------\r\n");
   
   int j = 0;
   for(j = 0; j < length; j++) {
     printf("%f ", input[j].label);
   }
   printf("\r\n-------------------------\r\n");
}

int main(argc, argv)
int argc;
char *argv[];
{
   int rank, size, length;
   char name[BUFSIZ];

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   MPI_Get_processor_name(name, &length);

   printf("name %s: hello world from process %d of %d\n", name, rank, size);

   struct LabeledPoint labels[20] = {{1, 1} , {2, 2}, {3, 3}, {3, 4}, {1, 5}, {6, 6}, {7, 7}, {8, 8}, {11, 9}, {9, 10}, {10, 11}, {12, 12}, {14, 13}, {15, 14}, {17, 15}, {16, 16}, {17, 17}, {18, 18}, {19, 19}, {20, 20}};
   
   printArray(labels, 20);
   struct LabeledPoint *newlabels;
   
   newlabels = pava(labels, 20);
   printArray(newlabels, 20);

   MPI_Finalize();

   exit(0);
}