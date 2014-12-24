#include <stdio.h>  /* printf and BUFSIZ defined there */
#include <stdlib.h> /* exit defined there */
#include <limits.h>
#include <string.h>
#include <mpi.h>    /* all MPI-2 functions defined there */

int main(argc, argv)
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