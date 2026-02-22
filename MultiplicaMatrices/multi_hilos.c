#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

// Estructura para pasar múltiples argumentos al hilo
typedef struct {
    int id;
    int n;
    int num_hilos;
    int **A;
    int **B;
    int **C;
} DatosHilo;

int** crear_matriz(int n){
    int **matriz = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++){
        matriz[i] = (int*)malloc(n * sizeof(int)); // Corregido: sizeof(int)
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

// Función que ejecutará cada hilo
void* multiplicar_hilo(void* arg) {
    DatosHilo* datos = (DatosHilo*)arg;
    int n = datos->n;
    
    // Calcular el rango de filas que le toca a este hilo
    int filas_por_hilo = n / datos->num_hilos;
    int inicio_fila = datos->id * filas_por_hilo;
    int fin_fila = (datos->id == datos->num_hilos - 1) ? n : inicio_fila + filas_por_hilo;

    for (int i = inicio_fila; i < fin_fila; i++) {
        for (int k = 0; k < n; k++) {
            int temp = datos->A[i][k];
            for (int j = 0; j < n; j++) {
                datos->C[i][j] += temp * datos->B[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

void liberar_matriz(int **matriz, int n) {
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

int main(int argc, char *argv[]) {
    // Ahora pedimos N y el número de Hilos
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <n> <num_hilos>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int num_hilos = atoi(argv[2]);
    srand(time(NULL));

    int **A = crear_matriz(n);
    int **B = crear_matriz(n);
    int **C = crear_matriz(n);

    inicializar_matrices(A, B, C, n);

    // Configuración de hilos
    pthread_t hilos[num_hilos];
    DatosHilo datos[num_hilos];

    // Medición de tiempo (incluye la creación y unión de hilos)
    clock_t inicio = clock();

    for (int i = 0; i < num_hilos; i++) {
        datos[i].id = i;
        datos[i].n = n;
        datos[i].num_hilos = num_hilos;
        datos[i].A = A;
        datos[i].B = B;
        datos[i].C = C;
        pthread_create(&hilos[i], NULL, multiplicar_hilo, (void*)&datos[i]);
    }

    for (int i = 0; i < num_hilos; i++) {
        pthread_join(hilos[i], NULL);
    }

    clock_t fin = clock();

    double tiempo_cpu = (double)(fin - inicio) / CLOCKS_PER_SEC;

    // Salida JSON
    printf("  { \"n\": %d, \"hilos\": %d, \"tiempo\": %f },\n", n, num_hilos, tiempo_cpu);

    liberar_matriz(A, n);
    liberar_matriz(B, n);
    liberar_matriz(C, n);

    return 0;
}