#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <float.h>

// gcc macierz.c -o executable
const int sizeOfMainMatrix = 3;
const int sizeOfMiniMatrix = 4; // it have to bo a square. So 3 means 3 rows and 3 columns = 9 cells inside.
const int placeForOneNumber = 6;

void readMyPartOfMatrixFromFile(const char *fileName, const int procesID, float localMatrix[sizeOfMiniMatrix][sizeOfMiniMatrix])
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
            fscanf(fileMatrix, "%f", &localMatrix[i][j]);
        }
        fseek(fileMatrix, (sizeOfMainMatrix * sizeOfMiniMatrix * placeForOneNumber + 1 - (sizeOfMiniMatrix * placeForOneNumber)), SEEK_CUR);
    }
    fclose(fileMatrix);
    // ToDo zbadac czy dodanie fclose cos zmieni dla innych procesow czytajacych z pliku
}

void readBothMatrixFromFile(const char *fileNameA, const char *fileNameB, float matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], float matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
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

void saveLocalMatrixToFile(const char *fileName, float localMatrix[sizeOfMiniMatrix][sizeOfMiniMatrix])
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
            fprintf(fileMatrix, "%6.1f", localMatrix[row][column]); // placeForOneNumber
        }
        fprintf(fileMatrix, "\n");
    }
    fclose(fileMatrix);
}

void save(const char *fileName, float wholeMatrix[sizeOfMiniMatrix * sizeOfMainMatrix][sizeOfMiniMatrix * sizeOfMainMatrix])
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
            fprintf(fileMatrix, "%6.1f", wholeMatrix[row][column]); // placeForOneNumber
        }
        fprintf(fileMatrix, "\n");
    }
    fclose(fileMatrix);
}

