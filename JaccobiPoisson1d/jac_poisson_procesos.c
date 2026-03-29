/*
 * jacobi_poisson_1d_procesos.c
 *
 * Solución paralela de la ecuación de Poisson 1D usando iteración de Jacobi
 * con múltiples procesos y memoria compartida (mmap).
 *
 * Compilación: gcc -O2 -o jacobi_poisson_procesos jac_poisson_procesos.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>

/* =========================================================
 * ESTRUCTURAS PARA MEMORIA COMPARTIDA
 * ========================================================= */

typedef struct {
    int nk;
    int num_procesos;
    double tol;
    double *A;
    double *f;
    double *u;
    double *u_viejo;
    double *residual;
    double res_rms;
    int iteracion;
    int convergido;
    volatile int barrera_llegados;
    volatile int barrera_generacion;
} datos_compartidos_t;

/* =========================================================
 * FUNCIONES MATEMÁTICAS DEL PROBLEMA
 * ========================================================= */

double exact(double x) {
    return x * (x - 1.0) * exp(x);
}

double force(double x) {
    return -x * (x + 3.0) * exp(x);
}

/* =========================================================
 * GESTIÓN DE MEMORIA
 * ========================================================= */

double *crear_vector(int n) {
    double *v = (double *)calloc(n, sizeof(double));
    if (!v) {
        fprintf(stderr, "Error: no se pudo reservar memoria\n");
        exit(EXIT_FAILURE);
    }
    return v;
}

double *crear_matriz(int n) {
    double *A = (double *)calloc((size_t)n * n, sizeof(double));
    if (!A) {
        fprintf(stderr, "Error: no se pudo reservar memoria\n");
        exit(EXIT_FAILURE);
    }
    return A;
}

#define MAT(A, i, j, n) ((A)[(i) * (n) + (j)])

void liberar_vector(double *v) {
    free(v);
}

void liberar_matriz(double *A) {
    free(A);
}

/* =========================================================
 * CONSTRUCCIÓN DEL SISTEMA
 * ========================================================= */

void construir_malla(double *xk, int nk, double a, double b) {
    double hk = (b - a) / (double)(nk - 1);
    for (int j = 0; j < nk; j++) {
        xk[j] = a + j * hk;
    }
}

void construir_rhs(double *fk, double *xk, int nk, double ua, double ub) {
    for (int j = 0; j < nk; j++) {
        fk[j] = force(xk[j]);
    }
    fk[0]      = ua;
    fk[nk - 1] = ub;
}

void construir_matriz_A(double *A, int nk, double hk) {
    double hk2 = hk * hk;

    for (int i = 1; i < nk - 1; i++) {
        MAT(A, i, i - 1, nk) = -1.0 / hk2;
        MAT(A, i, i,     nk) =  2.0 / hk2;
        MAT(A, i, i + 1, nk) = -1.0 / hk2;
    }

    MAT(A, 0,      0,      nk) = 1.0;
    MAT(A, nk - 1, nk - 1, nk) = 1.0;
}

/* =========================================================
 * FUNCIONES DE CÁLCULO
 * ========================================================= */

double norma_rms(double *v, int n) {
    double suma = 0.0;
    for (int i = 0; i < n; i++) {
        suma += v[i] * v[i];
    }
    return sqrt(suma / (double)n);
}

void calcular_residual(double *A, double *u, double *f, double *r, int nk) {
    for (int i = 0; i < nk; i++) {
        double Au_i = 0.0;
        for (int j = 0; j < nk; j++) {
            Au_i += MAT(A, i, j, nk) * u[j];
        }
        r[i] = Au_i - f[i];
    }
}

/* =========================================================
 * WORKER PARA PROCESOS CON FORK
 * ========================================================= */

void jacobi_worker_proceso(datos_compartidos_t *datos, int id_proceso) {
    int nk = datos->nk;
    int num_procesos = datos->num_procesos;
    
    /* Calcular rango de trabajo */
    int items_por_proceso = nk / num_procesos;
    int residuo = nk % num_procesos;
    
    int inicio = id_proceso * items_por_proceso + ((id_proceso < residuo) ? id_proceso : residuo);
    int fin = inicio + items_por_proceso + ((id_proceso < residuo) ? 1 : 0);

    while (!datos->convergido) {
        /* Actualización de Jacobi */
        for (int i = inicio; i < fin; i++) {
            double suma = datos->f[i];
            for (int j = 0; j < nk; j++) {
                if (j != i) {
                    suma -= MAT(datos->A, i, j, nk) * datos->u_viejo[j];
                }
            }
            datos->u[i] = suma / MAT(datos->A, i, i, nk);
        }

        /* BARRERA: Esperar a que todos terminen */
        int gen = datos->barrera_generacion;
        int llegados = __sync_add_and_fetch((int*)&datos->barrera_llegados, 1);
        
        if (llegados == num_procesos) {
            /* Soy el último: copiar u a u_viejo y verificar convergencia */
            for (int i = 0; i < nk; i++) {
                datos->u_viejo[i] = datos->u[i];
            }

            /* Calcular residual y convergencia */
            calcular_residual(datos->A, datos->u, datos->f, datos->residual, nk);
            datos->res_rms = norma_rms(datos->residual, nk);
            datos->iteracion++;

            if (datos->res_rms <= datos->tol) {
                datos->convergido = 1;
            }

            /* Cambiar generación para despertar a otros */
            datos->barrera_llegados = 0;
            __sync_add_and_fetch((int*)&datos->barrera_generacion, 1);
        } else {
            /* No soy el último: esperar a que el último termine */
            while (datos->barrera_generacion == gen && !datos->convergido) {
                usleep(10);
            }
        }
    }
}

