#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

const int sizeOfMainMatrix = 3;
const int sizeOfMiniMatrix = 3; // it have to bo a square. So 3 means 3 rows and 3 columns = 9 cells inside.
const int placeForOneNumber = 6;

void readBothMatrixFromFile(const char *fileNameA, const char *fileNameB, int matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], int matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{
    FILE *fileMatrixA = fopen(fileNameA, "r");
    FILE *fileMatrixB = fopen(fileNameB, "r");

    if (fileNameA == NULL || fileNameB == NULL)
    {
        printf("Blad otwarcia pliku\n");
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

void save(const char *fileName, int wholeMatrix[sizeOfMiniMatrix * sizeOfMainMatrix][sizeOfMiniMatrix * sizeOfMainMatrix])
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
            fprintf(fileMatrix, "%*d", placeForOneNumber, wholeMatrix[row][column]);
        }
        fprintf(fileMatrix, "\n");
    }
    fclose(fileMatrix);
}

int main(int argc, char **argv)
{
    // static int matrixA[12][12]; //[sizeOfMainMatrix*sizeOfMiniMatrix][sizeOfMainMatrix*sizeOfMiniMatrix]
    // static int matrixB[12][12];
    // static int matrixC[12][12];

    int matrixA[sizeOfMainMatrix*sizeOfMiniMatrix][sizeOfMainMatrix*sizeOfMiniMatrix];
    int matrixB[sizeOfMainMatrix*sizeOfMiniMatrix][sizeOfMainMatrix*sizeOfMiniMatrix];
    int matrixC[sizeOfMainMatrix*sizeOfMiniMatrix][sizeOfMainMatrix*sizeOfMiniMatrix];

    FILE *matrixC_file;

    // Zerowanie macierzy
    memset(matrixA, 0, sizeOfMainMatrix * sizeOfMiniMatrix * sizeOfMainMatrix * sizeOfMiniMatrix * sizeof(int));
    memset(matrixB, 0, sizeOfMainMatrix * sizeOfMiniMatrix * sizeOfMainMatrix * sizeOfMiniMatrix * sizeof(int));
    memset(matrixC, 0, sizeOfMainMatrix * sizeOfMiniMatrix * sizeOfMainMatrix * sizeOfMiniMatrix * sizeof(int));

    // Odczyt macierzy z pliku
    readBothMatrixFromFile("Matrix_A.txt", "Matrix_B.txt", matrixA, matrixB);

    struct timeval begin, end;
    long seconds, microseconds;

    gettimeofday(&begin, 0);

    // Schemat ijk
    for (int i = 0; i < sizeOfMainMatrix * sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMainMatrix * sizeOfMiniMatrix; j++)
        {
            for (int k = 0; k < sizeOfMainMatrix * sizeOfMiniMatrix; k++)
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
        }
    }

    // Schemat ikj
    for (int i = 0; i < sizeOfMainMatrix * sizeOfMiniMatrix; i++)
    {
        for (int k = 0; k < sizeOfMainMatrix * sizeOfMiniMatrix; k++)
        {
            for (int j = 0; j < sizeOfMainMatrix * sizeOfMiniMatrix; j++)
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
        }
    }

    gettimeofday(&end, 0);
    seconds = end.tv_sec - begin.tv_sec;
    microseconds = end.tv_usec - begin.tv_usec;
    double ijk = seconds + microseconds * 1e-6;

    // Zerowanie macierzy C
    memset(matrixC, 0, sizeOfMainMatrix * sizeOfMiniMatrix * sizeOfMainMatrix * sizeOfMiniMatrix * sizeof(int));

    gettimeofday(&begin, 0);

    // Schemat ikj
    for (int i = 0; i < sizeOfMainMatrix * sizeOfMiniMatrix; i++)
    {
        for (int k = 0; k < sizeOfMainMatrix * sizeOfMiniMatrix; k++)
        {
            for (int j = 0; j < sizeOfMainMatrix * sizeOfMiniMatrix; j++)
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
        }
    }

    gettimeofday(&end, 0);
    seconds = end.tv_sec - begin.tv_sec;
    microseconds = end.tv_usec - begin.tv_usec;
    double ikj = seconds + microseconds * 1e-6;

    matrixC_file = fopen("ResultFromSequencer.txt", "w");
    for (int i = 0; i < sizeOfMainMatrix * sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMainMatrix * sizeOfMiniMatrix; j++)
        {
            fprintf(matrixC_file, "%*d", placeForOneNumber, matrixC[i][j]);
        }
        fprintf(matrixC_file, "\n");
    }
    fclose(matrixC_file);

    printf("Czas obliczen ijk = %f\n", ijk);
    printf("Czas obliczen ikj = %f\n", ikj);

    return 0;
}