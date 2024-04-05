#include <stdio.h>
// #include <mpi.h>
#include <stdlib.h>
#include <time.h>

// gcc macierz.c -o executable
const int sizeOfMainMatrix = 3;
const int sizeOfMiniMatrix = 3; // it have to bo a square. So 3 means 3 rows and 3 columns = 9 cells inside.
const int placeForOneNumber = 2;

void readMyPartOfMatrixFromFile(const char *fileName, const int procesID, int localMatrix[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    FILE *fileMatrix = fopen(fileName, "r");

        if (fileMatrix == NULL)
    {
        printf("Error while file opening\n");
        return;
    }

    int rowsToRead = sizeOfMiniMatrix;
    int columnsToRead = sizeOfMiniMatrix * placeForOneNumber;
    int startRow = (procesID/sizeOfMainMatrix) * rowsToRead; //it is an int so for ex 4/3=1, 2/3=0 so process 4 start in 1st row and proces 2 start in row 0
    int startColumn = (procesID%sizeOfMainMatrix)* (sizeOfMiniMatrix * placeForOneNumber);

    fseek(fileMatrix, startRow * (sizeOfMainMatrix * sizeOfMiniMatrix* placeForOneNumber +1), SEEK_SET); // +1 bo znak konca nowej linii
    fseek(fileMatrix, startColumn, SEEK_CUR);


    for (int i=0; i<rowsToRead; i++)
    {
        for(int j=0; j<sizeOfMiniMatrix; j++)
        {
            fscanf(fileMatrix, "%d", &localMatrix[i][j]);
        }
        fseek(fileMatrix, (sizeOfMainMatrix * sizeOfMiniMatrix* placeForOneNumber +1 -(sizeOfMiniMatrix*placeForOneNumber)), SEEK_CUR);
    }
}

void saveLocalMatrixToFile(int localMatrix[sizeOfMiniMatrix][sizeOfMiniMatrix])
{
    FILE *fileMatrix = fopen("LocalMatrix.txt", "w");

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

int main(int argc, char *argv[])
{

    // int rank, size;

    // MPI_Init(0, 0);
    // MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // MPI_Comm_size(MPI_COMM_WORLD, &size);

    // if (size != numberOfProcess && rank == 0)
    // {
    //     printf("Your number of process is incorrect!\n");
    //     MPI_Finalize();
    //     return 0;
    // }

    int localMatrix[sizeOfMiniMatrix][sizeOfMiniMatrix];
    readMyPartOfMatrixFromFile("Matrix_A.txt",8, localMatrix);
    saveLocalMatrixToFile(localMatrix);

    // MPI_Finalize();
    return 0;
}