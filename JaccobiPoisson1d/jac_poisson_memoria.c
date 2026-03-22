/*
 * jac_poisson_memoria.c
 *
 * Solución secuencial de la ecuación de Poisson 1D usando iteración de Jacobi.
 * OPTIMIZACIÓN: almacenamiento tridiagonal en lugar de matriz densa NK×NK.
 *
 * Uso: ./jacobi_memoria <k>
 *   k : índice de malla. El número de nodos es NK = 2^k + 1.
 *
 * Compilación:
 *   gcc -O2 -o jacobi_memoria jac_poisson_memoria.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef struct {
    double *diag;   /* diagonal principal: A[i][i]   */
    double *sup;    /* superdiagonal:      A[i][i+1] */
    double *sub;    /* subdiagonal:        A[i][i-1] */
    int n;
} MatrizTridiagonal;

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
        fprintf(stderr, "Error: no se pudo reservar memoria para vector de tamaño %d\n", n);
        exit(EXIT_FAILURE);
    }
    return v;
}

void liberar_vector(double *v) {
    free(v);
}

/* OPTIMIZACIÓN: crear solo 3 vectores en vez de matriz NK×NK */
MatrizTridiagonal crear_tridiagonal(int n) {
    MatrizTridiagonal T;
    T.n    = n;
    T.diag = crear_vector(n);
    T.sup  = crear_vector(n);
    T.sub  = crear_vector(n);
    return T;
}

