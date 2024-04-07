#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>


int main(int argc, char **argv)
{
    const int sizeOfMainMatrix = 2;
    const int sizeOfMiniMatrix = 3; // it have to bo a square. So 3 means 3 rows and 3 columns = 9 cells inside.

    static float matrixA[6][6]; //[sizeOfMainMatrix*sizeOfMiniMatrix][sizeOfMainMatrix*sizeOfMiniMatrix]
	static float matrixB[6][6];
    static float matrixC[6][6];

    FILE *matrixA_file, *matrixB_file, *matrixC_file;

    //Zerowanie macierzy C
   memset(matrixC, 0, sizeOfMainMatrix*sizeOfMiniMatrix * sizeOfMainMatrix*sizeOfMiniMatrix * sizeof(int));

    //Odczyt macierzy z pliku
    matrixA_file = fopen("MatrixA.txt", "r");
    matrixB_file = fopen("MatrixB.txt", "r");
    if(matrixA_file == NULL || matrixB_file == NULL)
    {
    	printf("Blad otwarcia pliku\n");
    	return -1;
    }
	for (int i=0;i<sizeOfMainMatrix*sizeOfMiniMatrix;i++) 
	{
	    for(int j=0;j<sizeOfMainMatrix*sizeOfMiniMatrix;j++)
		{
			fscanf(matrixA_file, "%f", &matrixA[i][j]);
			fscanf(matrixB_file, "%f", &matrixB[i][j]);
		}
	}

    struct timeval begin, end;
    long seconds, microseconds;

    gettimeofday(&begin, 0);

    //Schemat ijk
    for(int i = 0; i < sizeOfMainMatrix*sizeOfMiniMatrix; i++)
    {
        for(int j = 0; j < sizeOfMainMatrix*sizeOfMiniMatrix; j++)
        {
            for(int k = 0; k < sizeOfMainMatrix*sizeOfMiniMatrix; k++)
                matrixC[i][j] += matrixA[i][k]*matrixB[k][j];
        }
    }

        //Schemat ikj
    for(int i = 0; i < sizeOfMainMatrix*sizeOfMiniMatrix; i++)
    {
        for(int k = 0; k < sizeOfMainMatrix*sizeOfMiniMatrix; k++)
        {
            for(int j = 0; j < sizeOfMainMatrix*sizeOfMiniMatrix; j++)
                matrixC[i][j] += matrixA[i][k]*matrixB[k][j];
        }
    }
    

    gettimeofday(&end, 0);
    seconds = end.tv_sec - begin.tv_sec;
    microseconds = end.tv_usec - begin.tv_usec;
    double ijk = seconds + microseconds*1e-6;

    //Zerowanie macierzy C
    memset(matrixC, 0, sizeOfMainMatrix*sizeOfMiniMatrix * sizeOfMainMatrix*sizeOfMiniMatrix * sizeof(int));

    gettimeofday(&begin, 0);

    //Schemat ikj
    for(int i = 0; i < sizeOfMainMatrix*sizeOfMiniMatrix; i++)
    {
        for(int k = 0; k < sizeOfMainMatrix*sizeOfMiniMatrix; k++)
        {
            for(int j = 0; j < sizeOfMainMatrix*sizeOfMiniMatrix; j++)
                matrixC[i][j] += matrixA[i][k]*matrixB[k][j];
        }
    }

    gettimeofday(&end, 0);
    seconds = end.tv_sec - begin.tv_sec;
    microseconds = end.tv_usec - begin.tv_usec;
    double ikj = seconds + microseconds*1e-6;

    matrixC_file = fopen("wynikSekwencyjny.txt","w");
    for (int i=0; i<sizeOfMainMatrix*sizeOfMiniMatrix; i++)
    {
        for (int j=0; j<sizeOfMainMatrix*sizeOfMiniMatrix; j++)
        {
            fprintf(matrixC_file, "%6.1f ",matrixC[i][j]);
        }
        fprintf(matrixC_file, "\n");
    }
    fclose(matrixC_file);

    printf("Czas obliczen ijk = %f\n", ijk);
    printf("Czas obliczen ikj = %f\n", ikj);

    return 0;
}