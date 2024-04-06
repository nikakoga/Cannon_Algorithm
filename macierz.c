#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// gcc macierz.c -o executable
const int sizeOfMainMatrix = 2;
const int sizeOfMiniMatrix = 3; // it have to bo a square. So 3 means 3 rows and 3 columns = 9 cells inside.
const int placeForOneNumber = 3;

void readMyPartOfMatrixFromFile(const char *fileName, const int procesID, int localMatrix[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    FILE *fileMatrix = fopen(fileName, "r");

    if (fileMatrix == NULL)
    {
        printf("Error while file opening\n");
        MPI_Finalize();
        return;
    }

    int rowsPerProcess = sizeOfMiniMatrix;
    int columnsPerProcess = sizeOfMiniMatrix * placeForOneNumber;
    int startRow = (procesID / sizeOfMainMatrix) * rowsPerProcess; // it is an int so for ex 4/3=1, 2/3=0 so process 4 start in 1st row and proces 2 start in row 0
    int startColumn = (procesID % sizeOfMainMatrix) * (sizeOfMiniMatrix * placeForOneNumber);

    fseek(fileMatrix, startRow * (sizeOfMainMatrix * sizeOfMiniMatrix * placeForOneNumber + 1), SEEK_SET); // +1 bo znak konca nowej linii
    fseek(fileMatrix, startColumn, SEEK_CUR);

    for (int i = 0; i < rowsPerProcess; i++)
    {
        for (int j = 0; j < sizeOfMiniMatrix; j++)
        {
            fscanf(fileMatrix, "%d", &localMatrix[i][j]);
        }
        fseek(fileMatrix, (sizeOfMainMatrix * sizeOfMiniMatrix * placeForOneNumber + 1 - (sizeOfMiniMatrix * placeForOneNumber)), SEEK_CUR);
    }
    fclose(fileMatrix);
    // ToDo zbadac czy dodanie fclose cos zmieni dla innych procesow czytajacych z pliku
}

void readBothMatrixFromFile(const char *fileNameA, const char *fileNameB, int matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], int matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{
    FILE *fileMatrixA = fopen(fileNameA, "r");
    FILE *fileMatrixB = fopen(fileNameB, "r");

    if (fileNameA == NULL || fileNameB == NULL)
    {
        printf("Blad otwarcia pliku\n");
        MPI_Finalize();
        return;
    }
    for (int row = 0; row < (sizeOfMainMatrix * sizeOfMiniMatrix); row++)
    {
        for (int column = 0; column < (sizeOfMainMatrix * sizeOfMiniMatrix); column++)
        {
            fscanf(fileMatrixA, "%d", &matrixA[row][column]);
            fscanf(fileMatrixB, "%d", &matrixB[row][column]);
        }
    }
    fclose(fileMatrixA);
    fclose(fileMatrixB);
}

void splitAndSendMatrixForProcesses(int processNumber, int matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], int matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{

    int rowsPerProcess = sizeOfMiniMatrix;
    int columnsPerProcess = sizeOfMiniMatrix * placeForOneNumber;

    // Przygotowanie bufora do wysłania
    int sendBuffer_A[sizeOfMiniMatrix * sizeOfMiniMatrix];
    int sendBuffer_B[sizeOfMiniMatrix * sizeOfMiniMatrix];

    MPI_Request reqSendA, reqRecvA;
    MPI_Request reqSendB, reqRecvB;

    for (int processID = 0; processID < processNumber; processID++)
    {
        // Obliczenie indeksów początkowych wierszy dla danego procesu
        int startRow = (processID / sizeOfMainMatrix) * rowsPerProcess; // it is an int so for ex 4/3=1, 2/3=0 so process 4 start in 1st row and proces 2 start in row 0
        int startColumn = (processID % sizeOfMainMatrix) * sizeOfMiniMatrix;

        int index = 0;

        // Wypełnienie bufora danymi z macierzy A i B dla danego procesu
        for (int row = startRow; row < startRow + rowsPerProcess; row++)
        {
            for (int i = startColumn; i < startColumn + sizeOfMiniMatrix; i++)
            {
                sendBuffer_A[index] = matrixA[row][i];
                sendBuffer_B[index] = matrixB[row][i];
                index++;
            }
        }

        // Wysłanie danych do odpowiedniego procesu
        MPI_Isend(sendBuffer_A, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, processID, 0, MPI_COMM_WORLD, &reqSendA);
        MPI_Isend(sendBuffer_B, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, processID, 1, MPI_COMM_WORLD, &reqSendB);
    }
}

void saveLocalMatrixToFile(const char *fileName, int localMatrix[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    FILE *fileMatrix = fopen(fileName, "w");

    if (fileMatrix == NULL)
    {
        printf("Error while file opening\n");
        return;
    }

    for (int row = 0; row < sizeOfMiniMatrix; row++)
    {
        for (int column = 0; column < sizeOfMiniMatrix; column++)
        {
            fprintf(fileMatrix, "%*d", placeForOneNumber, localMatrix[row][column]);
        }
        fprintf(fileMatrix, "\n");
    }
    fclose(fileMatrix);
}

void save(const char *fileName, int localMatrix[sizeOfMiniMatrix * sizeOfMainMatrix][sizeOfMiniMatrix * sizeOfMainMatrix])
{
    FILE *fileMatrix = fopen(fileName, "w");

    if (fileMatrix == NULL)
    {
        printf("Error while file opening\n");
        return;
    }

    for (int row = 0; row < (sizeOfMiniMatrix * sizeOfMainMatrix); row++)
    {
        for (int column = 0; column < (sizeOfMiniMatrix * sizeOfMainMatrix); column++)
        {
            fprintf(fileMatrix, "%*d", placeForOneNumber, localMatrix[row][column]);
        }
        fprintf(fileMatrix, "\n");
    }
    fclose(fileMatrix);
}

void prepareResultMatrix(int result[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], int size)
{
    memset(result, 0, size * size * sizeof(int));
}

void multiplyMatrices(int matrixA[sizeOfMiniMatrix][sizeOfMiniMatrix], int matrixB[sizeOfMiniMatrix][sizeOfMiniMatrix], int matrixC[sizeOfMiniMatrix][sizeOfMiniMatrix]) {
    
    // schemat ikj
    for (int i = 0; i < sizeOfMiniMatrix; i++) {
        for (int j = 0; j < sizeOfMiniMatrix; j++) {
            matrixC[i][j] = 0; //zerowanie tej macierzy bo tam mogą być śmieci z pamięci
            for (int k = 0; k < sizeOfMiniMatrix; k++) {
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }

    // //Schemat ijk
    // for(int i = 0; i < m; i++)
    // {
    //     for(int j = 0; j < m; j++)
    //     {
    //         for(int k = 0; k < m; k++)
    //             C_matrix[i][j] += A_matrix[i][k]*B_matrix[k][j];
    //     }
    // }

    //     //Schemat ikj
    // for(int i = 0; i < m; i++)
    // {
    //     for(int k = 0; k < m; k++)
    //     {
    //         for(int j = 0; j < m; j++)
    //             C_matrix[i][j] += A_matrix[i][k]*B_matrix[k][j];
    //     }
    // }
}

int main(int argc, char *argv[])
{

    int rank, size;
    MPI_Init(0, 0);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != (sizeOfMainMatrix * sizeOfMainMatrix) && rank == 0)
    {
        printf("Your number of process is incorrect!\nPlease try again on %d processes", sizeOfMainMatrix * sizeOfMainMatrix);
        MPI_Finalize();
        return 1;
    }

    // skladanie macierzy bedzie robil tylko proces 0 i liczyc czas w zwiazku z tym tez bedzie tylko proces 0
    // startTimer = MPI_Wtime();
    // endTimer = MPI_Wtime();
    // printf("Czas obliczen = %f\n", endwtime - startwtime);

    int personalMatrixA[sizeOfMiniMatrix][sizeOfMiniMatrix];
    int personalMatrixB[sizeOfMiniMatrix][sizeOfMiniMatrix];

    char fileName[50];

    if (rank == 0)
    {

        readMyPartOfMatrixFromFile("Matrix_A.txt", rank, personalMatrixA);
        sprintf(fileName, "Matrix_A_%d.txt", rank);
        saveLocalMatrixToFile(fileName, personalMatrixA);

        readMyPartOfMatrixFromFile("Matrix_B.txt", rank, personalMatrixB);
        sprintf(fileName, "Matrix_B_%d.txt", rank);
        saveLocalMatrixToFile(fileName, personalMatrixB);

        // int result[sizeOfMainMatrix*sizeOfMiniMatrix][sizeOfMainMatrix*sizeOfMiniMatrix];
        // prepareResultMatrix(result, sizeOfMainMatrix*sizeOfMiniMatrix);
        // saveLocalMatrixToFile("Result.txt",result);

        // int matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
        // int matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
        // readBothMatrixFromFile("Matrix_A.txt", "Matrix_B.txt", matrixA, matrixB);

        // save("A.txt", matrixA);
        // save("B.txt", matrixB);

        // splitAndSendMatrixForProcesses(size, matrixA, matrixB);
        
        int personalMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix];
        multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);

        sprintf(fileName, "Matrix_C_%d.txt", rank);
        saveLocalMatrixToFile(fileName, personalMatrixC);

    }

    else
    {

        readMyPartOfMatrixFromFile("Matrix_A.txt", rank, personalMatrixA);
        readMyPartOfMatrixFromFile("Matrix_B.txt", rank, personalMatrixB);

        // MPI_Recv(&personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // MPI_Recv(&personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        sprintf(fileName, "Matrix_A_%d.txt", rank);
        saveLocalMatrixToFile(fileName, personalMatrixA);

        sprintf(fileName, "Matrix_B_%d.txt", rank);
        saveLocalMatrixToFile(fileName, personalMatrixB);

        int personalMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix];
        multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);

        sprintf(fileName, "Matrix_C_%d.txt", rank);
        saveLocalMatrixToFile(fileName, personalMatrixC);
    }

    MPI_Finalize();
    return 0;
}