/* =========================================================
 * JACOBI PARALELO CON PROCESOS
 * ========================================================= */

int jacobi_paralelo(int nk, double *A, double *f, double *u, double tol, int num_procesos) {
    const char *mmap_file = "/tmp/jacobi_mmap_XXXXXX";
    char mmap_fname[256];
    strcpy(mmap_fname, mmap_file);
    
    int fd = mkstemp(mmap_fname);
    if (fd == -1) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    size_t size_matrices = sizeof(double) * nk * nk;
    size_t size_vectores = sizeof(double) * nk * 4;
    size_t size_struct = sizeof(datos_compartidos_t);
    size_t total_size = size_struct + size_matrices + size_vectores;

    if (ftruncate(fd, total_size) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *mmap_addr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_addr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    close(fd);

    datos_compartidos_t *datos = (datos_compartidos_t *)mmap_addr;
    double *A_shared = (double *)((char *)mmap_addr + size_struct);
    double *f_shared = (double *)((char *)A_shared + size_matrices);
    double *u_shared = (double *)((char *)f_shared + sizeof(double) * nk);
    double *u_viejo_shared = (double *)((char *)u_shared + sizeof(double) * nk);
    double *residual_shared = (double *)((char *)u_viejo_shared + sizeof(double) * nk);

    datos->nk = nk;
    datos->num_procesos = num_procesos;
    datos->tol = tol;
    datos->A = A_shared;
    datos->f = f_shared;
    datos->u = u_shared;
    datos->u_viejo = u_viejo_shared;
    datos->residual = residual_shared;
    datos->res_rms = 1.0;
    datos->iteracion = 0;
    datos->convergido = 0;
    datos->barrera_llegados = 0;
    datos->barrera_generacion = 0;

    memcpy(A_shared, A, size_matrices);
    memcpy(f_shared, f, sizeof(double) * nk);
    memcpy(u_shared, u, sizeof(double) * nk);
    memcpy(u_viejo_shared, u, sizeof(double) * nk);
    memset(residual_shared, 0, sizeof(double) * nk);

    pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * num_procesos);

    for (int i = 0; i < num_procesos; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            jacobi_worker_proceso(datos, i);
            exit(0);
        } else {
            pids[i] = pid;
        }
    }

    for (int i = 0; i < num_procesos; i++) {
        waitpid(pids[i], NULL, 0);
    }

    int it_final = datos->iteracion;
    for (int i = 0; i < nk; i++) {
        u[i] = datos->u[i];
    }

    munmap(mmap_addr, total_size);
    unlink(mmap_fname);
    free(pids);

    return it_final;
}

/* =========================================================
 * PROGRAMA PRINCIPAL
 * ========================================================= */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <k> [num_procesos]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int k = atoi(argv[1]);
    int num_procesos = (argc >= 3) ? atoi(argv[2]) : 4;

    if (num_procesos < 1) {
        fprintf(stderr, "Error: número de procesos debe ser >= 1\n");
        return EXIT_FAILURE;
    }

    double a  = 0.0, b  = 1.0;
    double ua = 0.0, ub = 0.0;
    double tol = 1.0e-6;

    int nk = (1 << k) + 1;
    double hk = (b - a) / (double)(nk - 1);

    double *xk  = crear_vector(nk);
    double *fk  = crear_vector(nk);
    double *ujk = crear_vector(nk);
    double *A   = crear_matriz(nk);

    construir_malla(xk, nk, a, b);
    construir_rhs(fk, xk, nk, ua, ub);
    construir_matriz_A(A, nk, hk);

    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    int it_num = jacobi_paralelo(nk, A, fk, ujk, tol, num_procesos);
    clock_gettime(CLOCK_MONOTONIC, &fin);
    
    double tiempo = (fin.tv_sec - inicio.tv_sec) + 
                    (fin.tv_nsec - inicio.tv_nsec) / 1e9;

    printf("{\"exponente_k\": %d, \"num_procesos\": %d, \"tiempo_ejecucion_s\": %.6f}\n", k, num_procesos, tiempo);

    liberar_vector(xk);
    liberar_vector(fk);
    liberar_vector(ujk);
    liberar_matriz(A);

    return EXIT_SUCCESS;
}
