#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#define HEAVY 1000
#define SIZE 40
#define RADIUS 10
#define FILE_NAME "points.txt"
#define MASTER 0
enum ranks{ROOT,N=400}; //N = number of points per slave
// This function simulates heavy computations, 
// its run time depends on x and y values
// DO NOT change this function!!

/* types of messages:
   ** with tag WORK: sent by the master to a worker with N
   numbers (worker needs to find their maximum)
   ** with tag STOP: sent by the master to a worker.
      This message says that there is no more work.
   ** with tag LOCAL_MAX: sent by the worker to the master.
      The message contains the maximum found by the worker
	  (maximum of the last N numbers received by the worker)
*/
enum tags {WORK, STOP, SLAVE_ANSWER_HEAVY};
void dynamicTaskPool(int argc, char *argv[]);
void masterProcess(int num_procs, int* points, int numberOfPoints);
void workerProcess();
void staticTaskPool(int argc, char *argv[]);
double bcastSizeToSlave(int* points, int amountOfValuesPerProcces);
double calcSumHeavy(int* points, int amountOfValuesToCalc, int* work_arrStatic);
void sequentialSolution(int argc, char *argv[]);


double heavy(int x, int y) {
    int i, loop;
    double sum = 0;
    if (sqrt((x - 0.25 * SIZE) * (x - 0.25 * SIZE) + (y - 0.75 * SIZE) * (y - 0.75 * SIZE)) < RADIUS)
        loop = 5 * x * y;
    else
        loop = abs(x-y) + x;
    for (i = 0; i < loop * HEAVY; i++)
        sum += sin(exp(cos((double)i / HEAVY)))/HEAVY;
    return sum;
}

double calcHeavy(int* points, int numberOfPoints){
    double answer=0;
    for (int i = 0; i < numberOfPoints; i++){
        answer += heavy(points[2 * i], points[2 * i + 1]);
    }
    return answer;
}
// Reads a number of points from the file.
// The first line contains a number of points defined.
// Following lines contain two integers each - point coordinates x, y
int *readFromFile(const char *fileName, int *numberOfPoints) {
    FILE* fp;
    int* points;
// Open file for reading points
    if ((fp = fopen(fileName, "r")) == 0) {
        printf("cannot open file %s for reading\n", fileName);
        exit(0);
    }
// Number of points
    fscanf(fp, "%d", numberOfPoints);
// Allocate array of points end Read data from the file
    points = (int*)malloc(2 * *numberOfPoints * sizeof(int));
    if (points == NULL) {
        printf("Problem to allocate memotry\n");

        exit(0);
    }
    for (int i = 0; i < *numberOfPoints; i++) {
        fscanf(fp, "%d %d", &points[2*i], &points[2*i + 1]);
    }
    fclose(fp);
    return points;
}

int main(int argc, char* argv[]) {
    /*Uncomment the function you want to run*/
///////////////////////////////SEQUENTIAL///////////////////////////////
    //sequentialSolution(argc, argv);

///////////////////////////////STATIC///////////////////////////////
    staticTaskPool(argc, argv);
        
///////////////////////////////DYNAMIC///////////////////////////////
    //dynamicTaskPool(argc, argv);
    
    
    MPI_Finalize();
    return 0;
}



void sequentialSolution(int argc, char *argv[]){
    /*I wrote the MPI function only for the convenience in the terminal*/
    double timeExecution,
           answer;
    int* points;
    int numberOfPoints,
        my_rank,
        num_procs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    points = readFromFile(FILE_NAME, &numberOfPoints);
    timeExecution = MPI_Wtime();
    answer = calcHeavy(points, numberOfPoints);
    printf("Sequensial answer = %e \n", answer);
    printf("EXEC Sequensial time: %lf \n", MPI_Wtime() - timeExecution);
}

double calcSumHeavy(int* points, int amountOfValuesToCalc, int* work_arrStatic){
    double slaveAnswer,
           masterAnswer;
    MPI_Scatter(points, amountOfValuesToCalc, MPI_INT, work_arrStatic,amountOfValuesToCalc,MPI_INT,ROOT,MPI_COMM_WORLD);
    slaveAnswer = calcHeavy(work_arrStatic, amountOfValuesToCalc/2);
    MPI_Reduce(&slaveAnswer,&masterAnswer,1,MPI_DOUBLE,MPI_SUM,ROOT,MPI_COMM_WORLD);
    return masterAnswer;
}

