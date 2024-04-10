#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <float.h>
#include <math.h>

// gcc macierz.c -o executable
// ssh Inf151574@polluks.cs.put.poznan.pl
// ssh lab-cad-3

#define WHOLEMATRIX 2000
#define SMOLATRIX 500
#define BIGATRIX 250000 // SMOLATRIX * SMOLATRIX
#define SQRT_PROCESS_NUM 4

const int sizeOfMainMatrix = SQRT_PROCESS_NUM;
const int sizeOfMiniMatrix = 500;
const int placeForOneNumber = 10;

static float matrixA[WHOLEMATRIX][WHOLEMATRIX];
static float matrixB[WHOLEMATRIX][WHOLEMATRIX];

static float personalMatrixA[SMOLATRIX][SMOLATRIX];
static float personalMatrixB[SMOLATRIX][SMOLATRIX];
static float personalMatrixC[SMOLATRIX][SMOLATRIX];

// static float sendBuffer_A[BIGATRIX];
// static float sendBuffer_B[BIGATRIX];

// stworzenie potrzebnych zmiennych
static float receivedA[SMOLATRIX][SMOLATRIX]; // gdzie bede odbierac
static float receivedB[SMOLATRIX][SMOLATRIX];

static float sequencerMatrix[WHOLEMATRIX][WHOLEMATRIX];

// static float sendFirstA[500][500], sendFirstB[500][500];

static float result[WHOLEMATRIX][WHOLEMATRIX];

void getMatrixForProcess0(float wholeMatrix[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], float myPart[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    for (int i = 0; i < sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMiniMatrix; j++)
        {
            myPart[i][j] = wholeMatrix[i][j];
        }
    }
}

void readBothMatrixFromFile(const char *fileNameA, const char *fileNameB, float matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix],
                            float matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{
    FILE *fileMatrixA = fopen(fileNameA, "r");
    FILE *fileMatrixB = fopen(fileNameB, "r");

    if (fileNameA == NULL || fileNameB == NULL)
    {
        printf("Error while file opening\n");
        MPI_Finalize();
        return;
    }
    for (int row = 0; row < (sizeOfMainMatrix * sizeOfMiniMatrix); row++)
    {
        for (int column = 0; column < (sizeOfMainMatrix * sizeOfMiniMatrix); column++)
        {
            fscanf(fileMatrixA, "%f", &matrixA[row][column]);
            fscanf(fileMatrixB, "%f", &matrixB[row][column]);
        }
    }
    fclose(fileMatrixA);
    fclose(fileMatrixB);
}

void saveMatrixToFile(const char *fileName, float wholeMatrix[sizeOfMiniMatrix * sizeOfMainMatrix][sizeOfMiniMatrix * sizeOfMainMatrix])
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
            fprintf(fileMatrix, "%10.1f", wholeMatrix[row][column]); // placeForOneNumber
        }
        fprintf(fileMatrix, "\n");
    }
    fclose(fileMatrix);
}

