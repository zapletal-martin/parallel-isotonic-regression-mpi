#include <stdio.h>  /* printf and BUFSIZ defined there */
#include <stdlib.h> /* exit defined there */
#include <mpi.h>    /* all MPI-2 functions defined there */

struct LabeledPoint
{
   double label;
   double feature;
} labeledPoint; 

void printArray(struct LabeledPoint *input, int length) {
   printf("\r\n-------------------------\r\n");
   
   int j = 0;
   for(j = 0; j < length; j++) {
     printf("%f ", input[j].label);
   }
   printf("\r\n-------------------------\r\n");
}

int sum(struct LabeledPoint *input, int start, int end) {
   int sum = 0;

   for(; start <= end; start++) {
      sum += input[start].label;
   }  

   return sum;
}

void pool(struct LabeledPoint *input, int start, int end) {
   double weightedSum = sum(input, start, end);
   double weight = end - start + 1;

   for(; start <= end; start++) {
      input[start].label = weightedSum / weight;
   }
}

struct LabeledPoint *pava(struct LabeledPoint *input, int length) {
   int i = 0;

   while(i < length) {
      int j = i;

      //find monotonicity violating sequence, if any
      while(j < length - 1 && !(input[j].label <= input[j + 1].label)) {
        j = j + 1;
      }

      //if monotonicity was not violated, move to next data point
      if(i == j) {
        i = i + 1;
      } else {
        //otherwise pool the violating sequence
        //and check if pooling caused monotonicity violation in previously processed points
        while (i >= 0 && !(input[i].label <= input[i + 1].label)) {
          pool(input, i, j);
          i = i - 1;
        }

        i = j;
      }
   }

   return input;
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

   if (size < 2) {
      //TODO SIMPLE PAVA
      fprintf(stderr,"Requires at least two processes.\n");
      exit(-1);
   }

   //Initialize MPI struct for LabeledPoint
   //const int nitems = 2;
   //int          blocklengths[2] = {1, 1};
   //MPI_Datatype types[2] = {MPI_DOUBLE, MPI_DOUBLE};
   //MPI_Datatype mpi_LabeledPoint_type;
   //MPI_Aint     offsets[2];

   //offsets[0] = offsetof(labeledPoint, label);
   //offsets[1] = offsetof(labeledPoint, feature);

   //MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_car_type);
   //MPI_Type_commit(&mpi_car_type);

   printf("name %s: hello world from process %d of %d\n", name, rank, size);

   struct LabeledPoint labels[20] = {{1, 1} , {2, 2}, {3, 3}, {3, 4}, {1, 5}, {6, 6}, {7, 7}, {8, 8}, {11, 9}, {9, 10}, {10, 11}, {12, 12}, {14, 13}, {15, 14}, {17, 15}, {16, 16}, {17, 17}, {18, 18}, {19, 19}, {20, 20}};
   
   printArray(labels, 20);
   struct LabeledPoint *newlabels;
   
   newlabels = pava(labels, 20);
   printArray(newlabels, 20);

   MPI_Finalize();

   exit(0);
}