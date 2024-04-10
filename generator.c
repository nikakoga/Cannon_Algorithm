#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const int sizeOfMainMatrix = 4;
const int sizeOfMiniMatrix = 500;
int placeForOneNumber = 10;

// float matrix_A[16][16];
// float matrix_B[16][16];

float matrix_A[2000][2000];
float matrix_B[2000][2000];

void createMatrix(int sizeOfMainMatrix, int sizeOfMiniMatrix, int range, float matrix[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
{
    for (int row = 0; row < (sizeOfMiniMatrix * sizeOfMainMatrix); row++)
    {
        for (int column = 0; column < (sizeOfMiniMatrix * sizeOfMainMatrix); column++)
        {
            matrix[row][column] = (rand()%range);
        }
    }
}

void saveToFile(const char *fileName, float matrix[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix])
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
            fprintf(fileMatrix, "%10.1f", matrix[row][column]); //placeForOneNumber
        }
        fprintf(fileMatrix, "\n");
    }
    fclose(fileMatrix);
}


int main(int argc, char *argv[])
{
    srand(time(NULL));

    createMatrix(sizeOfMainMatrix, sizeOfMiniMatrix, 100, matrix_A);
    saveToFile("Matrix_A.txt", matrix_A);

    createMatrix(sizeOfMainMatrix, sizeOfMiniMatrix, 100, matrix_B);
    saveToFile("Matrix_B.txt", matrix_B);
}