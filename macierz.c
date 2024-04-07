#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// gcc macierz.c -o executable
const int sizeOfMainMatrix = 4;
const int sizeOfMiniMatrix = 3; // it have to bo a square. So 3 means 3 rows and 3 columns = 9 cells inside.
const int placeForOneNumber = 6;

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

void multiplyMatrices(int matrixA[sizeOfMiniMatrix][sizeOfMiniMatrix], int matrixB[sizeOfMiniMatrix][sizeOfMiniMatrix], int matrixC[sizeOfMiniMatrix][sizeOfMiniMatrix])
{

    // schemat ikj
    for (int i = 0; i < sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMiniMatrix; j++)
        {
            matrixC[i][j] = 0; // zerowanie tej macierzy bo tam mogą być śmieci z pamięci
            for (int k = 0; k < sizeOfMiniMatrix; k++)
            {
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

void copyMatrix(int source[sizeOfMiniMatrix][sizeOfMiniMatrix], int destination[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    for (int i = 0; i < sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMiniMatrix; j++)
        {
            destination[i][j] = source[i][j];
        }
    }
}

void sendFirstLocalMatrixForProcesses(int processNumber, int matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], int matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{

    // Przygotowanie bufora do wysłania
    int sendBuffer_A[sizeOfMiniMatrix * sizeOfMiniMatrix];
    int sendBuffer_B[sizeOfMiniMatrix * sizeOfMiniMatrix];

    int helpMatrix[sizeOfMainMatrix][sizeOfMainMatrix]; // pomocnicza macierz ktora ma w swojej komorce numer procesu. Przesuwam sie po niej i odczytuje wartosc zeby wiedziec do jakiego procesu wysłać po przesunięciu
    int indeks = 0;

    for (int row = 0; row < sizeOfMainMatrix; row++)
    {
        for (int column = 0; column < sizeOfMainMatrix; column++)
        {
            helpMatrix[row][column] = indeks;
            indeks++;
        }
    }

    for (int processID = 1; processID < processNumber; processID++)
    {
        // Obliczenie indeksów początkowych wierszy dla danego procesu
        int startRow = (processID / sizeOfMainMatrix) * sizeOfMiniMatrix; // it is an int so for ex 4/3=1, 2/3=0 so process 4 start in 1st row and proces 2 start in row 0
        int startColumn = (processID % sizeOfMainMatrix) * sizeOfMiniMatrix;
        int myRow = processID / sizeOfMainMatrix;
        int myColumn = processID % sizeOfMainMatrix;

        int index = 0;

        // Wypełnienie bufora danymi z macierzy A i B dla danego procesu
        for (int row = startRow; row < startRow + sizeOfMiniMatrix; row++)
        {
            for (int col = startColumn; col < startColumn + sizeOfMiniMatrix; col++)
            {
                sendBuffer_A[index] = matrixA[row][col];
                sendBuffer_B[index] = matrixB[row][col];
                index++;
            }
        }

        MPI_Send(sendBuffer_A, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, (helpMatrix[myRow][(myColumn + myRow) % sizeOfMainMatrix]), 0, MPI_COMM_WORLD); // prawy_sasiad = m[i][(j+1) % n]
        MPI_Send(sendBuffer_B, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, (helpMatrix[(myRow + myColumn) % sizeOfMainMatrix][myColumn]), 1, MPI_COMM_WORLD); // dolny_sasiad = m[(i+1) % n][j]
    }
}

void shiftSendAndMultiply(int procesID, int personalMatrixA[sizeOfMiniMatrix][sizeOfMiniMatrix], int personalMatrixB[sizeOfMiniMatrix][sizeOfMiniMatrix], int personalMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    // Tworzenie komunikatorów wiersza i kolumny
    MPI_Comm rowCommunicator;
    MPI_Comm colCommunicator;
    MPI_Comm_split(MPI_COMM_WORLD, procesID / sizeOfMainMatrix, procesID % sizeOfMainMatrix, &rowCommunicator); // stwórz osobny komunikator dla procesów w tym samym rzędzie i nadaj im ID na podstawie ich kolumny
    MPI_Comm_split(MPI_COMM_WORLD, procesID % sizeOfMainMatrix, procesID / sizeOfMainMatrix, &colCommunicator); // odwrotnie

    // Identyfikacja procesów w obrębie komunikatorów
    int MyIDinRow, MyIDinColumn;
    MPI_Comm_rank(rowCommunicator, &MyIDinRow);    // przypisuje sobie identyfikator w rzedzie
    MPI_Comm_rank(colCommunicator, &MyIDinColumn); // w kolumnie

    // stworzenie potrzebnych zmiennych
    int receivedA[sizeOfMiniMatrix][sizeOfMiniMatrix]; // gdzie bede odbierac
    int receivedB[sizeOfMiniMatrix][sizeOfMiniMatrix];

    MPI_Request req1[sizeOfMiniMatrix], req2[sizeOfMiniMatrix];
    MPI_Request reqS1[sizeOfMiniMatrix], reqS2[sizeOfMiniMatrix];

    for (int step = 0; step < sizeOfMainMatrix; step++)
    {
        // Mnożenie macierzy
        multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);

        int leftNeighbor = (MyIDinRow - 1 + sizeOfMainMatrix) % sizeOfMainMatrix;
        int rightNeighbor = (MyIDinRow + 1) % sizeOfMainMatrix;
        int topNeighbor = (MyIDinColumn - 1 + sizeOfMainMatrix) % sizeOfMainMatrix;
        int bottomNeigbor = (MyIDinColumn + 1) % sizeOfMainMatrix;

        // Wysyłanie i odbieranie wyników
        MPI_Irecv(receivedA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, rightNeighbor, 0, rowCommunicator, &req1[step]);
        MPI_Irecv(receivedB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, bottomNeigbor, 0, colCommunicator, &req2[step]);
        MPI_Isend(personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, leftNeighbor, 0, rowCommunicator, &reqS1[step]);
        MPI_Isend(personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, topNeighbor, 0, colCommunicator, &reqS2[step]);

        // Oczekiwanie na macierze
        MPI_Wait(&req1[step], MPI_STATUS_IGNORE);
        MPI_Wait(&req2[step], MPI_STATUS_IGNORE);
        MPI_Wait(&reqS1[step], MPI_STATUS_IGNORE);
        MPI_Wait(&reqS2[step], MPI_STATUS_IGNORE);

        copyMatrix(receivedA, personalMatrixA);
        copyMatrix(receivedB, personalMatrixB);
    }
}

void createResult(int result[sizeOfMainMatrix*sizeOfMiniMatrix][sizeOfMainMatrix*sizeOfMiniMatrix], int processNumber)
{
    int localMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix];
    for(int procesID = 0; procesID < processNumber; procesID++)
    {
        MPI_Recv(&localMatrixC, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, procesID, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Odebralam od procesu %d\n", procesID);

        int startRow = (procesID / sizeOfMainMatrix) * sizeOfMiniMatrix;
        int startColumn = (procesID % sizeOfMainMatrix) * sizeOfMiniMatrix;
        //int myRow = procesID / sizeOfMainMatrix;
        //int myColumn = procesID % sizeOfMainMatrix;

        // Wypełnienie bufora danymi z macierzy A i B dla danego procesu
        printf("startrow %d, size %d\n", startRow, sizeOfMiniMatrix);
        for (int row = startRow; row < startRow + sizeOfMiniMatrix; row++)
        {
            for (int col = startColumn; col < startColumn + sizeOfMiniMatrix; col++)
            {
                result[row][col] = localMatrixC[row - startRow][col - startColumn];
            }
        }
    }
    
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

    if (rank == 0)
    {
        readMyPartOfMatrixFromFile("Matrix_A.txt", rank, personalMatrixA);
        readMyPartOfMatrixFromFile("Matrix_B.txt", rank, personalMatrixB);

        int matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
        int matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
        readBothMatrixFromFile("Matrix_A.txt", "Matrix_B.txt", matrixA, matrixB);

        sendFirstLocalMatrixForProcesses(size, matrixA, matrixB);
    }

    else
    {
        MPI_Recv(&personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    int personalMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix];
    char fileName[50];

    // sprintf(fileName, "Matrix_A_%d.txt", rank);
    // saveLocalMatrixToFile(fileName, personalMatrixA);

    // sprintf(fileName, "Matrix_B_%d.txt", rank);
    // saveLocalMatrixToFile(fileName, personalMatrixB);

    // mnozenie macierzy ktore zostaly odpowiednio przemieszczone juz podczas pierwszego rozeslania
    multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);

    // kolejne przemieszczenia macierzy i mnożenia
    shiftSendAndMultiply(rank, personalMatrixA, personalMatrixB, personalMatrixC);

    // lokalny wynik
    // sprintf(fileName, "Matrix_C_%d.txt", rank);
    // saveLocalMatrixToFile(fileName, personalMatrixC);

    MPI_Send(personalMatrixC, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_INT, 0, 0, MPI_COMM_WORLD);// wyslij do procesu 0 koncowa macierz

    if(rank==0)
    {  
        int result[sizeOfMainMatrix*sizeOfMiniMatrix][sizeOfMainMatrix*sizeOfMiniMatrix];
        createResult(result,size);
        save("Result.txt",result);
    }

    MPI_Finalize();
    return 0;
}