void multiplyMatrices(float matrixA[sizeOfMiniMatrix][sizeOfMiniMatrix], float matrixB[sizeOfMiniMatrix][sizeOfMiniMatrix],
                      float matrixC[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    // schemat ikj
    for (int i = 0; i < sizeOfMiniMatrix; i++)
    {
        for (int k = 0; k < sizeOfMiniMatrix; k++)
        {
            for (int j = 0; j < sizeOfMiniMatrix; j++)
            {
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }
}

void copyMatrix(float source[sizeOfMiniMatrix][sizeOfMiniMatrix], float destination[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    for (int i = 0; i < sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMiniMatrix; j++)
        {
            destination[i][j] = source[i][j];
        }
    }
}

// void sendFirstLocalMatrixForProcesses(int processNumber, float matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix],
//                                       float matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
// {
//     // Przygotowanie bufora do wysłania
//     int helpMatrix[sizeOfMainMatrix][sizeOfMainMatrix]; // pomocnicza macierz ktora ma w swojej komorce numer procesu. Przesuwam sie po niej i odczytuje wartosc zeby wiedziec do jakiego procesu wysłać po przesunięciu
//     int indeks = 0;

//     for (int row = 0; row < sizeOfMainMatrix; row++)
//     {
//         for (int column = 0; column < sizeOfMainMatrix; column++)
//         {
//             helpMatrix[row][column] = indeks;
//             indeks++;
//         }
//     }

//     for (int processID = 1; processID < processNumber; processID++)
//     {
//         // Obliczenie indeksów początkowych wierszy dla danego procesu
//         int startRow = (processID / sizeOfMainMatrix) * sizeOfMiniMatrix;
//         int startColumn = (processID % sizeOfMainMatrix) * sizeOfMiniMatrix;
//         int myRow = processID / sizeOfMainMatrix;
//         int myColumn = processID % sizeOfMainMatrix;

//         int leftNeighbor = helpMatrix[myRow][(sizeOfMainMatrix - myRow + myColumn) % sizeOfMainMatrix];
//         int topNeighbor = helpMatrix[(sizeOfMainMatrix + myRow - myColumn) % sizeOfMainMatrix][myColumn];

//         // Jeśli indeks kolumny jest równy 0, przesuwamy się na ostatnią kolumnę
//         if (myColumn == 0)
//         {
//             topNeighbor = processID;
//             leftNeighbor = helpMatrix[myRow][sizeOfMainMatrix - myRow];
//         }

//         if (myRow == 0) // procesow z rzedu 0 nie tykamy
//         {
//             leftNeighbor = processID;
//             topNeighbor = helpMatrix[sizeOfMainMatrix - myColumn][myColumn];
//         }

//         int index = 0;

//         // Wypełnienie bufora danymi z macierzy A i B dla danego procesu
//         for (int row = startRow; row < startRow + sizeOfMiniMatrix; row++)
//         {
//             for (int col = startColumn; col < startColumn + sizeOfMiniMatrix; col++)
//             {
//                 sendBuffer_A[index] = matrixA[row][col];
//                 sendBuffer_B[index] = matrixB[row][col];
//                 index++;
//             }
//         }
//         printf("Proces 0 wysylam do %d\n", leftNeighbor);
//         MPI_Send(sendBuffer_A, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, leftNeighbor, 0, MPI_COMM_WORLD);
//         printf("Proces 0 wyslal do %d\n", leftNeighbor);
//         printf("Proces 0 wysylam do %d\n", topNeighbor);
//         MPI_Send(sendBuffer_B, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, topNeighbor, 1, MPI_COMM_WORLD);
//         printf("Proces 0 wyslal do %d\n", topNeighbor);
//     }
// }

void cutRightPieceOfMatrix(int x, int y, float matrix[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], float result[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    for (int i = 0; i < sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMiniMatrix; j++)
        {
            result[i][j] = matrix[x + i][y + j];
        }
    }
}

void shiftSendAndMultiply(int procesID, float personalMatrixA[sizeOfMiniMatrix][sizeOfMiniMatrix], float personalMatrixB[sizeOfMiniMatrix][sizeOfMiniMatrix],
                          float personalMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    MPI_Request reqA[sizeOfMainMatrix * sizeOfMainMatrix * sizeOfMainMatrix], reqB[sizeOfMainMatrix * sizeOfMainMatrix * sizeOfMainMatrix];
    MPI_Request reqSendA[sizeOfMainMatrix * sizeOfMainMatrix * sizeOfMainMatrix], reqSendB[sizeOfMainMatrix * sizeOfMainMatrix * sizeOfMainMatrix];

    // Tworzenie komunikatorów wiersza i kolumny
    MPI_Comm rowCommunicator;
    MPI_Comm colCommunicator;
    MPI_Comm_split(MPI_COMM_WORLD, procesID / sizeOfMainMatrix, procesID % sizeOfMainMatrix, &rowCommunicator); // stwórz osobny komunikator dla procesów w tym samym rzędzie i nadaj im ID na podstawie ich kolumny
    MPI_Comm_split(MPI_COMM_WORLD, procesID % sizeOfMainMatrix, procesID / sizeOfMainMatrix, &colCommunicator); // odwrotnie

    // Identyfikacja procesów w obrębie komunikatorów
    int MyIDinRow, MyIDinColumn;
    MPI_Comm_rank(rowCommunicator, &MyIDinRow);    // przypisuje sobie identyfikator w rzedzie
    MPI_Comm_rank(colCommunicator, &MyIDinColumn); // w kolumnie

    char fileName[50];

    for (int step = 0; step < sizeOfMainMatrix - 1; step++)
    {

        int leftNeighbor = (MyIDinRow - 1 + sizeOfMainMatrix) % sizeOfMainMatrix;
        int rightNeighbor = (MyIDinRow + 1) % sizeOfMainMatrix;
        int topNeighbor = (MyIDinColumn - 1 + sizeOfMainMatrix) % sizeOfMainMatrix;
        int bottomNeigbor = (MyIDinColumn + 1) % sizeOfMainMatrix;

        // Wysyłanie i odbieranie wyników
        printf("Jestem %d odebralam od %d iteracja %d\n", procesID, rightNeighbor, step);
        MPI_Irecv(receivedA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, rightNeighbor, 0, rowCommunicator, &reqA[procesID + (sizeOfMainMatrix * step)]);
        printf("Jestem %d odebralam od %d iteracja %d\n", procesID, bottomNeigbor, step);
        MPI_Irecv(receivedB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, bottomNeigbor, 0, colCommunicator, &reqB[procesID + (sizeOfMainMatrix * step)]);

        printf("Jestem %d, wysylam do %d iteracja %d\n", procesID, leftNeighbor, step);
        MPI_Isend(personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, leftNeighbor, 0, rowCommunicator, &reqSendA[procesID + (sizeOfMainMatrix * step)]);
        printf("Jestem %d, wysylam do %d iteracja %d\n", procesID, topNeighbor, step);
        MPI_Isend(personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, topNeighbor, 0, colCommunicator, &reqSendB[procesID + (sizeOfMainMatrix * step)]);

        // Oczekiwanie na macierze
        MPI_Wait(&reqA[procesID + (sizeOfMainMatrix * step)], MPI_STATUS_IGNORE);
        MPI_Wait(&reqB[procesID + (sizeOfMainMatrix * step)], MPI_STATUS_IGNORE);
        MPI_Wait(&reqSendA[procesID + (sizeOfMainMatrix * step)], MPI_STATUS_IGNORE);
        MPI_Wait(&reqSendB[procesID + (sizeOfMainMatrix * step)], MPI_STATUS_IGNORE);

        copyMatrix(receivedA, personalMatrixA);
        copyMatrix(receivedB, personalMatrixB);

        // Mnożenie macierzy
        multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);
    }
}

void createResult(float result[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], int processNumber)
{
    float localMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix];
    for (int procesID = 0; procesID < processNumber; procesID++)
    {
        MPI_Recv(&localMatrixC, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, procesID, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int startRow = (procesID / sizeOfMainMatrix) * sizeOfMiniMatrix;
        int startColumn = (procesID % sizeOfMainMatrix) * sizeOfMiniMatrix;

        // Wypełnienie bufora danymi z macierzy A i B dla danego procesu
        for (int row = startRow; row < startRow + sizeOfMiniMatrix; row++)
        {
            for (int col = startColumn; col < startColumn + sizeOfMiniMatrix; col++)
            {
                result[row][col] = localMatrixC[row - startRow][col - startColumn];
            }
        }
    }
}

void checkResultWithFileFromSequencer(float result[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{
    FILE *sequencerFile = fopen("ResultFromSequencer.txt", "r");

    for (int row = 0; row < sizeOfMainMatrix * sizeOfMiniMatrix; row++)
    {
        for (int col = 0; col < sizeOfMainMatrix * sizeOfMiniMatrix; col++)
        {
            fscanf(sequencerFile, "%f", &sequencerMatrix[row][col]);
        }
    }
    // Sprawdzenie poprawności

    for (int row = 0; row < sizeOfMainMatrix * sizeOfMiniMatrix; row++)
    {
        for (int col = 0; col < sizeOfMainMatrix * sizeOfMiniMatrix; col++)
        {
            if (fabs(sequencerMatrix[row][col] - result[row][col]) >= 0.1 * sequencerMatrix[row][col])
            {
                printf("Wynik niepoprawny\n");
                return;
            }
        }
    }
    printf("Wynik poprawny!");
    fclose(sequencerFile);
}

int main(int argc, char *argv[])
{
    int rank, size;
    MPI_Init(0, 0);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != (sizeOfMainMatrix * sizeOfMainMatrix))
    {
        if (rank == 0)
        {
            printf("Your number of process is incorrect!\nPlease try again on %d processes\n\n", sizeOfMainMatrix * sizeOfMainMatrix);
        }
        MPI_Finalize();
        return 1;
    }

    memset(personalMatrixC, 0, sizeOfMiniMatrix * sizeOfMiniMatrix * sizeof(float));
    char fileName[50];

    if (rank == 0)
    {

        readBothMatrixFromFile("Matrix_A.txt", "Matrix_B.txt", matrixA, matrixB);

        // getMatrixForProcess0(matrixA, personalMatrixA);
        // getMatrixForProcess0(matrixB, personalMatrixB);

        // sendFirstLocalMatrixForProcesses(size, matrixA, matrixB);
        int i_to_send, j_to_send;
        for (int k = 1; k < size; k++)
        {
            i_to_send = k / sizeOfMainMatrix; // wiersz
            j_to_send = k % sizeOfMainMatrix; // kolumna
            cutRightPieceOfMatrix(i_to_send * sizeOfMiniMatrix, ((j_to_send + i_to_send) % sizeOfMainMatrix) * sizeOfMiniMatrix, matrixA, personalMatrixA);
            MPI_Send(personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, k, k, MPI_COMM_WORLD);
            cutRightPieceOfMatrix(((i_to_send + j_to_send) % sizeOfMainMatrix) * sizeOfMiniMatrix, j_to_send * sizeOfMiniMatrix, matrixB, personalMatrixB);
            MPI_Send(personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, k, k * 2, MPI_COMM_WORLD);
        }
        cutRightPieceOfMatrix(0, 0, matrixA, personalMatrixA);
        cutRightPieceOfMatrix(0, 0, matrixB, personalMatrixB);

        double startTimer;
        startTimer = MPI_Wtime();

        multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);

        // kolejne przemieszczenia macierzy i mnożenia
        shiftSendAndMultiply(rank, personalMatrixA, personalMatrixB, personalMatrixC);

        // wyslij do samego siebie koncowa macierz
        MPI_Send(personalMatrixC, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);

        createResult(result, size);

        double endTimer;
        endTimer = MPI_Wtime();

        saveMatrixToFile("ResultFromCannon.txt", result);

        checkResultWithFileFromSequencer(result);

        printf("Czas obliczen = %f\n", endTimer - startTimer);
    }

    else
    {
        // printf("Proces %d czekam na wiadomosc\n", rank);
        // MPI_Recv(&personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // printf("Proces %d odebralam pierwsza wiadomosc\n", rank);
        // MPI_Recv(&personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // printf("Proces %d odebralam druga wiadomosc\n", rank);

        // printf("Proces %d czekam na wiadomosc\n", rank);
        MPI_Recv(personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // printf("Proces %d odebralam pierwsza wiadomosc\n", rank);
        MPI_Recv(personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, rank * 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Proces %d odebralam druga wiadomosc\n", rank);

        // mnozenie macierzy ktore zostaly odpowiednio przemieszczone juz podczas pierwszego rozeslania
        multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);

        // kolejne przemieszczenia macierzy i mnożenia
        shiftSendAndMultiply(rank, personalMatrixA, personalMatrixB, personalMatrixC);

        // wyslij do procesu 0 koncowa macierz
        MPI_Send(personalMatrixC, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
