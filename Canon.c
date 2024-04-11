#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define MATRIX_SIZE 2000
#define PARTIAL_MATRIX_SIZE 500
#define MAIN_PROCESS_ID 0

// Dimensions of process matrix
static int processMatrixDimension = (MATRIX_SIZE / PARTIAL_MATRIX_SIZE);

// Number of MPI processes
static int processCount = (MATRIX_SIZE / PARTIAL_MATRIX_SIZE) * (MATRIX_SIZE / PARTIAL_MATRIX_SIZE);

// Main matrices
static float matrixA[MATRIX_SIZE][MATRIX_SIZE];
static float matrixB[MATRIX_SIZE][MATRIX_SIZE];
static float result[MATRIX_SIZE][MATRIX_SIZE];

// Local and received matrices
static float localMatrixA[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE], localMatrixB[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE], localMatrixC[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE];
static float receivedMatrixA[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE], receivedMatrixB[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE], receivedMatrixC[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE];

// Initial matrices to be sent to processes
static float initalMatrixA[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE], initalMatrixB[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE];
static float sequencerMatrix[MATRIX_SIZE][MATRIX_SIZE];

void readInitialMatrices()
{
    FILE *fileMatrixA = fopen("Matrix_A.txt", "r");
    FILE *fileMatrixB = fopen("Matrix_B.txt", "r");

    for (int row = 0; row < MATRIX_SIZE; row++)
    {
        for (int column = 0; column < MATRIX_SIZE; column++)
        {
            fscanf(fileMatrixA, "%f", &matrixA[row][column]);
            fscanf(fileMatrixB, "%f", &matrixB[row][column]);
        }
    }
    fclose(fileMatrixA);
    fclose(fileMatrixB);
}

void cut_right_part_of_matrix(int row_offset, int col_offset, float matrix[MATRIX_SIZE][MATRIX_SIZE], float result[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE])
{
    for (int row = 0; row < PARTIAL_MATRIX_SIZE; row++)
    {
        for (int col = 0; col < PARTIAL_MATRIX_SIZE; col++)
        {
            result[row][col] = matrix[row_offset + row][col_offset + col];
        }
    }
}

void sendMatricesToProcesses()
{
    for (int processId = 1; processId < processCount; processId++)
    {
        int row_part = processId / processMatrixDimension;
        int col_part = processId % processMatrixDimension;
        cut_right_part_of_matrix(row_part * PARTIAL_MATRIX_SIZE, ((col_part + row_part) % processMatrixDimension) * PARTIAL_MATRIX_SIZE, matrixA, initalMatrixA);
        MPI_Send(initalMatrixA, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, processId, processId, MPI_COMM_WORLD);
        cut_right_part_of_matrix(((row_part + col_part) % processMatrixDimension) * PARTIAL_MATRIX_SIZE, col_part * PARTIAL_MATRIX_SIZE, matrixB, initalMatrixB);
        MPI_Send(initalMatrixB, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, processId, processId * 2, MPI_COMM_WORLD);
    }
}

void setInitialLocalMatrices()
{
    cut_right_part_of_matrix(0, 0, matrixA, localMatrixA);
    cut_right_part_of_matrix(0, 0, matrixB, localMatrixB);
}