double bcastSizeToSlave(int* points, int amountOfValuesPerProcces){
    int amountOfValuesToCalc = amountOfValuesPerProcces;
    MPI_Bcast(&amountOfValuesToCalc , 1 , MPI_INT , ROOT , MPI_COMM_WORLD);
    int* work_arrStatic;//Array to receive the points
    double masterAnswer;
    work_arrStatic = (int*)malloc(amountOfValuesToCalc * sizeof(int));
    if (work_arrStatic == NULL) {
        printf("Problem to allocate memory\n");

        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    return calcSumHeavy(points, amountOfValuesToCalc, work_arrStatic);
}


void staticTaskPool(int argc, char *argv[]){
    int* points;
    int* work_arrStatic;
    int my_rank,
        num_procs,
        numberOfPoints,
        amountOfValuesToCalc;
    double timeExecution,
           masterAnswer;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if(my_rank == ROOT){
        timeExecution = MPI_Wtime();
        points = readFromFile(FILE_NAME, &numberOfPoints);
        amountOfValuesToCalc = (numberOfPoints / num_procs) *2;//Calculate how much points every procces get
    }
    masterAnswer = bcastSizeToSlave(points, amountOfValuesToCalc);
    if(my_rank == ROOT){
        printf("The STATIC sum for the heavy: %e \n", masterAnswer);
        printf("EXEC STATIC time: %lf\n", MPI_Wtime() - timeExecution);
    }
}

void workerProcess(){
    int* arr, tag;
    MPI_Status status;
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    arr = (int*)malloc(N * sizeof(int));
    if (arr == NULL) {
        printf("Problem to allocate memory\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    do
    {
        MPI_Recv(arr,N,MPI_INT,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
        //Tag is the enum that the master sent to the slave to WORK or STOP
        //It came from the status of the message
        tag = status.MPI_TAG;
		if (tag == WORK) {								    
            double answer = calcHeavy(arr, N/2);
            MPI_Send(&answer,1,MPI_DOUBLE,ROOT,
			         SLAVE_ANSWER_HEAVY,MPI_COMM_WORLD);
		} 
    } while (tag != STOP);
}

void masterProcess(int num_procs, int* points, int numberOfPoints){
    MPI_Status status;
	double timeExecution = MPI_Wtime();
    int arrsize = numberOfPoints*2;//The arr size is twice the amount of points, every point has to values
	//Start workers
	int jobs_sent = 0;
    double totalSum=0;
    //The initialization for slaves. Sends the firsts values
    for(int worker_id =1;worker_id<num_procs; worker_id++){
        MPI_Send(points+jobs_sent*N,N,MPI_INT,worker_id,WORK,MPI_COMM_WORLD);
        jobs_sent++;
    }
	int jobs_total = arrsize/N;
    for(int jobs_done=0;jobs_done < jobs_total; jobs_done++)
    {
        double answerSlave;
        MPI_Recv(&answerSlave, 1, MPI_DOUBLE, MPI_ANY_SOURCE,
		         SLAVE_ANSWER_HEAVY, MPI_COMM_WORLD, &status);
		totalSum+= answerSlave;
				
		int jobs_left = jobs_total-jobs_sent;
        //Checks if remians jobs to send to the slaves
		if (jobs_left > 0) {
            MPI_Send(points+jobs_sent*N,N,MPI_INT,status.MPI_SOURCE,WORK,MPI_COMM_WORLD);
			jobs_sent++;
		}
		else {
			/* Send STOP message. message has no data */
			int stopProcces;
			MPI_Send(&stopProcces,0, MPI_INT, status.MPI_SOURCE,
			         STOP, MPI_COMM_WORLD);
        }
    }

    printf("Parallel DYNAMIC sum is %e\n", totalSum);
    printf("EXEC DYNAMIC time: %lf\n", MPI_Wtime() - timeExecution);

}

void dynamicTaskPool(int argc, char *argv[]){
    int *points;
    int my_rank,
        num_procs,
        numberOfPoints;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if(my_rank == ROOT){
        points = readFromFile(FILE_NAME, &numberOfPoints);
        masterProcess(num_procs, points, numberOfPoints);
    }else{
        workerProcess();
    }
}







