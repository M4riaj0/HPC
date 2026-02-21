#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int** crear_matriz(int n){
    int **matriz = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++){
        matriz[i] = (int*)malloc(n * sizeof(int *));
    }
    return matriz;
}

int inicializar_matrices(int **A, int **B, int **C, int n){
    for(int i = 0; i < n ; i++){
        for(int j = 0; j < n; j++){
            A[i][j] = rand() % 500;
            B[i][j] = rand() % 500;
            C[i][j] = 0;
        }
    }
}

int multiplicar_secuencial(int **A, int **B, int **C, int n){
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            int temp = A[i][k];
            for (int j = 0; j < n; j++) {
                C[i][j] += temp * B[k][j];
            }
        }
    }
}

void liberar_matriz(int **matriz, int n) {
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    srand(time(NULL));

    // Uso de las funciones
    int **A = crear_matriz(n);
    int **B = crear_matriz(n);
    int **C = crear_matriz(n);

    inicializar_matrices(A, B, C, n);

    // Medición de tiempo
    clock_t inicio = clock();
    multiplicar_secuencial(A, B, C, n);
    clock_t fin = clock();

    double tiempo_cpu = (double)(fin - inicio) / CLOCKS_PER_SEC;

    // Salida JSON para tu script de Bash
    printf("  { \"n\": %d, \"tiempo\": %f },\n", n, tiempo_cpu);

    liberar_matriz(A, n);
    liberar_matriz(B, n);
    liberar_matriz(C, n);

    return 0;
}



