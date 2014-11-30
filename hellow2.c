#include <stdio.h>  /* printf and BUFSIZ defined there */
#include <stdlib.h> /* exit defined there */
#include <limits.h>
#include <mpi.h>    /* all MPI-2 functions defined there */

#define MAX_PARTITION_SIZE 1048576

typedef struct LabeledPoint
{
   double label;
   double feature;
   double weight;
} LabeledPointT; 

void printArray(LabeledPointT *input, int length) {
   printf("\r\n-------------------------\r\n");
   
   int j = 0;
   for(j = 0; j < length; j++) {
     printf("%f ", input[j].label);
   }
   printf("\r\n-------------------------\r\n");
}

int sum(LabeledPointT *input, int start, int end) {
   int sum = 0;

   for(; start <= end; start++) {
      sum += input[start].label;
   }  

   return sum;
}

void pool(LabeledPointT *input, int start, int end) {
   double weightedSum = sum(input, start, end);
   double weight = end - start + 1;

   for(; start <= end; start++) {
      input[start].label = weightedSum / weight;
   }
}

LabeledPointT *pava(LabeledPointT *input, int length) {
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

MPI_Datatype MPI_Init_Type_LabeledPoint () {
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

  struct LabeledPoint _labeledPoint;

  int count; //Says how many kinds of data your structure has
  count = 3; 

  //Says the type of every block
  MPI_Datatype array_of_types[count];
  //You just have int
  array_of_types[0] = MPI_DOUBLE;
  array_of_types[1] = MPI_DOUBLE;
  array_of_types[2] = MPI_DOUBLE;

  //Says how many elements for block
  int array_of_blocklengths[count];
  //You have 8 int
  array_of_blocklengths[0] = 1;
  array_of_blocklengths[1] = 1;
  array_of_blocklengths[2] = 1;

  /*Says where every block starts in memory, counting from the beginning of the struct.*/
  MPI_Aint array_of_displaysments[count];
  MPI_Aint address1, address2, address3, address4;
  MPI_Get_address(&_labeledPoint, &address1);
  MPI_Get_address(&_labeledPoint.label, &address2);
  MPI_Get_address(&_labeledPoint.feature, &address3);
  MPI_Get_address(&_labeledPoint.weight, &address4);
  array_of_displaysments[0] = address2 - address1;
  array_of_displaysments[1] = address3 - address1;
  array_of_displaysments[2] = address4 - address1;
  //array_of_displaysments[0] = offsetof(_labeledPoint, label);
  //array_of_displaysments[1] = offsetof(_labeledPoint, feature);

  /*Create MPI Datatype and commit*/
  MPI_Datatype mpi_LabeledPoint_type;
  MPI_Type_create_struct(count, array_of_blocklengths, array_of_displaysments, array_of_types, &mpi_LabeledPoint_type);
  MPI_Type_commit(&mpi_LabeledPoint_type);

  return mpi_LabeledPoint_type;
  //Now we are ready to send
  //MPI_Send(&_info, 1, stat_type, dest, tag, comm),
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

   MPI_Datatype MPI_LabeledPoint = MPI_Init_Type_LabeledPoint();

   printf("name %s: hello world from process %d of %d\n", name, rank, size);

   if(rank == 0) {
      LabeledPointT labels[21] = {{1, 1, 1} , {2, 2, 1}, {3, 3, 1}, {3, 4, 1}, {1, 5, 1}, {6, 6, 1}, {7, 7, 1}, {8, 8, 1}, {11, 9, 1}, {9, 10, 1}, {10, 11, 1}, {12, 12, 1}, {14, 13, 1}, {15, 14, 1}, {17, 15, 1}, {16, 16, 1}, {17, 17, 1}, {18, 18, 1}, {19, 19, 1}, {20, 20, 1}, {21, 21, 1}};

      int destination;
      int partition = 0;
      
      //distribute to workers
      for(destination = 1; destination < size; destination++) {
        MPI_Send(labels + partition, 7, MPI_LabeledPoint, destination, 1, MPI_COMM_WORLD);
        partition += 7;
      }
      
      MPI_Status status;
      LabeledPointT result[21];
      int count;
      partition = 0;
      for(destination = 1; destination < size; destination++) {
         MPI_Recv(result + partition, MAX_PARTITION_SIZE, MPI_LabeledPoint, destination, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
         MPI_Get_count(&status, MPI_LabeledPoint, &count);
         //printf("RECEIVED COUNT %d\n", count);
         partition += count;
      }

      LabeledPointT *finalResult = pava(result, 21);

      printArray(finalResult, 21);

   } else {
      MPI_Status status;
      int count;
      //LabeledPointT buffer[MAX_PARTITION_SIZE];

      //TODO: LEAKING
      LabeledPointT *buffer = malloc(MAX_PARTITION_SIZE * sizeof(LabeledPointT));

      MPI_Recv(buffer, MAX_PARTITION_SIZE, MPI_LabeledPoint, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_LabeledPoint, &count);

      LabeledPointT *result = pava(buffer, count);

      MPI_Send(result, count, MPI_LabeledPoint, 0, 1, MPI_COMM_WORLD);
   }

   MPI_Type_free(&MPI_LabeledPoint);
   MPI_Finalize();

   exit(0);
}