/* OPTIMIZACIÓN: liberar los 3 vectores */
void liberar_tridiagonal(MatrizTridiagonal *T) {
    liberar_vector(T->diag);
    liberar_vector(T->sup);
    liberar_vector(T->sub);
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

/* OPTIMIZACIÓN: llenar los 3 vectores en vez de una matriz NK×NK
 * Antes: doble bucle i,j recorriendo NK² celdas
 * Ahora: un solo bucle de NK pasos llenando diag/sup/sub */
void construir_tridiagonal(MatrizTridiagonal *T, int nk, double hk) {
    double hk2 = hk * hk;

    /* Filas interiores: patrón -1/h², 2/h², -1/h² */
    for (int i = 1; i < nk - 1; i++) {
        T->sub [i] = -1.0 / hk2;   /* vecino izquierdo */
        T->diag[i] =  2.0 / hk2;   /* diagonal         */
        T->sup [i] = -1.0 / hk2;   /* vecino derecho   */
    }

    /* Filas de frontera: identidad (u = valor conocido) */
    T->diag[0]      = 1.0;
    T->diag[nk - 1] = 1.0;
    /* sub[0] y sup[nk-1] quedan en 0 por calloc */
}

/* =========================================================
 * SOLUCIÓN DIRECTA (algoritmo de Thomas para tridiagonales)
 * También aprovecha la estructura tridiagonal: O(NK) pasos
 * ========================================================= */

void resolver_directo(MatrizTridiagonal *T, double *f, double *ud, int nk) {
    double *c_prima = crear_vector(nk);
    double *d_prima = crear_vector(nk);

    /* Paso 1: barrido hacia adelante */
    c_prima[0] = T->sup[0] / T->diag[0];
    d_prima[0] = f[0]      / T->diag[0];

    for (int i = 1; i < nk; i++) {
        double denom = T->diag[i] - T->sub[i] * c_prima[i - 1];
        c_prima[i]   = T->sup[i] / denom;
        d_prima[i]   = (f[i] - T->sub[i] * d_prima[i - 1]) / denom;
    }

    /* Paso 2: sustitución hacia atrás */
    ud[nk - 1] = d_prima[nk - 1];
    for (int i = nk - 2; i >= 0; i--) {
        ud[i] = d_prima[i] - c_prima[i] * ud[i + 1];
    }

    liberar_vector(c_prima);
    liberar_vector(d_prima);
}

/* =========================================================
 * ITERACIÓN DE JACOBI
 * ========================================================= */

double norma_rms(double *v, int n) {
    double suma = 0.0;
    for (int i = 0; i < n; i++) {
        suma += v[i] * v[i];
    }
    return sqrt(suma / (double)n);
}

/* OPTIMIZACIÓN: calcular_residual usando estructura tridiagonal.
 * Antes: doble bucle i,j → O(NK²) operaciones por iteración
 * Ahora: bucle simple i   → O(NK)  operaciones por iteración
 * Cada fila solo tiene 3 valores no-cero, no NK */
void calcular_residual_tri(MatrizTridiagonal *T, double *u,
                           double *f, double *r, int nk) {
    for (int i = 0; i < nk; i++) {
        double Au_i = T->diag[i] * u[i];
        if (i > 0)      Au_i += T->sub[i] * u[i - 1];
        if (i < nk - 1) Au_i += T->sup[i] * u[i + 1];
        r[i] = Au_i - f[i];
    }
}

/* OPTIMIZACIÓN: actualización de Jacobi usando estructura tridiagonal.
 * Antes: u[i] = (f[i] - suma de todos los j≠i) / diag[i]
 *        → bucle interno de NK pasos multiplicando ceros
 * Ahora: solo restamos los vecinos que existen (máximo 2)
 *        → 2 operaciones en vez de NK por nodo */
int jacobi(int nk, MatrizTridiagonal *T, double *f, double *u, double tol) {
    /* POINTER SWAP: dos buffers que se alternan cada iteracion.
     * u_actual apunta al buffer donde escribimos valores nuevos.
     * u_viejo  apunta al buffer donde leemos valores del paso anterior.
     * Antes: for (i) u_viejo[i] = u[i]  ->  O(NK) escrituras por iteracion
     * Ahora: intercambio de punteros     ->  O(1)  siempre                  */
    double *buffer_extra = crear_vector(nk);
    double *u_actual     = u;            /* empieza escribiendo en u[]        */
    double *u_viejo      = buffer_extra; /* empieza leyendo del buffer extra  */
    double *residual     = crear_vector(nk);

    int it = 0;

    while (1) {
        /* Actualización de Jacobi: lee de u_viejo, escribe en u_actual */
        for (int i = 0; i < nk; i++) {
            double suma = f[i];
            if (i > 0)      suma -= T->sub[i] * u_viejo[i - 1];
            if (i < nk - 1) suma -= T->sup[i] * u_viejo[i + 1];
            u_actual[i] = suma / T->diag[i];
        }

        /* Residual tridiagonal: O(NK) */
        calcular_residual_tri(T, u_actual, f, residual, nk);

        double res_rms = norma_rms(residual, nk);
        it++;

        if (res_rms <= tol) {
            /* Si el resultado final quedo en buffer_extra (no en u[]),
             * copiarlo de vuelta para que main() lo vea correcto */
            if (u_actual != u) {
                for (int i = 0; i < nk; i++) u[i] = u_actual[i];
            }

            /* Calcular cambio para mostrarlo en convergencia */
            double *diff = crear_vector(nk);
            for (int i = 0; i < nk; i++) diff[i] = u_actual[i] - u_viejo[i];
            double cambio_rms = norma_rms(diff, nk);
            liberar_vector(diff);

            printf("  %6d     %14.6e   %14.6e  <- convergio\n",
                   it, res_rms, cambio_rms);
            break;
        }

        /* POINTER SWAP: O(1) — los datos no se mueven, solo los punteros.
         * u_actual y u_viejo apuntan alternativamente a u[] y buffer_extra */
        double *temp = u_actual;
        u_actual     = u_viejo;
        u_viejo      = temp;
    }

    liberar_vector(buffer_extra);  /* solo liberamos el buffer que creamos */
    liberar_vector(residual);

    return it;
}
/* =========================================================
 * PROGRAMA PRINCIPAL
 * ========================================================= */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <k>\n", argv[0]);
        fprintf(stderr, "  k : indice de malla (NK = 2^k + 1 nodos)\n");
        return EXIT_FAILURE;
    }

    int k = atoi(argv[1]);

    /* --- Parámetros del problema --- */
    double a  = 0.0, b  = 1.0;
    double ua = 0.0, ub = 0.0;
    double tol = 1.0e-6;

    int nk = (1 << k) + 1;
    double hk = (b - a) / (double)(nk - 1);

    /* --- Reserva de memoria ---
     * OPTIMIZACIÓN: MatrizTridiagonal usa 3×NK doubles
     * en vez de NK×NK doubles de la versión original */
    double           *xk  = crear_vector(nk);
    double           *fk  = crear_vector(nk);
    double           *uek = crear_vector(nk);
    double           *udk = crear_vector(nk);
    double           *ujk = crear_vector(nk);
    MatrizTridiagonal T   = crear_tridiagonal(nk);  /* ← 3 vectores */

    /* --- Construcción del sistema --- */
    construir_malla(xk, nk, a, b);
    construir_rhs(fk, xk, nk, ua, ub);
    construir_tridiagonal(&T, nk, hk);              /* ← O(NK) en vez de O(NK²) */

    /* --- Solución exacta --- */
    for (int i = 0; i < nk; i++) {
        uek[i] = exact(xk[i]);
    }

    /* --- Solución directa (Thomas) --- */
    resolver_directo(&T, fk, udk, nk);

    /* --- Jacobi optimizado --- */
    printf("\nJACOBI_POISSON_1D\n");
    printf("  Solucion de la ecuacion de Poisson 1D con iteracion de Jacobi.\n");
    printf("  Paso       Residual RMS     Cambio RMS\n");
    printf("  --------   --------------   --------------\n");

    clock_t inicio = clock();
    int it_num = jacobi(nk, &T, fk, ujk, tol);
    clock_t fin = clock();
    double tiempo = (double)(fin - inicio) / CLOCKS_PER_SEC;

    printf("{\"exponente_k\": %d, \"num_procesos\": %d, \"iteraciones\": %d, \"tiempo_ejecucion_s\": %.6f}\n",
           k, 1, it_num, tiempo);

    /* --- Liberación de memoria --- */
    liberar_vector(xk);
    liberar_vector(fk);
    liberar_vector(uek);
    liberar_vector(udk);
    liberar_vector(ujk);
    liberar_tridiagonal(&T);                        /* ← libera los 3 vectores */

    return EXIT_SUCCESS;
}