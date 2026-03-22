#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define MAT(A, i, j, n) ((A)[(i) * (n) + (j)])

typedef struct {
    int id, nk, num_hilos;
    double tol;
    double *A, *f, **u_ptr, **u_viejo_ptr;
    double *error_local; // Cada hilo escribe su error aquí
    int *convergido;
    pthread_barrier_t *barrera;
} DatosHilo;

void* jacobi_worker(void* arg) {
    DatosHilo* d = (DatosHilo*)arg;
    int nk = d->nk;
    int por_hilo = nk / d->num_hilos;
    int residuo  = nk % d->num_hilos;
    int inicio = d->id * por_hilo + (d->id < residuo ? d->id : residuo);
    int fin    = inicio + por_hilo + (d->id < residuo ? 1 : 0);

    while (!(*(d->convergido))) {
        double *u_actual = *(d->u_ptr);
        double *u_v      = *(d->u_viejo_ptr);

        /* Paso 1: actualizar mi rango con u_viejo */
        for (int i = inicio; i < fin; i++) {
            double suma = d->f[i];
            if (i > 0)       suma -= MAT(d->A, i, i-1, nk) * u_v[i-1];
            if (i < nk - 1)  suma -= MAT(d->A, i, i+1, nk) * u_v[i+1];
            u_actual[i] = suma / MAT(d->A, i, i, nk);
        }

        /* Barrera 1: todos terminaron de escribir u_actual */
        pthread_barrier_wait(d->barrera);

        /* Paso 2: calcular residual de mi rango AHORA que u_actual está completo */
        double suma_local = 0.0;
        for (int i = inicio; i < fin; i++) {
            double r = MAT(d->A, i, i, nk) * u_actual[i];
            if (i > 0)      r += MAT(d->A, i, i-1, nk) * u_actual[i-1];
            if (i < nk - 1) r += MAT(d->A, i, i+1, nk) * u_actual[i+1];
            r -= d->f[i];
            suma_local += r * r;
        }
        d->error_local[d->id] = suma_local;

        /* Barrera 2: todos calcularon su error parcial */
        pthread_barrier_wait(d->barrera);

        /* Solo hilo 0 suma, decide convergencia y hace el swap */
        if (d->id == 0) {
            double total = 0.0;
            for (int h = 0; h < d->num_hilos; h++) total += d->error_local[h];
            if (sqrt(total / nk) <= d->tol) *(d->convergido) = 1;

            /* Pointer swap: O(1) en vez de copiar NK elementos */
            double *temp    = *(d->u_ptr);
            *(d->u_ptr)     = *(d->u_viejo_ptr);
            *(d->u_viejo_ptr) = temp;
        }

        /* Barrera 3: hilo 0 terminó el swap y actualizó convergido */
        pthread_barrier_wait(d->barrera);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;

    int k = atoi(argv[1]);
    int num_hilos = atoi(argv[2]);
    int nk = (1 << k) + 1;
    double hk = 1.0 / (double)(nk - 1);

    double *A = (double *)calloc(nk * nk, sizeof(double));
    double *f = (double *)calloc(nk, sizeof(double));
    double *u1 = (double *)calloc(nk, sizeof(double));
    double *u2 = (double *)calloc(nk, sizeof(double));
    
    // Punteros dinámicos para el intercambio
    double *u_ptr = u1;
    double *u_viejo_ptr = u2;

    // Llenado de matriz (omito por brevedad, igual al anterior)
    for (int i = 1; i < nk - 1; i++) {
        MAT(A, i, i-1, nk) = -1.0/(hk*hk);
        MAT(A, i, i, nk)   =  2.0/(hk*hk);
        MAT(A, i, i+1, nk) = -1.0/(hk*hk);
    }
    MAT(A, 0, 0, nk) = 1.0; MAT(A, nk-1, nk-1, nk) = 1.0;

    pthread_t hilos[num_hilos];
    DatosHilo datos[num_hilos];
    pthread_barrier_t barrera;
    pthread_barrier_init(&barrera, NULL, num_hilos);
    
    double error_locales[num_hilos];
    int convergido = 0;

    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    for (int i = 0; i < num_hilos; i++) {
        datos[i] = (DatosHilo){i, nk, num_hilos, 1e-6, A, f, &u_ptr, &u_viejo_ptr, error_locales, &convergido, &barrera};
        pthread_create(&hilos[i], NULL, jacobi_worker, &datos[i]);
    }

    for (int i = 0; i < num_hilos; i++) pthread_join(hilos[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &fin);
    printf("Tiempo: %.6f s\n", (fin.tv_sec - inicio.tv_sec) + (fin.tv_nsec - inicio.tv_nsec) / 1e9);

    pthread_barrier_destroy(&barrera);
    free(A); free(f); free(u1); free(u2);
    return 0;
}