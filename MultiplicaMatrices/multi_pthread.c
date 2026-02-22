#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    int **A;
    int **B;
    int **C;
    int n;
    int fila_inicio;
    int fila_fin;
} ThreadData;

int** crear_matriz(int n){
    int **matriz = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++){
        matriz[i] = (int*)malloc(n * sizeof(int));
    }
    return matriz;
}

void inicializar_matrices(int **A, int **B, int **C, int n){
    for(int i = 0; i < n ; i++){
        for(int j = 0; j < n; j++){
            A[i][j] = rand() % 500;
            B[i][j] = rand() % 500;
            C[i][j] = 0;
        }
    }
}

void* multiplicar_hilo(void *arg){
    ThreadData *data = (ThreadData *)arg;
    int **A = data->A;
    int **B = data->B;
    int **C = data->C;
    int n = data->n;

    for (int i = data->fila_inicio; i < data->fila_fin; i++) {
        for (int k = 0; k < n; k++) {
            int temp = A[i][k];
            for (int j = 0; j < n; j++) {
                C[i][j] += temp * B[k][j];
            }
        }
    }
    return NULL;
}

void liberar_matriz(int **matriz, int n) {
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <n> <num_hilos>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int num_hilos = atoi(argv[2]);

    if (num_hilos > n) num_hilos = n;

    srand(time(NULL));

    int **A = crear_matriz(n);
    int **B = crear_matriz(n);
    int **C = crear_matriz(n);

    inicializar_matrices(A, B, C, n);

    pthread_t *hilos = malloc(num_hilos * sizeof(pthread_t));
    ThreadData *datos = malloc(num_hilos * sizeof(ThreadData));

    int filas_por_hilo = n / num_hilos;
    int filas_sobrantes = n % num_hilos;

    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    int fila_actual = 0;
    for (int t = 0; t < num_hilos; t++) {
        datos[t].A = A;
        datos[t].B = B;
        datos[t].C = C;
        datos[t].n = n;
        datos[t].fila_inicio = fila_actual;
        datos[t].fila_fin = fila_actual + filas_por_hilo + (t < filas_sobrantes ? 1 : 0);
        fila_actual = datos[t].fila_fin;

        pthread_create(&hilos[t], NULL, multiplicar_hilo, &datos[t]);
    }

    for (int t = 0; t < num_hilos; t++) {
        pthread_join(hilos[t], NULL);
    }

    struct timespec fin_ts;
    clock_gettime(CLOCK_MONOTONIC, &fin_ts);

    double tiempo = (fin_ts.tv_sec - inicio.tv_sec) + (fin_ts.tv_nsec - inicio.tv_nsec) / 1e9;

    printf("  { \"n\": %d, \"hilos\": %d, \"tiempo\": %f },\n", n, num_hilos, tiempo);

    free(hilos);
    free(datos);
    liberar_matriz(A, n);
    liberar_matriz(B, n);
    liberar_matriz(C, n);

    return 0;
}
