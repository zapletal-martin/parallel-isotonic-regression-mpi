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

typedef int bool;
#define true 1
#define false 0

void printArray(LabeledPointT *input, int length) {
   printf("\r\n-------------------------\r\n");
   
   int j = 0;
   for(j = 0; j < length; j++) {
     printf("%f ", input[j].label);
   }
   printf("\r\n-------------------------\r\n");
}

double weightedLabelSum(LabeledPointT *input, int start, int end) {
   double sum = 0;

   for(; start <= end; start++) {
      sum += input[start].label * input[start].weight;
   }  

   return sum;
}

double weightSum(LabeledPointT *input, int start, int end) {
  double sum = 0;

   for(; start <= end; start++) {
      sum += input[start].weight;
   }  

   return sum; 
}

void pool(LabeledPointT *input, int start, int end) {
   double weightedSum = weightedLabelSum(input, start, end);
   double weight = weightSum(input, start, end);

   for(; start <= end; start++) {
      input[start].label = weightedSum / weight;
   }
}

bool poolAdjacentViolators(LabeledPointT *input, int length) {
   int i = 0;
   bool pooled = false;

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
          pooled = true;
          pool(input, i, j);
          i = i - 1;
        }

        i = j;
      }
   }

   return pooled;
}

MPI_Datatype MPI_Init_Type_LabeledPoint () {
  struct LabeledPoint _labeledPoint;

  int count; 
  count = 3; 

  MPI_Datatype array_of_types[count];

  array_of_types[0] = MPI_DOUBLE;
  array_of_types[1] = MPI_DOUBLE;
  array_of_types[2] = MPI_DOUBLE;

  int array_of_blocklengths[count];
  array_of_blocklengths[0] = 1;
  array_of_blocklengths[1] = 1;
  array_of_blocklengths[2] = 1;

  MPI_Aint array_of_displaysments[count];
  MPI_Aint address1, address2, address3, address4;
  MPI_Get_address(&_labeledPoint, &address1);
  MPI_Get_address(&_labeledPoint.label, &address2);
  MPI_Get_address(&_labeledPoint.feature, &address3);
  MPI_Get_address(&_labeledPoint.weight, &address4);
  array_of_displaysments[0] = address2 - address1;
  array_of_displaysments[1] = address3 - address1;
  array_of_displaysments[2] = address4 - address1;

  MPI_Datatype mpi_LabeledPoint_type;
  MPI_Type_create_struct(count, array_of_blocklengths, array_of_displaysments, array_of_types, &mpi_LabeledPoint_type);
  MPI_Type_commit(&mpi_LabeledPoint_type);

  return mpi_LabeledPoint_type;
}

int isMaster(int rank) {
  return rank == 0;
} 

int availablePartitions(int numberOfProcesses) {
  return numberOfProcesses - 1;
}

void masterSend(MPI_Datatype MPI_LabeledPoint, int numberOfPartitions, LabeledPointT *labels, int partitionSize) {
  int destination;
  int partition = 0;
  
  //distribute to workers
  for(destination = 1; destination <= numberOfPartitions; destination++) {
    MPI_Send(labels + partition, partitionSize, MPI_LabeledPoint, destination, 1, MPI_COMM_WORLD);
    partition += partitionSize;
  }
}

