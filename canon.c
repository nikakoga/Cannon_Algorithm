#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>

#define MATRIX_SIZE 2000
#define PARTIAL_MATRIX_SIZE 500
#define MAIN_PROCESS_ID 0

// Dimensions of process matrix
static int process_matrix_dimension = (MATRIX_SIZE / PARTIAL_MATRIX_SIZE);

// Number of MPI processes
static int process_count = (MATRIX_SIZE / PARTIAL_MATRIX_SIZE) * (MATRIX_SIZE / PARTIAL_MATRIX_SIZE);

// Main matrices
static float A_matrix[MATRIX_SIZE][MATRIX_SIZE];
static float B_matrix[MATRIX_SIZE][MATRIX_SIZE];
static float result_matrix_C[MATRIX_SIZE][MATRIX_SIZE];

// Local and received matrices
static float local_matrix_A[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE], local_matrix_B[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE];
static float received_matrix_A[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE], received_matrix_B[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE];
static float local_matrix_C[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE];
static float received_matrix_C[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE];

// Initial matrices to be sent to processes
static float inital_matrix_A[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE], inital_matrix_B[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE];

void read_initial_matrices()
{
    FILE *file_matrix_A = fopen("Matrix_A.txt", "r");
    FILE *file_matrix_B = fopen("Matrix_B.txt", "r");

    for (int row = 0; row < MATRIX_SIZE; row++)
    {
        for (int column = 0; column < MATRIX_SIZE; column++)
        {
            fscanf(file_matrix_A, "%f", &A_matrix[row][column]);
            fscanf(file_matrix_B, "%f", &B_matrix[row][column]);
        }
    }
    fclose(file_matrix_A);
    fclose(file_matrix_B);
}

void get_part_of_matrix(int row_offset, int col_offset, float matrix[MATRIX_SIZE][MATRIX_SIZE], float result[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE])
{
    for (int row = 0; row < PARTIAL_MATRIX_SIZE; row++)
    {
        for (int col = 0; col < PARTIAL_MATRIX_SIZE; col++)
        {
            result[row][col] = matrix[row_offset + row][col_offset + col];
        }
    }
}

void send_matrices_to_processes()
{
    for (int process_id = 1; process_id < process_count; process_id++)
    {
        int row_part = process_id / process_matrix_dimension;
        int col_part = process_id % process_matrix_dimension;
        get_part_of_matrix(row_part * PARTIAL_MATRIX_SIZE, ((col_part + row_part) % process_matrix_dimension) * PARTIAL_MATRIX_SIZE, A_matrix, inital_matrix_A);
        MPI_Send(inital_matrix_A, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, process_id, process_id, MPI_COMM_WORLD);
        get_part_of_matrix(((row_part + col_part) % process_matrix_dimension) * PARTIAL_MATRIX_SIZE, col_part * PARTIAL_MATRIX_SIZE, B_matrix, inital_matrix_B);
        MPI_Send(inital_matrix_B, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, process_id, process_id * 2, MPI_COMM_WORLD);
    }
}

void set_initial_local_matrices()
{
    get_part_of_matrix(0, 0, A_matrix, local_matrix_A);
    get_part_of_matrix(0, 0, B_matrix, local_matrix_B);
}

