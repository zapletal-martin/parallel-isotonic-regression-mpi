#include <stdio.h>  /* printf and BUFSIZ defined there */
#include <stdlib.h> /* exit defined there */
#include <limits.h>
#include <string.h>
#include <mpi.h>    /* all MPI-2 functions defined there */

int main2(argc, argv)
int argc;
char *argv[];
{
  FILE *fOut;
  fOut = fopen("data2.csv", "w");
  char line[128];
  long i;

  for (i = 9999L; i > 0; i--) {
    snprintf(line, 128, "%lu,%lu,%lu\r\n", i, i, 1);    
    fputs(line, fOut);
  }

  fclose(fOut);
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

int main(argc, argv)
int argc;
char *argv[];
{
  srand(1);
  FILE *fOut;
  FILE *fIn;

  fOut = fopen("NASDAQ_OUT.csv", "w");

  char line[128];
  double i = 1;

  double val;
  long counter = 0;

  for (i = 1; i < 350; i += 0.1) {
  	fIn = fopen("NASDAQ.csv", "r");

  	while (fgets(line, sizeof line, fIn) != NULL && counter <= 10000000L) {
      sscanf(line, "%lf", &val);

      snprintf(line, 128, "%lf,%lf,%lf\r\n", val * i, 1.0, 1.0);    
      fputs(line, fOut);

      counter++;
  	}

  	fclose(fIn);
  }

  fclose(fOut);
}