void receiveMatrices(int processId)
{
    MPI_Recv(localMatrixA, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, MAIN_PROCESS_ID, processId, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(localMatrixB, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, MAIN_PROCESS_ID, processId * 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void ikj_multiply()
{
    for (int i = 0; i < PARTIAL_MATRIX_SIZE; i++)
    {
        for (int k = 0; k < PARTIAL_MATRIX_SIZE; k++)
        {
            for (int j = 0; j < PARTIAL_MATRIX_SIZE; j++)
            {
                localMatrixC[i][j] += localMatrixA[i][k] * localMatrixB[k][j];
            }
        }
    }
}

void reassignCalculatedToLocal()
{
    for (int row = 0; row < PARTIAL_MATRIX_SIZE; row++)
    {
        for (int col = 0; col < PARTIAL_MATRIX_SIZE; col++)
        {
            localMatrixA[row][col] = receivedMatrixA[row][col];
            localMatrixB[row][col] = receivedMatrixA[row][col];
        }
    }
}

void multiplyIteratively(int processId, MPI_Comm rowCommunicator, MPI_Comm colCommunicator)
{
    MPI_Request req1[processMatrixDimension], req2[processMatrixDimension];
    MPI_Request reqS1[processMatrixDimension], reqS2[processMatrixDimension];

    int myRowId, myColId;
    for (int shiftNum = 0; shiftNum < processMatrixDimension; shiftNum++)
    {
        ikj_multiply();

        MPI_Comm_rank(rowCommunicator, &myRowId);
        MPI_Comm_rank(colCommunicator, &myColId);
        int leftNeighbor = (myRowId - 1 + processMatrixDimension) % processMatrixDimension;
        int rightNeighbor = (myRowId + 1) % processMatrixDimension;
        int topNeighbor = (myColId - 1 + processMatrixDimension) % processMatrixDimension;
        int bottomNeigbor = (myColId + 1) % processMatrixDimension;

        float bufsize = MATRIX_SIZE * MATRIX_SIZE / processMatrixDimension / processMatrixDimension;

        // Receive calculated matrices
        //printf("receiving from right %d in %d\n", rightNeighbor, processId);
        MPI_Irecv(receivedMatrixA, bufsize, MPI_FLOAT, rightNeighbor, 0, rowCommunicator, &req1[shiftNum]);
        //printf("receiving from bottom left %d in %d\n", bottomNeigbor, processId);
        MPI_Irecv(receivedMatrixB, bufsize, MPI_FLOAT, bottomNeigbor, 0, colCommunicator, &req2[shiftNum]);

        // Send calculated matrices
        //printf("sending to left %d from %d\n", leftNeighbor, processId);
        MPI_Isend(localMatrixA, bufsize, MPI_FLOAT, leftNeighbor, 0, rowCommunicator, &reqS1[shiftNum]);
        //printf("sending to top %d from %d\n", topNeighbor, processId);
        MPI_Isend(localMatrixB, bufsize, MPI_FLOAT, topNeighbor, 0, colCommunicator, &reqS2[shiftNum]);

        //printf("Process %d waiting for matrices\n", processId);
        // Wait for matrices
        MPI_Wait(&req1[shiftNum], MPI_STATUS_IGNORE);
        MPI_Wait(&req2[shiftNum], MPI_STATUS_IGNORE);
        MPI_Wait(&reqS1[shiftNum], MPI_STATUS_IGNORE);
        MPI_Wait(&reqS2[shiftNum], MPI_STATUS_IGNORE);
        //printf("Process %d finished waiting for matrices\n", processId);

        //printf("Process %d reassigning calculated to local\n", processId);
        reassignCalculatedToLocal();
        //printf("Process %d reassigned calculated to local\n", processId);
    }
    //printf("Process %d finished multiplying\n", processId);
}

void multiplyMatrices(int processId)
{
    int processMatrixRow = processId / processMatrixDimension;
    int processMatrixCol = processId % processMatrixDimension;

    MPI_Comm rowCommunicator;
    MPI_Comm colCommunicator;
    MPI_Comm_split(MPI_COMM_WORLD, processMatrixRow, processMatrixCol, &rowCommunicator);
    MPI_Comm_split(MPI_COMM_WORLD, processMatrixCol, processMatrixRow, &colCommunicator);

    multiplyIteratively(processId, rowCommunicator, colCommunicator);
}

void resetMatrixC()
{
    for (int i = 0; i < PARTIAL_MATRIX_SIZE; i++)
    {
        for (int j = 0; j < PARTIAL_MATRIX_SIZE; j++)
        {
            localMatrixC[i][j] = 0;
        }
    }
}

void saveToResult(int x, int y, float matrix[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE])
{
    for (int i = 0; i < PARTIAL_MATRIX_SIZE; i++)
    {
        for (int j = 0; j < PARTIAL_MATRIX_SIZE; j++)
        {
            result[x + i][y + j] = matrix[i][j];
        }
    }
}

void checkResultWithSequencer()
{
    FILE *sequencerFile = fopen("ResultFromSequencer.txt", "r");

    for (int row = 0; row < MATRIX_SIZE; row++)
    {
        for (int col = 0; col < MATRIX_SIZE; col++)
        {
            fscanf(sequencerFile, "%f", &sequencerMatrix[row][col]);
        }
    }
    // Sprawdzenie poprawności

    for (int row = 0; row < MATRIX_SIZE; row++)
    {
        for (int col = 0; col < MATRIX_SIZE; col++)
        {
            if (fabs(sequencerMatrix[row][col] - result[row][col]) >= 0.1 * sequencerMatrix[row][col])
            {
                printf("Wynik niepoprawny\n");
                return;
            }
        }
    }
    printf("Wynik poprawny!\n");
    fclose(sequencerFile);
}

void writeResultToFile()
{
    FILE *resultFile = fopen("ResultFromCanon.txt", "w");
    for (int row = 0; row < MATRIX_SIZE; row++)
    {
        for (int col = 0; col < MATRIX_SIZE; col++)
        {
            fprintf(resultFile, "%10.1f ", result[row][col]);
        }
        fprintf(resultFile, "\n");
    }
    fclose(resultFile);
}

int handleChildCommunication(int processId)
{
    resetMatrixC();
    receiveMatrices(processId);
    multiplyMatrices(processId);

    // Send calculated matrix C to main proc
    //printf("Process %d sending to main\n", processId);
    MPI_Send(localMatrixC, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, MAIN_PROCESS_ID, processId, MPI_COMM_WORLD);
    //printf("Process %d sent to main\n", processId);
    return 0;
}

int handleMainCommunication()
{
    resetMatrixC();

    readInitialMatrices();
    sendMatricesToProcesses();
    setInitialLocalMatrices();

    double startTimer;
    startTimer = MPI_Wtime();

    multiplyMatrices(MAIN_PROCESS_ID);

    int processMatrixRow, processMatrixCol;

    //printf("Process %d starts saving result\n", MAIN_PROCESS_ID);
    saveToResult(0, 0, localMatrixC);
    for (int processId = 1; processId < processCount; processId++)
    {

        //printf("Main process receving from %d\n", processId);
        processMatrixRow = processId / processMatrixDimension;
        processMatrixCol = processId % processMatrixDimension;
        MPI_Recv(receivedMatrixC, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, processId, processId, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //printf("Main process receving from %d\n", processId);
        //printf("Main process saving from %d\n", processId);
        saveToResult(processMatrixRow * PARTIAL_MATRIX_SIZE, processMatrixCol * PARTIAL_MATRIX_SIZE, receivedMatrixC);
        //printf("Main process saved from %d\n", processId);
    }

    double endTimer;
    endTimer = MPI_Wtime();

    printf("Czas obliczen = %f\n", endTimer - startTimer);

    checkResultWithSequencer();
}

int main(int argc, char *argv[])
{
    // Initialize MPI communication
    MPI_Init(0, 0);

    // Receive process id and count of all processes invoked
    int processId, processCountActual;
    MPI_Comm_rank(MPI_COMM_WORLD, &processId);
    MPI_Comm_size(MPI_COMM_WORLD, &processCountActual);

    // Check if process count is as expected
    if (processCountActual != processCount)
    {
        if (processId == 0)
        {
            printf("Your number of process is incorrect!\nPlease try again on %d processes\n\n", processMatrixDimension * processMatrixDimension);
        }
        MPI_Finalize();
        return -1;
    }

    int status = -1;
    if (processId == MAIN_PROCESS_ID)
    {
        status = handleMainCommunication();
    }
    else
    {
        status = handleChildCommunication(processId);
    }
    MPI_Finalize();
    return status;
}