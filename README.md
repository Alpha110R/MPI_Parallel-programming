# MPI_Parallel-programming
In the PDF there ara the instructions for this exercise.

In the main there is 3 functions:
- 1)Sequential
- 2)Static task pool
- 3)Dynamic task pool

You need to uncomment the function that you want to execute and run it in the terminal.

To compile the program (Linux):
- Enter to the program directory in the teminal
- mpicc program.c -lm -o program
- mpiexec -n (number of processes) program
