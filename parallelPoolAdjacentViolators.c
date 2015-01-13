#include <stdio.h>  /* printf and BUFSIZ defined there */
#include <stdlib.h> /* exit defined there */
#include <limits.h>
#include <string.h>
#include <math.h>
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

bool lessEqualTo(double a, double b) {
    return a < b || fabs(a - b) < 0.001;
}

bool poolAdjacentViolators(LabeledPointT *input, int length) {
   int i = 0;
   bool pooled = false;

   while(i < length) {
      int j = i;

      //find monotonicity violating sequence, if any
      while(j < length - 1 && !(lessEqualTo(input[j].label, input[j + 1].label))) {
        j = j + 1;
      }

      //if monotonicity was not violated, move to next data point
      if(i == j) {
        i = i + 1;
      } else {
        //otherwise pool the violating sequence
        //and check if pooling caused monotonicity violation in previously processed points
        while (i >= 0 && !(lessEqualTo(input[i].label, input[i + 1].label))) {
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

long countLines(char *fileName) {
  FILE *ptr_readfile;
  char line [128]; /* or some other suitable maximum line size */
  long lines = 0;
  char ch;

  ptr_readfile = fopen(fileName,"r");

  while (fgets(line, sizeof line, ptr_readfile) != NULL) {
    ++lines;
  }

  return lines;
}

LabeledPointT parseLine(char *line) {
  char* token;
  LabeledPointT label;

  token = strtok(line, " ,\t\n");
  sscanf(token, "%lf", &label.label);

  token = strtok(NULL, " ,\t\n");
  sscanf(token, "%lf", &label.feature);

  token = strtok(NULL, " ,\t\n");
  sscanf(token, "%lf", &label.weight);

  //label = (LabeledPointT *)malloc(1 * sizeof(LabeledPointT));

  return label;
}

LabeledPointT *readFile(char *fileName) {
  FILE *ptr_readfile;
  char line [128]; /* or some other suitable maximum line size */
  LabeledPointT *labels = malloc(countLines(fileName) * sizeof(LabeledPointT));
  int i = 0;

  ptr_readfile = fopen(fileName,"r");

  while (fgets(line, sizeof line, ptr_readfile) != NULL) {
    labels[i] = parseLine(line);
    i++;
  }

  fclose(ptr_readfile);
  return labels;
}

void writeFile(char *fileName, LabeledPointT *labels, int size) {
  FILE *fOut;
  fOut = fopen(fileName, "w");
  char line[128];
  int i ;

  for (i = 0; i < size; i++) {
    LabeledPointT label = labels[i];
    snprintf(line, 128, "%f,%f,%f\r\n", label.label, label.feature, label.weight);    
    fputs(line, fOut);
  }

  fclose(fOut);
}

void masterSend(MPI_Datatype MPI_LabeledPoint, int numberOfPartitions, LabeledPointT *labels, long partitionSize, bool terminate) {
  int destination;
  int partition = 0;
  //distribute to workers 
  for(destination = 1; destination <= numberOfPartitions; destination++) {

    MPI_Send(&terminate, 1, MPI_INT, destination, 1, MPI_COMM_WORLD);

    if(terminate == false) {
      MPI_Send(&partitionSize, 1, MPI_LONG, destination, 1, MPI_COMM_WORLD);
      MPI_Send(labels + partition, partitionSize, MPI_LabeledPoint, destination, 1, MPI_COMM_WORLD);
      partition += partitionSize;
    }
  }
}

bool masterReceive(MPI_Datatype MPI_LabeledPoint, int numberOfPartitions, LabeledPointT *labels, long partitionSize) {
  MPI_Status status;
  int count;
  int destination;
  int partition = 0;
  bool p = false;
  bool *pooled = &p;
  bool pooledReturn = false;

  for(destination = 1; destination <= numberOfPartitions; destination++) {

printf("RECEIVING\n");
    MPI_Recv(labels + partition, partitionSize, MPI_LabeledPoint, destination, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_LabeledPoint, &count);

    MPI_Recv(pooled, 1, MPI_INT, destination, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    if(*pooled == true) {
      pooledReturn = true;
    }
    
    partition += count;
  }
  
  return pooledReturn;
}

void master(MPI_Datatype MPI_LabeledPoint, int numberOfProcesses, char* inputFileName, char* outputFileName) {
  LabeledPointT *labels = readFile(inputFileName);
  long labelsSize = countLines(inputFileName);

  long partitionSize = labelsSize / availablePartitions(numberOfProcesses);

  masterSend(MPI_LabeledPoint, availablePartitions(numberOfProcesses), labels, partitionSize, false);
  masterReceive(MPI_LabeledPoint, availablePartitions(numberOfProcesses), labels, partitionSize);

  poolAdjacentViolators(labels, labelsSize);
  
  writeFile(outputFileName, labels, countLines(inputFileName));  
}

void iterativeMaster(MPI_Datatype MPI_LabeledPoint, int numberOfProcesses, char* inputFileName, char* outputFileName) {
  LabeledPointT *labels = readFile(inputFileName);
  long labelsSize = countLines(inputFileName);
  bool pooled = true; 
  int i = 0;

  int numberOfPartitions = availablePartitions(numberOfProcesses);
  long partitionSize = labelsSize / numberOfPartitions;

  LabeledPointT *iterator = labels;

  while(pooled == true) {
    masterSend(MPI_LabeledPoint, numberOfPartitions, iterator, partitionSize, false);
    pooled = masterReceive(MPI_LabeledPoint, numberOfPartitions, iterator, partitionSize);

    if(iterator == labels) {
      iterator += partitionSize / 2;
      numberOfPartitions -= 1;
    } else {
      iterator -= partitionSize / 2;
      numberOfPartitions += 1;
    } 

    i++;
  }

  writeFile(outputFileName, labels, countLines(inputFileName));
  masterSend(MPI_LabeledPoint, numberOfPartitions, iterator, partitionSize, true);
  //MPI_Abort(MPI_COMM_WORLD, 1);
}

void partition(MPI_Datatype MPI_LabeledPoint) {
  MPI_Status status;
  long count;
  int terminate = false;

  MPI_Recv(&terminate, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

  if(terminate == true) {
    MPI_Type_free(&MPI_LabeledPoint);
    MPI_Finalize();
  } else {
    MPI_Recv(&count, 1, MPI_LONG, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    LabeledPointT *buffer = malloc(count * sizeof(LabeledPointT));

    MPI_Recv(buffer, count, MPI_LabeledPoint, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //MPI_Get_count(&status, MPI_LabeledPoint, &count);

    bool pooled = poolAdjacentViolators(buffer, count);

    MPI_Send(buffer, count, MPI_LabeledPoint, 0, 1, MPI_COMM_WORLD);
    MPI_Send(&pooled, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
  }
}

void iteraivePartition(MPI_Datatype MPI_LabeledPoint) {
  while(true) {
    partition(MPI_LabeledPoint);
  }
}

void iterativeParallelPoolAdjacentViolators(MPI_Datatype MPI_LabeledPoint, int numberOfProcesses, int rank, char *inputFileName, char *outputFileName) {
  if(isMaster(rank)) {
    iterativeMaster(MPI_LabeledPoint, numberOfProcesses, inputFileName, outputFileName);
  } else {
    iteraivePartition(MPI_LabeledPoint);
  }
}

void parallelPoolAdjacentViolators(MPI_Datatype MPI_LabeledPoint, int numberOfProcesses, int rank, char *inputFileName, char *outputFileName) {
  if(isMaster(rank)) {
    master(MPI_LabeledPoint, numberOfProcesses, inputFileName, outputFileName);
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

   if (argc < 1) {
      fprintf(stderr,"Requires filename argument.\n");
      exit(-1);
   }

   if (numberOfProcesses < 2) {
      //TODO SIMPLE PAVA
      fprintf(stderr,"Requires at least two processes.\n");
      exit(-1);
   }

   //LabeledPointT labels[21] = {{1, 1, 1} , {2, 2, 1}, {3, 3, 1}, {3, 4, 1}, {1, 5, 1}, {6, 6, 1}, {7, 7, 1}, {8, 8, 1}, {11, 9, 1}, {9, 10, 1}, {10, 11, 1}, {12, 12, 1}, {14, 13, 1}, {15, 14, 1}, {17, 15, 1}, {16, 16, 1}, {17, 17, 1}, {18, 18, 1}, {19, 19, 1}, {20, 20, 1}, {21, 21, 1}};
   MPI_Datatype MPI_LabeledPoint = MPI_Init_Type_LabeledPoint();

   iterativeParallelPoolAdjacentViolators(MPI_LabeledPoint, numberOfProcesses, rank, argv[1], argv[2]);

   MPI_Type_free(&MPI_LabeledPoint);
   MPI_Finalize();

   exit(0);
}