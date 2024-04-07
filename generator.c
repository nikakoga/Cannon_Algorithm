#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const int sizeOfMainMatrix = 3; // it have to be a square root from number of process in a program. 9 processes = size of Main Matrix 3 
const int sizeOfMiniMatrix = 4; // 2x2 matrix inside every process 
int placeForOneNumber = 6;

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
            fprintf(fileMatrix, "%6.1f", matrix[row][column]); //placeForOneNumber
        }
        fprintf(fileMatrix, "\n");
    }
    fclose(fileMatrix);
}


int main(int argc, char *argv[])
{
    srand(time(NULL));

    float matrix_A[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
    createMatrix(sizeOfMainMatrix, sizeOfMiniMatrix, 100, matrix_A);
    saveToFile("Matrix_A.txt", matrix_A);

    float matrix_B[sizeOfMainMatrix * sizeOfMiniMatrix][sizeOfMainMatrix * sizeOfMiniMatrix];
    createMatrix(sizeOfMainMatrix, sizeOfMiniMatrix, 100, matrix_B);
    saveToFile("Matrix_B.txt", matrix_B);
}