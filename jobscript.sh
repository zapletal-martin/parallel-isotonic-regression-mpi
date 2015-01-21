#!/bin/bash
#PBS -q qprod
#PBS -l select=8:ncpus=16:mpiprocs=16:ompthreads=1,walltime=00:30:00
#PBS -A IT4I-6-8

# change to scratch directory, exit on failure
SCRDIR=/scratch/$USER/parallel-isotonic-regression-mpi
cd $SCRDIR || exit

# load the mpi module
module load openmpi

# execute the calculation
mpiexec -bycore -bind-to-core ./a.out NASDAQ_REAL.csv NASDAQ_REAL_OUT.csv

#exit
exit