void receive_matrices(int process_id)
{
    MPI_Recv(local_matrix_A, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, MAIN_PROCESS_ID, process_id, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(local_matrix_B, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, MAIN_PROCESS_ID, process_id * 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void ikj_multiply()
{
    for (int i = 0; i < PARTIAL_MATRIX_SIZE; i++)
    {
        for (int k = 0; k < PARTIAL_MATRIX_SIZE; k++)
        {
            for (int j = 0; j < PARTIAL_MATRIX_SIZE; j++)
            {
                local_matrix_C[i][j] += local_matrix_A[i][k] * local_matrix_B[k][j];
            }
        }
    }
}

void reassign_calculated_to_local()
{
    for (int row = 0; row < PARTIAL_MATRIX_SIZE; row++)
    {
        for (int col = 0; col < PARTIAL_MATRIX_SIZE; col++)
        {
            local_matrix_A[row][col] = received_matrix_A[row][col];
            local_matrix_B[row][col] = received_matrix_A[row][col];
        }
    }
}

void multiply_iteratively(int process_id, MPI_Comm row_communicator, MPI_Comm col_communicator)
{
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    MPI_Request req1[process_count * process_matrix_dimension], req2[process_count * process_matrix_dimension];
    MPI_Request reqS1[process_count * process_matrix_dimension], reqS2[process_count * process_matrix_dimension];
    // imo w tej formie w jakiej zrobila to magda jest to bez sensu,
    // skoro kazdy proces uruchamiajac te funkcje stworzy swoja tablice requestow to
    // totalnie wystarczy aby jej rozmiar wynosil [liczba iteracji = liczba rzedow macierzy procesow]
    int my_row_Id, my_col_Id;
    for (int shift_num = 0; shift_num < process_matrix_dimension; shift_num++)
    {
        ikj_multiply();

        MPI_Comm_rank(row_communicator, &my_row_Id);
        MPI_Comm_rank(col_communicator, &my_col_Id);
        int left_neighbor = (my_row_Id - 1 + process_matrix_dimension) % process_matrix_dimension;
        int right_neighbor = (my_row_Id + 1) % process_matrix_dimension;
        int top_neighbor = (my_col_Id - 1 + process_matrix_dimension) % process_matrix_dimension;
        int bottom_neigbor = (my_col_Id + 1) % process_matrix_dimension;

        float bufsize = MATRIX_SIZE * MATRIX_SIZE / process_matrix_dimension / process_matrix_dimension;

        // Receive calculated matrices
        printf("receiving from right %d in %d\n", right_neighbor, process_id);
        MPI_Irecv(received_matrix_A, bufsize, MPI_FLOAT, right_neighbor, 0, row_communicator, &req1[process_id + (process_matrix_dimension * shift_num)]);
        printf("receiving from bottom left %d in %d\n", bottom_neigbor, process_id);
        MPI_Irecv(received_matrix_B, bufsize, MPI_FLOAT, bottom_neigbor, 0, col_communicator, &req2[process_id + (process_matrix_dimension * shift_num)]);

        // Send calculated matrices
        printf("sending to left %d from %d\n", left_neighbor, process_id);
        MPI_Isend(local_matrix_A, bufsize, MPI_FLOAT, left_neighbor, 0, row_communicator, &reqS1[process_id + (process_matrix_dimension * shift_num)]);
        printf("sending to top %d from %d\n", top_neighbor, process_id);
        MPI_Isend(local_matrix_B, bufsize, MPI_FLOAT, top_neighbor, 0, col_communicator, &reqS2[process_id + (process_matrix_dimension * shift_num)]);

        // Wait for matrices
        MPI_Wait(&req1[process_id + (process_matrix_dimension * shift_num)], MPI_STATUS_IGNORE);
        MPI_Wait(&req2[process_id + (process_matrix_dimension * shift_num)], MPI_STATUS_IGNORE);
        MPI_Wait(&reqS1[process_id + (process_matrix_dimension * shift_num)], MPI_STATUS_IGNORE);
        MPI_Wait(&reqS2[process_id + (process_matrix_dimension * shift_num)], MPI_STATUS_IGNORE);

        reassign_calculated_to_local();
    }
}

void multiply_matrices(int process_id)
{
    int process_matrix_row = process_id / process_matrix_dimension;
    int process_matrix_col = process_id % process_matrix_dimension;

    MPI_Comm row_communicator;
    MPI_Comm col_communicator;
    MPI_Comm_split(MPI_COMM_WORLD, process_matrix_row, process_matrix_col, &row_communicator);
    MPI_Comm_split(MPI_COMM_WORLD, process_matrix_col, process_matrix_row, &col_communicator);

    multiply_iteratively(process_id, row_communicator, col_communicator);
}

void reset_matrix_C()
{
    for (int i = 0; i < PARTIAL_MATRIX_SIZE; i++)
    {
        for (int j = 0; j < PARTIAL_MATRIX_SIZE; j++)
        {
            local_matrix_C[i][j] = 0;
        }
    }
}

void save_to_result(int x, int y, float matrix[PARTIAL_MATRIX_SIZE][PARTIAL_MATRIX_SIZE])
{
    for (int i = 0; i < PARTIAL_MATRIX_SIZE; i++)
    {
        for (int j = 0; j < PARTIAL_MATRIX_SIZE; j++)
        {
            result_matrix_C[x + i][y + j] = matrix[i][j];
        }
    }
}

int handle_child_communication(int process_id)
{
    reset_matrix_C();
    receive_matrices(process_id);
    multiply_matrices(process_id);

    // Send calculated matrix C to main proc
    MPI_Send(local_matrix_C, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, MAIN_PROCESS_ID, process_id, MPI_COMM_WORLD);
    return 0;
}

int handle_main_communication()
{
    reset_matrix_C();

    read_initial_matrices();
    send_matrices_to_processes();
    set_initial_local_matrices();

    multiply_matrices(MAIN_PROCESS_ID);

    int process_matrix_row, process_matrix_col;
    save_to_result(0, 0, local_matrix_C);
    for (int process_id = 1; process_id < process_count; process_id++)
    {
        process_matrix_row = process_id / process_matrix_dimension;
        process_matrix_col = process_id % process_matrix_dimension;
        MPI_Recv(received_matrix_C, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, process_id, process_id, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        save_to_result(process_matrix_row * PARTIAL_MATRIX_SIZE, process_matrix_col * PARTIAL_MATRIX_SIZE, received_matrix_C);
    }

    // endwtime = MPI_Wtime();
    // printf("Czas odczytu pliku = %f\n", endwtime2 - startwtime1);
    // printf("Czas przetwarzania = %f\n", endwtime - startwtime1);
    // printf("Czas obliczen = %f\n", endwtime - startwtime2);

    // Zapis wyniku do pliku
    FILE *matrixC_MPI_file = fopen("wynikAB_MPI_2004_2.txt", "w");
    for (int k = 0; k < MATRIX_SIZE; k++)
    {
        for (int l = 0; l < MATRIX_SIZE; l++)
        {
            fprintf(matrixC_MPI_file, "%6.1f ", result_matrix_C[k][l]);
        }
        fprintf(matrixC_MPI_file, "\n");
    }
    fclose(matrixC_MPI_file);
    // Wczytanie wyniku sekwencyjnego
    FILE *matrixC_s_file = fopen("wynikAB_sekwencyjny_2004_2.txt", "r");
    for (int k = 0; k < MATRIX_SIZE; k++)
    {
        for (int l = 0; l < MATRIX_SIZE; l++)
        {
            fscanf(matrixC_s_file, "%f", &inital_matrix_A[k][l]);
        }
    }
    // Sprawdzenie poprawności
    int correct_result = 1;
    for (int k = 0; k < MATRIX_SIZE; k++)
    {
        for (int l = 0; l < MATRIX_SIZE; l++)
        {
            if (result_matrix_C[k][l] / inital_matrix_A[k][l] >= 1.1 || result_matrix_C[k][l] / inital_matrix_A[k][l] <= 0.9)
            {
                correct_result = 0;
                printf("%d %d %f %f\n", k, l, result_matrix_C[k][l], inital_matrix_A[k][l]);
            }
        }
    }

    if (correct_result == 1)
    {
        printf("Wynik poprawny.\n");
        return 0;
    }
    else
    {
        printf("Wynik nieprawidłowy.\n");
        return 0;
    }
}

int main(int argc, char *argv[])
{
    // Initialize MPI communication
    MPI_Init(0, 0);

    // Receive process id and count of all processes invoked
    int process_id, process_count_actual;
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count_actual);

    // Check if process count is as expected
    if (process_count_actual != process_count)
    {
        if (process_id == 0)
        {
            printf("Your number of process is incorrect!\nPlease try again on %d processes\n\n", process_matrix_dimension * process_matrix_dimension);
        }
        MPI_Finalize();
        return -1;
    }

    int status = -1;
    if (process_id == MAIN_PROCESS_ID)
    {
        status = handle_main_communication();
    }
    else
    {
        status = handle_child_communication(process_id);
    }
    MPI_Finalize();
    return status;
}