bool masterReceive(MPI_Datatype MPI_LabeledPoint, int numberOfPartitions, LabeledPointT *labels, int partitionSize) {
  MPI_Status status;
  int count;
  int destination;
  int partition = 0;
  bool p = false;
  bool *pooled = &p;
  bool pooledReturn = false;

  for(destination = 1; destination <= numberOfPartitions; destination++) {
    MPI_Recv(labels + partition, MAX_PARTITION_SIZE, MPI_LabeledPoint, destination, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_LabeledPoint, &count);

    MPI_Recv(pooled, 1, MPI_INT, destination, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    if(*pooled == true) {
      pooledReturn = true;
    }
    
    partition += count;
  }
  
  return pooledReturn;
}

void master(MPI_Datatype MPI_LabeledPoint, int numberOfProcesses, LabeledPointT *labels, int labelsSize) {
  int partitionSize = labelsSize / availablePartitions(numberOfProcesses);

  masterSend(MPI_LabeledPoint, availablePartitions(numberOfProcesses), labels, partitionSize);
  masterReceive(MPI_LabeledPoint, availablePartitions(numberOfProcesses), labels, partitionSize);

  poolAdjacentViolators(labels, labelsSize);
  printArray(labels, labelsSize);
}

void iterativeMaster(MPI_Datatype MPI_LabeledPoint, int numberOfProcesses, LabeledPointT *labels, int labelsSize) {
  bool pooled = true; 

  int partitionSize = labelsSize / availablePartitions(numberOfProcesses);

  while(pooled == true) {
    masterSend(MPI_LabeledPoint, availablePartitions(numberOfProcesses), labels, partitionSize);
    pooled = masterReceive(MPI_LabeledPoint, availablePartitions(numberOfProcesses), labels, partitionSize);
  }

  printArray(labels, labelsSize);
}

void partition(MPI_Datatype MPI_LabeledPoint) {
  MPI_Status status;
  int count;

  LabeledPointT *buffer = malloc(MAX_PARTITION_SIZE * sizeof(LabeledPointT));

  MPI_Recv(buffer, MAX_PARTITION_SIZE, MPI_LabeledPoint, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  MPI_Get_count(&status, MPI_LabeledPoint, &count);

  bool pooled = poolAdjacentViolators(buffer, count);

  MPI_Send(buffer, count, MPI_LabeledPoint, 0, 1, MPI_COMM_WORLD);
  MPI_Send(&pooled, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
}

void iteraivePartition(MPI_Datatype MPI_LabeledPoint) {
  while(true) {
    partition(MPI_LabeledPoint);
  }
}

void iterativeParallelPoolAdjacentViolators(MPI_Datatype MPI_LabeledPoint, int numberOfProcesses, int rank) {
  LabeledPointT labels[21] = {{1, 1, 1} , {2, 2, 1}, {3, 3, 1}, {3, 4, 1}, {1, 5, 1}, {6, 6, 1}, {7, 7, 1}, {8, 8, 1}, {11, 9, 1}, {9, 10, 1}, {10, 11, 1}, {12, 12, 1}, {14, 13, 1}, {15, 14, 1}, {17, 15, 1}, {16, 16, 1}, {17, 17, 1}, {18, 18, 1}, {19, 19, 1}, {20, 20, 1}, {21, 21, 1}};

  if(isMaster(rank)) {
    iterativeMaster(MPI_LabeledPoint, numberOfProcesses, labels, 21);
    MPI_Abort(MPI_COMM_WORLD, 1);
  } else {
    iteraivePartition(MPI_LabeledPoint);
  }
}

void parallelPoolAdjacentViolators(MPI_Datatype MPI_LabeledPoint, int numberOfProcesses, int rank) {
  LabeledPointT labels[21] = {{1, 1, 1} , {2, 2, 1}, {3, 3, 1}, {3, 4, 1}, {1, 5, 1}, {6, 6, 1}, {7, 7, 1}, {8, 8, 1}, {11, 9, 1}, {9, 10, 1}, {10, 11, 1}, {12, 12, 1}, {14, 13, 1}, {15, 14, 1}, {17, 15, 1}, {16, 16, 1}, {17, 17, 1}, {18, 18, 1}, {19, 19, 1}, {20, 20, 1}, {21, 21, 1}};  

  if(isMaster(rank)) {
    master(MPI_LabeledPoint, numberOfProcesses, labels, 21);
  } else {
    partition(MPI_LabeledPoint);
  }
}

int main(argc, argv)
int argc;
char *argv[];
{
   int rank, numberOfProcesses, length;
   char name[BUFSIZ];

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcesses);
   MPI_Get_processor_name(name, &length);

   if (numberOfProcesses < 2) {
      //TODO SIMPLE PAVA
      fprintf(stderr,"Requires at least two processes.\n");
      exit(-1);
   }

   MPI_Datatype MPI_LabeledPoint = MPI_Init_Type_LabeledPoint();
   iterativeParallelPoolAdjacentViolators(MPI_LabeledPoint, numberOfProcesses, rank);

   printf("name %s: hello world from process %d of %d\n", name, rank, numberOfProcesses);

   MPI_Type_free(&MPI_LabeledPoint);
   MPI_Finalize();

   exit(0);
}