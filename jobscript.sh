#!/bin/bash
#PBS -q qprod
#PBS -l select=1:ncpus=16:mpiprocs=16:ompthreads=1,walltime=01:45:00
#PBS -A IT4I-6-8

# change to scratch directory, exit on failure
SCRDIR=/scratch/$USER/parallel-isotonic-regression-mpi
cd $SCRDIR || exit

# load the mpi module
module load openmpi

# execute the calculation
mpiexec -bycore -bind-to-core ./a.out NASDAQ_REAL_10M.csv NASDAQ_REAL_10M_OUT.csv

#exit
exit