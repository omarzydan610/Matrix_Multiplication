#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#define DEFAULT_INPUT1 "a"
#define DEFAULT_INPUT2 "b"
#define DEFAULT_OUTPUT "c"

int **A, **B, **C;
int r1, c1, r2, c2;
struct timeval stop, start;

// Function to read a matrix from a file
int **read_matrix(const char *filename, int *rows, int *cols)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("Error: Cannot open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fscanf(file, "row=%d col=%d", rows, cols);
    int **matrix = (int **)malloc((*rows) * sizeof(int *));
    for (int i = 0; i < *rows; i++)
    {
        matrix[i] = (int *)malloc((*cols) * sizeof(int));
        for (int j = 0; j < *cols; j++)
        {
            fscanf(file, "%d", &matrix[i][j]);
        }
    }
    fclose(file);
    return matrix;
}

// Function to write a matrix to a file
void write_matrix(const char *filename, int **matrix, int rows, int cols)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        printf("Error: Cannot open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fprintf(file, "row=%d col=%d\n", rows, cols);
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            fprintf(file, "%d ", matrix[i][j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

// Function to multiply matrices per matrix
void multiply_per_matrix(int **A, int **B, int **C, int r1, int c1, int c2, char *fileOutPrefix)
{
    gettimeofday(&start, NULL);
    for (int i = 0; i < r1; i++)
    {
        for (int j = 0; j < c2; j++)
        {
            C[i][j] = 0;
            for (int k = 0; k < c1; k++)
            {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    printf("Multiplication per Matrix done\n");
    gettimeofday(&stop, NULL);
    printf("Created Thread : %d\n", 1);
    printf("Seconds taken %ld\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken: %d\n\n", stop.tv_usec - start.tv_usec);
    char filename[50];
    sprintf(filename, "%s_per_matrix.txt", fileOutPrefix);
    write_matrix(filename, C, r1, c2);
}

// Thread function to multiply matrices per row
void *multiply_per_row_thread(void *arg)
{
    int row = *(int *)arg;
    for (int i = 0; i < c2; i++)
    {
        for (int j = 0; j < c1; j++)
        {
            C[row][i] += A[row][j] * B[j][i];
        }
    }
    pthread_exit(NULL);
}

// Function to multiply matrices per row
void multiply_per_row(int **A, int **B, int **C, int r1, int c1, int c2, char *fileOutPrefix)
{
    gettimeofday(&start, NULL);
    pthread_t threads[r1];
    for (int i = 0; i < r1; i++)
    {
        int *row = malloc(sizeof(int));
        *row = i;
        pthread_create(&threads[i], NULL, multiply_per_row_thread, (void *)row);
    }
    for (int i = 0; i < r1; i++)
    {
        pthread_join(threads[i], NULL);
    }
    printf("Multiplication per Row done\n");
    gettimeofday(&stop, NULL);
    printf("Created Threads : %d\n", r1);
    printf("Seconds taken %ld\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken: %d\n\n", stop.tv_usec - start.tv_usec);
    char filename[50];
    sprintf(filename, "%s_per_row.txt", fileOutPrefix);
    write_matrix(filename, C, r1, c2);
}

// Struct to pass arguments to the thread function
typedef struct
{
    int row;
    int col;
} multiply_per_element_args;

// Thread function to multiply matrices per element
void *multiply_per_element_thread(void *args)
{
    multiply_per_element_args *arguments = (multiply_per_element_args *)args;
    int row = arguments->row;
    int col = arguments->col;
    free(arguments);

    for (int i = 0; i < c1; i++)
    {
        C[row][col] += A[row][i] * B[i][col];
    }
    pthread_exit(NULL);
}

// Function to multiply matrices per element
void multiply_per_element(int **A, int **B, int **C, int r1, int c1, int c2, char *fileOutPrefix)
{
    gettimeofday(&start, NULL);
    pthread_t threads[r1 * c2];
    int cnt = 0;
    for (int i = 0; i < r1; i++)
    {
        for (int j = 0; j < c2; j++)
        {
            multiply_per_element_args *args = malloc(sizeof(multiply_per_element_args));
            args->row = i;
            args->col = j;
            pthread_create(&threads[cnt], NULL, multiply_per_element_thread, (void *)args);
            cnt++;
        }
    }
    cnt = 0;
    for (int i = 0; i < r1; i++)
    {
        for (int j = 0; j < c2; j++)
        {
            pthread_join(threads[cnt], NULL);
            cnt++;
        }
    }
    printf("Multiplication per Element done\n");
    gettimeofday(&stop, NULL);
    printf("Created Threads : %d\n", r1 * c2);
    printf("Seconds taken %ld\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken: %d\n", stop.tv_usec - start.tv_usec);
    char filename[50];
    sprintf(filename, "%s_per_element.txt", fileOutPrefix);
    write_matrix(filename, C, r1, c2);
}

// Main function
int main(int argc, char *argv[])
{
    char file1[50], file2[50], fileOutPrefix[50];

    // Copy argument or default value
    strcpy(file1, (argc > 1) ? argv[1] : DEFAULT_INPUT1);
    strcpy(file2, (argc > 2) ? argv[2] : DEFAULT_INPUT2);
    strcpy(fileOutPrefix, (argc > 3) ? argv[3] : DEFAULT_OUTPUT);

    // Append ".txt" safely
    strcat(file1, ".txt");
    strcat(file2, ".txt");

    A = read_matrix(file1, &r1, &c1);
    B = read_matrix(file2, &r2, &c2);

    if (c1 != r2)
    {
        printf("Error: Matrices cannot be multiplied (invalid dimensions).\n");
        exit(EXIT_FAILURE);
    }

    C = (int **)malloc(r1 * sizeof(int *));
    for (int i = 0; i < r1; i++)
    {
        C[i] = (int *)malloc(c2 * sizeof(int));
    }
    // Multiply using per-matrix method
    multiply_per_matrix(A, B, C, r1, c1, c2, fileOutPrefix);

    for (int i = 0; i < r1; i++)
    {
        for (int j = 0; j < c2; j++)
        {
            C[i][j] = 0;
        }
    }
    // Multiply using per-row method
    multiply_per_row(A, B, C, r1, c1, c2, fileOutPrefix);

    for (int i = 0; i < r1; i++)
    {
        for (int j = 0; j < c2; j++)
        {
            C[i][j] = 0;
        }
    }
    // Multiply using per-element method
    multiply_per_element(A, B, C, r1, c1, c2, fileOutPrefix);

    // Free allocated memory
    for (int i = 0; i < r1; i++)
        free(A[i]);
    for (int i = 0; i < r2; i++)
        free(B[i]);
    for (int i = 0; i < r1; i++)
        free(C[i]);
    free(A);
    free(B);
    free(C);

    return 0;
}
