#include <stdio.h>
#include <stdlib.h>
#include <omp.h> 

typedef struct {
    int *road;
    int *next_road;
    int n;
    int total_cars;
    float density;
} TrafficSimulation;

// Libera la memoria reservada
void cleanup(TrafficSimulation *sim) {
    free(sim->road);
    free(sim->next_road);
}

void initialize_road(TrafficSimulation *sim, int n, float density) {
    sim->n = n;
    sim->road = (int *)malloc(n * sizeof(int));
    sim->next_road = (int *)malloc(n * sizeof(int));
    sim->total_cars = 0;
    srand(42); 
    for (int i = 0; i < n; i++) {
        if ((float)rand() / RAND_MAX < density) {
            sim->road[i] = 1;
            sim->total_cars++;
        } else {
            sim->road[i] = 0;
        }
    }
}

int update_step_parallel(TrafficSimulation *sim) {
    int moved_count = 0;
    int n = sim->n;
    int *road = sim->road;
    int *next = sim->next_road;

    #pragma omp parallel for reduction(+:moved_count) schedule(static)
    for (int i = 0; i < n; i++) {
        // Frontera periódica: el mundo es un anillo
        int left = (i == 0) ? road[n - 1] : road[i - 1];
        int current = road[i];
        int right = (i == n - 1) ? road[0] : road[i + 1];

        if (current == 1) {
            // Si el de adelante está vacío, se mueve
            if (right == 0) {
                next[i] = 0; 
                moved_count++; 
            } else {
                next[i] = 1; // Bloqueado
            }
        } else {
            // Si está vacío, recibe al de atrás si este se movió
            next[i] = (left == 1) ? 1 : 0;
        }
    }
    return moved_count;
}

void swap_roads_optimized(TrafficSimulation *sim) {
    int *tmp = sim->road;
    sim->road = sim->next_road;
    sim->next_road = tmp;
}

int main(int argc, char *argv[]) {
    // 1. Verificación y lectura de argumentos
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <N> <Densidad> [Pasos]\n", argv[0]);
        return 1;
    }

    // Ahora las variables existen en el ámbito del main
    int n = atoi(argv[1]);
    float density = atof(argv[2]);
    int steps = (argc > 3) ? atoi(argv[3]) : 1000; // Por defecto 1000 pasos
    
    TrafficSimulation sim;
    initialize_road(&sim, n, density);

    // Obtener número de hilos para el JSON
    char* threads_env = getenv("OMP_NUM_THREADS");
    int num_threads = threads_env ? atoi(threads_env) : 1;

    // 2. Medición de tiempo real (Wall Clock) con OpenMP
    double start = omp_get_wtime();

    for (int t = 0; t < steps; t++) {
        update_step_parallel(&sim);
        swap_roads_optimized(&sim);
    }

    double end = omp_get_wtime();
    double time_spent = end - start;

    // 3. Salida compatible con tu script de Bash
    printf("  {\"n\": %d, \"density\": %.2f, \"threads\": %d, \"time\": %.6f},\n", 
            n, density, num_threads, time_spent);

    cleanup(&sim);
    return 0;
}