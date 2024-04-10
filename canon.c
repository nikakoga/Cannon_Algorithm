#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>

#define MATRIX_SIZE 2000
#define PARTIAL_MATRIX_SIZE 500
#define MAIN_PROCESS_ID 0

// Filenames
const char* filename_A = "Matrix_A.txt", filename_B = "Matrix_B.txt";

// Dimensions of process matrix
static int process_matrix_dimension = (MATRIX_SIZE / PARTIAL_MATRIX_SIZE);

// Number of MPI processes
static int process_count = process_matrix_dimension * process_matrix_dimension;

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

void read_initial_matrices() {
    FILE *file_matrix_A = fopen(filename_A, "r");
    FILE *file_matrix_B = fopen(filename_B, "r");

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
    for(int row = 0; row < PARTIAL_MATRIX_SIZE; row++)
    {
    	for(int col = 0; col < PARTIAL_MATRIX_SIZE; col++)
    	{
    	    result[row][col] = matrix[row_offset + row][col_offset + col];
    	}
    }
}

void send_matrices_to_processes() {
    for (int process_id = 1; process_id < process_count; process_id++) {
        int row_part = process_id / process_matrix_dimension;
        int col_part = process_id % process_matrix_dimension;
        get_part_of_matrix(row_part * PARTIAL_MATRIX_SIZE, ((col_part + row_part) % process_matrix_dimension) * PARTIAL_MATRIX_SIZE, A_matrix, inital_matrix_A);
        MPI_Send(inital_matrix_A, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, process_id, process_id, MPI_COMM_WORLD);
        get_part_of_matrix(((row_part + col_part) % process_matrix_dimension) * PARTIAL_MATRIX_SIZE, col_part * PARTIAL_MATRIX_SIZE, B_matrix, inital_matrix_B);
        MPI_Send(inital_matrix_B, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, process_id, process_id * 2, MPI_COMM_WORLD);
    }
}

void set_initial_local_matrices() {
    get_part_of_matrix(0, 0, A_matrix, local_matrix_A);
    get_part_of_matrix(0, 0, B_matrix, local_matrix_B);
}

int handle_main_communication() {
    reset_matrix_C();

    read_initial_matrices();
    send_matrices_to_processes();
    set_initial_local_matrices();
}

void receive_matrices(int process_id) {
    MPI_Recv(local_matrix_A, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, MAIN_PROCESS_ID, process_id, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(local_matrix_B, PARTIAL_MATRIX_SIZE * PARTIAL_MATRIX_SIZE, MPI_FLOAT, MAIN_PROCESS_ID, process_id * 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

int handle_child_communication(int process_id) {
    int process_matrix_row = process_id / process_matrix_dimension;
    int process_matrix_col = process_id % process_matrix_dimension;

    reset_matrix_C();

    receive_matrices(process_id);
    
    MPI_Comm row_communicator;
    MPI_Comm col_communicator;
	MPI_Comm_split(MPI_COMM_WORLD, process_matrix_row, process_matrix_col, &row_communicator);
    MPI_Comm_split(MPI_COMM_WORLD, process_matrix_col, process_matrix_row, &col_communicator);
}

void reset_matrix_C() {
	for(int i = 0; i < PARTIAL_MATRIX_SIZE; i++) {
		for(int j = 0; j < PARTIAL_MATRIX_SIZE; j++) {
			local_matrix_C[i][j]=0;
        }
	}
}

int main (int argc, char *argv[]) {
    // Initialize MPI communication
    MPI_Init(0, 0);
    
    // Receive process id and count of all processes invoked
    int process_id, process_count_actual;
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count_actual);

    // Check if process count is as expected
    if(process_count_actual != process_count)
    {
        if (process_id == 0)
        {
            printf("Your number of process is incorrect!\nPlease try again on %d processes\n\n", sizeOfMainMatrix * sizeOfMainMatrix);
        }
        MPI_Finalize();
        return -1;
    }

    if (process_id == MAIN_PROCESS_ID) {
        return handle_main_communication();
    } else {
        return handle_child_communication(process_id);
    }
}