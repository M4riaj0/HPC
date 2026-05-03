#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int *road;
    int *next_road;
    int n;
    int total_cars;
    float density;
} TrafficSimulation;

void initialize_road(TrafficSimulation *sim, int n, float density) {
    sim->n = n;
    sim->road = (int *)malloc(n * sizeof(int));
    sim->next_road = (int *)malloc(n * sizeof(int));
    sim->total_cars = 0;
    // Semilla fija para reproducibilidad en pruebas de desempeño
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

int update_step(TrafficSimulation *sim) {
    int moved_count = 0;
    int n = sim->n;
    for (int i = 0; i < n; i++) {
        int left = (i == 0) ? sim->road[n - 1] : sim->road[i - 1]; // Frontera periódica
        int current = sim->road[i];
        int right = (i == n - 1) ? sim->road[0] : sim->road[i + 1]; // Frontera periódica

        if (current == 1) {
            if (right == 0) {
                sim->next_road[i] = 0;
                moved_count++;
            } else {
                sim->next_road[i] = 1;
            }
        } else {
            sim->next_road[i] = (left == 1) ? 1 : 0;
        }
    }
    return moved_count;
}


/**
 * Transfiere el estado calculado al estado actual[cite: 1].
 */
void swap_roads(TrafficSimulation *sim) {
    for (int i = 0; i < sim->n; i++) {
        sim->road[i] = sim->next_road[i];
    }
}

// Optimización: Intercambio de punteros en lugar de copiar elemento por elemento[cite: 1]
// void swap_roads(TrafficSimulation *sim) {
//     int *tmp = sim->road;
//     sim->road = sim->next_road;
//     sim->next_road = tmp;
// }

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <N> <Densidad>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    float density = atof(argv[2]);
    int steps = 1000; // Pasos fijos para comparar tiempos equitativamente
    
    TrafficSimulation sim;
    initialize_road(&sim, n, density);

    // --- Inicio de medición Wall Clock ---
    clock_t start = clock();

    for (int t = 0; t < steps; t++) {
        update_step(&sim);
        swap_roads(&sim);
    }

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    // --- Fin de medición ---

    // Salida en formato JSON para el script
    printf("  {\"n\": %d, \"density\": %.2f, \"time\": %.6f},\n", n, density, time_spent);

    free(sim.road);
    free(sim.next_road);
    return 0;
}