#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    // 1. VALIDACIÓN DE ARGUMENTOS
    // Si no se pasa el tamaño por consola, el programa termina.
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <tamaño_matriz>\n", argv[0]);
        return 1;
    }

    // Convertimos el argumento de texto a un número entero
    int n = atoi(argv[1]);
    srand(time(NULL));

    // 2. ASIGNACIÓN DINÁMICA
    int **A = (int **)malloc(n * sizeof(int *));
    int **B = (int **)malloc(n * sizeof(int *));
    int **C = (int **)malloc(n * sizeof(int *));

    for (int i = 0; i < n; i++) {
        A[i] = (int *)malloc(n * sizeof(int));
        B[i] = (int *)malloc(n * sizeof(int));
        C[i] = (int *)malloc(n * sizeof(int));
    }

    // 3. LLENADO E INICIALIZACIÓN
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
            C[i][j] = 0;
        }
    }

    // 4. MEDICIÓN DE TIEMPO DE CPU
    clock_t inicio = clock();

    // Multiplicación optimizada (i, k, j)
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            for (int j = 0; j < n; j++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    clock_t fin = clock();
    double tiempo_cpu = (double)(fin - inicio) / CLOCKS_PER_SEC;

    // 5. SALIDA PARA EL SCRIPT
    // Imprimimos solo el tiempo para que el .doc sea fácil de leer/procesar
    printf("%f\n", tiempo_cpu);

    // 6. LIMPIEZA
    for (int i = 0; i < n; i++) {
        free(A[i]); free(B[i]); free(C[i]);
    }
    free(A); free(B); free(C);

    return 0;
}

