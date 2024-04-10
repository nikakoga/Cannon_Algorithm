#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

const int sizeOfMainMatrix = 4;
const int sizeOfMiniMatrix = 500;
#define MINISIZE 2000
const int placeForOneNumber = 6;

float matrixA[MINISIZE][MINISIZE];
float matrixB[MINISIZE][MINISIZE];
float matrixC[MINISIZE][MINISIZE];


void readBothMatrixFromFile(const char *fileNameA, const char *fileNameB, float matrixA[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix], float matrixB[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
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

int main(int argc, char **argv)
{
    // jakby nie dzialalo to macierze static o rozmiarach podanych liczbami

    FILE *FileResult;

    // Delete memory trash from array
    memset(matrixC, 0, sizeOfMainMatrix * sizeOfMiniMatrix * sizeOfMainMatrix * sizeOfMiniMatrix * sizeof(float));

    // Read matrix from file
    readBothMatrixFromFile("Matrix_A.txt", "Matrix_B.txt", matrixA, matrixB);

    struct timeval begin, end;
    long seconds, microseconds;

    gettimeofday(&begin, 0);

    // Scheme ijk
    for (int i = 0; i < sizeOfMainMatrix * sizeOfMiniMatrix; i++)
    {
        for (int j = 0; j < sizeOfMainMatrix * sizeOfMiniMatrix; j++)
        {
            for (int k = 0; k < sizeOfMainMatrix * sizeOfMiniMatrix; k++)
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
        }
    }

    gettimeofday(&end, 0);
    seconds = end.tv_sec - begin.tv_sec;
    microseconds = end.tv_usec - begin.tv_usec;
    double ijk = seconds + microseconds * 1e-6;

    // New try
    memset(matrixC, 0, sizeOfMainMatrix * sizeOfMiniMatrix * sizeOfMainMatrix * sizeOfMiniMatrix * sizeof(float));

    gettimeofday(&begin, 0);

    // Scheme ikj
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

    saveMatrixToFile("ResultFromSequencer.txt", matrixC);

    printf("Time for ijk = %f\n", ijk);
    printf("Time for ikj = %f\n", ikj);

    return 0;
}