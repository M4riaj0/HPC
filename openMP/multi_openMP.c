#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

double* crear_matriz(int n) {
    return (double*)malloc(n * n * sizeof(double));
}

void inicializar_matrices(double *A, double *B, double *C, int n) {
    for(int i = 0; i < n * n; i++) {
        A[i] = (double)rand() / RAND_MAX;
        B[i] = (double)rand() / RAND_MAX;
        C[i] = 0.0;
    }
}

void multiplicacion_openmp(double *A, double *B, double *C, int n, int num_hilos) {
    omp_set_num_threads(num_hilos);
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            double temporal = A[i * n + k];
            for (int j = 0; j < n; j++) {
                C[i * n + j] += temporal * B[k * n + j];
            }
        }
    }
}

void liberar_matriz(double *matriz) {
    free(matriz);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <n> <num_hilos>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int num_hilos = atoi(argv[2]);
    srand(time(NULL));

    double *A = crear_matriz(n);
    double *B = crear_matriz(n);
    double *C = crear_matriz(n);

    inicializar_matrices(A, B, C, n);

    double inicio = omp_get_wtime();
    multiplicacion_openmp(A, B, C, n, num_hilos);
    double fin = omp_get_wtime();

    double tiempo = fin - inicio;

    printf("  { \"n\": %d, \"hilos\": %d, \"tiempo\": %f },\n", n, num_hilos, tiempo);

    liberar_matriz(A);
    liberar_matriz(B);
    liberar_matriz(C);

    return 0;
}