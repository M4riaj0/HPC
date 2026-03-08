#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>     // Para fork()
#include <sys/wait.h>   // Para wait()
#include <sys/mman.h>   // Para mmap() (Memoria compartida)

// Función para crear la matriz en memoria compartida (necesaria para Procesos)
// Usamos un puntero simple para manejarla como arreglo plano: matriz[i*n + j]
int* crear_matriz_compartida(int n) {
    size_t tamano = n * n * sizeof(int);
    // mmap crea un espacio que el Padre y los Hijos pueden ver y tocar
    int *matriz = mmap(NULL, tamano, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (matriz == MAP_FAILED) {
        perror("Error en mmap");
        exit(1);
    }
    return matriz;
}

void liberar_matriz(int **matriz, int n) {
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

// Las matrices A y B pueden ser normales (malloc) porque los hijos solo las leen
int** crear_matriz_normal(int n) {
    int **matriz = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++) {
        matriz[i] = (int *)malloc(n * sizeof(int));
    }
    return matriz;
}

void inicializar_matrices(int **A, int **B, int *C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[i][j] = rand() % 500;
            B[i][j] = rand() % 500;
            C[i * n + j] = 0; // Matriz compartida es plana
        }
    }
}

// Lo que hace cada proceso (Multiplicación Tradicional Fila x Columna)
void multiplicar_tramo_proceso(int id, int n, int num_procesos, int **A, int **B, int *C) {
    int filas_por_proceso = n / num_procesos;
    int inicio_fila = id * filas_por_proceso;
    int fin_fila = (id == num_procesos - 1) ? n : inicio_fila + filas_por_proceso;

    for (int i = inicio_fila; i < fin_fila; i++) {
        for (int j = 0; j < n; j++) {
            int suma = 0;
            for (int k = 0; k < n; k++) {
                // Tradicional: Fila i de A por Columna j de B
                suma += A[i][k] * B[k][j];
            }
            C[i * n + j] = suma;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <n> <num_procesos>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int num_procesos = atoi(argv[2]);
    srand(time(NULL));

    // A y B pueden ser malloc (copia para cada proceso está bien para leer)
    int **A = crear_matriz_normal(n);
    int **B = crear_matriz_normal(n);
    // C DEBE ser compartida para que el padre reciba los resultados
    int *C = crear_matriz_compartida(n);

    inicializar_matrices(A, B, C, n);

    clock_t inicio = clock();

    for (int i = 0; i < num_procesos; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error al crear proceso");
            exit(1);
        }

        if (pid == 0) { // Lógica del HIJO
            multiplicar_tramo_proceso(i, n, num_procesos, A, B, C);
            exit(0); // El hijo muere aquí para no seguir creando procesos
        }
    }

    // El PADRE espera a todos sus hijos
    for (int i = 0; i < num_procesos; i++) {
        wait(NULL);
    }

    clock_t fin = clock();
    double tiempo_cpu = (double)(fin - inicio) / CLOCKS_PER_SEC;

    printf("  { \"n\": %d, \"procesos\": %d, \"tiempo\": %f },\n", n, num_procesos, tiempo_cpu);

    // Liberar memoria
    liberar_matriz(A, n);
    liberar_matriz(B, n);
    munmap(C, n * n * sizeof(int));

    return 0;
}