void multiplyMatrices(float matrixA[sizeOfMiniMatrix][sizeOfMiniMatrix], float matrixB[sizeOfMiniMatrix][sizeOfMiniMatrix], float matrixC[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    // schemat ikj
    for (int i = 0; i < sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMiniMatrix; j++)
        {
            for (int k = 0; k < sizeOfMiniMatrix; k++)
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

void sendFirstLocalMatrixForProcesses(int processNumber, float matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], float matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{
    // Przygotowanie bufora do wysłania
    float sendBuffer_A[sizeOfMiniMatrix * sizeOfMiniMatrix];
    float sendBuffer_B[sizeOfMiniMatrix * sizeOfMiniMatrix];

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

        int leftNeighbor = helpMatrix[myRow][(sizeOfMainMatrix - myRow + myColumn) % sizeOfMainMatrix]; // Dodajemy sizeOfMainMatrix, aby zapewnić, że wynik jest nieujemny
        int topNeighbor = helpMatrix[(sizeOfMainMatrix + myRow - myColumn) % sizeOfMainMatrix][myColumn];

        // Jeśli indeks kolumny jest równy 0, przesuwamy się na ostatnią kolumnę
        if (myColumn == 0)
        {
            topNeighbor = processID;
            leftNeighbor = helpMatrix[myRow][sizeOfMainMatrix - myRow];
        }

        if (myRow == 0) // procesow z rzedu 0 nie tykamy
        {
            leftNeighbor = processID;
            topNeighbor = helpMatrix[sizeOfMainMatrix - myColumn][myColumn];
        }

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

        MPI_Send(sendBuffer_A, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, leftNeighbor, 0, MPI_COMM_WORLD); // prawy_sasiad = m[i][(j+1) % n]
        MPI_Send(sendBuffer_B, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, topNeighbor, 1, MPI_COMM_WORLD);  // dolny_sasiad = m[(i+1) % n][j]
    }
}

void shiftSendAndMultiply(int procesID, float personalMatrixA[sizeOfMiniMatrix][sizeOfMiniMatrix], float personalMatrixB[sizeOfMiniMatrix][sizeOfMiniMatrix], float personalMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix])
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
    float receivedA[sizeOfMiniMatrix][sizeOfMiniMatrix]; // gdzie bede odbierac
    float receivedB[sizeOfMiniMatrix][sizeOfMiniMatrix];

    MPI_Request req1[sizeOfMiniMatrix], req2[sizeOfMiniMatrix];
    MPI_Request reqS1[sizeOfMiniMatrix], reqS2[sizeOfMiniMatrix];

    char fileName[50];

    for (int step = 0; step < sizeOfMainMatrix - 1; step++)
    {

        int leftNeighbor = (MyIDinRow - 1 + sizeOfMainMatrix) % sizeOfMainMatrix;
        int rightNeighbor = (MyIDinRow + 1) % sizeOfMainMatrix;
        int topNeighbor = (MyIDinColumn - 1 + sizeOfMainMatrix) % sizeOfMainMatrix;
        int bottomNeigbor = (MyIDinColumn + 1) % sizeOfMainMatrix;

        // Wysyłanie i odbieranie wyników
        MPI_Irecv(receivedA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, rightNeighbor, 0, rowCommunicator, &req1[step]);
        MPI_Irecv(receivedB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, bottomNeigbor, 0, colCommunicator, &req2[step]);
        MPI_Isend(personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, leftNeighbor, 0, rowCommunicator, &reqS1[step]);
        MPI_Isend(personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, topNeighbor, 0, colCommunicator, &reqS2[step]);

        // Oczekiwanie na macierze
        MPI_Wait(&req1[step], MPI_STATUS_IGNORE);
        MPI_Wait(&req2[step], MPI_STATUS_IGNORE);
        MPI_Wait(&reqS1[step], MPI_STATUS_IGNORE);
        MPI_Wait(&reqS2[step], MPI_STATUS_IGNORE);

        copyMatrix(receivedA, personalMatrixA);
        copyMatrix(receivedB, personalMatrixB);

        // Mnożenie macierzy
        multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);

        // sprintf(fileName, "Matrix_A_%d_%d.txt", procesID, step);
        // saveLocalMatrixToFile(fileName, personalMatrixA);

        // sprintf(fileName, "Matrix_B_%d_%d.txt", procesID, step);
        // saveLocalMatrixToFile(fileName, personalMatrixB);

        // sprintf(fileName, "Matrix_C_%d_%d.txt", procesID, step);
        // saveLocalMatrixToFile(fileName, personalMatrixC);
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

int checkResultWithFileFromSequencer(float result[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{
    FILE *sequencerFile = fopen("ResultFromSequencer.txt", "r");
    float sequencerMatrix[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];

    for (int row = 0; row < sizeOfMainMatrix * sizeOfMiniMatrix; row++)
    {
        for (int col = 0; col < sizeOfMainMatrix * sizeOfMiniMatrix; col++)
        {
            fscanf(sequencerFile, "%f", &sequencerMatrix[row][col]);
        }
    }
    // Sprawdzenie poprawności
    int correct_result = 1;
    for (int row = 0; row < sizeOfMainMatrix * sizeOfMiniMatrix; row++)
    {
        for (int col = 0; col < sizeOfMainMatrix * sizeOfMiniMatrix; col++)
        {
            if (sequencerMatrix[row][col] / result[row][col] >= 1.1 || sequencerMatrix[row][col] / result[row][col] <= 0.9)
            {
                return -1;
            }
        }
    }

    return 1;
    fclose(sequencerFile);
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

    float personalMatrixA[sizeOfMiniMatrix][sizeOfMiniMatrix];
    float personalMatrixB[sizeOfMiniMatrix][sizeOfMiniMatrix];
    float personalMatrixC[sizeOfMiniMatrix][sizeOfMiniMatrix];

    memset(personalMatrixC, 0, sizeOfMiniMatrix * sizeOfMiniMatrix * sizeof(float));
    char fileName[50];

    if (rank == 0)
    {
        readMyPartOfMatrixFromFile("Matrix_A.txt", rank, personalMatrixA);
        readMyPartOfMatrixFromFile("Matrix_B.txt", rank, personalMatrixB);

        float matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
        float matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
        readBothMatrixFromFile("Matrix_A.txt", "Matrix_B.txt", matrixA, matrixB);

        double startTimer;
        startTimer = MPI_Wtime();

        sendFirstLocalMatrixForProcesses(size, matrixA, matrixB);
        multiplyMatrices(personalMatrixA, personalMatrixB, personalMatrixC);

        // kolejne przemieszczenia macierzy i mnożenia
        shiftSendAndMultiply(rank, personalMatrixA, personalMatrixB, personalMatrixC);

        // wyslij do samego siebie koncowa macierz
        MPI_Send(personalMatrixC, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);

        double endTimer;
        endTimer = MPI_Wtime();

        float result[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
        createResult(result, size);
        save("ResultFromCannon.txt", result);

        if (checkResultWithFileFromSequencer(result))
        {
            printf("Wynik poprawny!\n");
            printf("Czas obliczen = %f\n", endTimer - startTimer);
        }
        else
        {
            printf("Wynik niepoprawny!\n");
        }
    }

    else
    {
        MPI_Recv(&personalMatrixA, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&personalMatrixB, sizeOfMiniMatrix * sizeOfMiniMatrix, MPI_FLOAT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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
