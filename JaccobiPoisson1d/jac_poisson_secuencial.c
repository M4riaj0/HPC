/*
 * jacobi_poisson_1d.c
 *
 * Solución secuencial de la ecuación de Poisson 1D usando iteración de Jacobi.
 *
 * Uso: ./jacobi_poisson_1d <k>
 *   k : índice de malla. El número de nodos es NK = 2^k + 1.
 *
 * Ejemplo de compilación:
 *   gcc -O2 -o jacobi_poisson_1d jac_poisson_secuencial.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* =========================================================
 * FUNCIONES MATEMÁTICAS DEL PROBLEMA
 * ========================================================= */

/*
 * exact: solución exacta del problema continuo.
 *   u(x) = x * (x - 1) * e^x
 */
double exact(double x) {
    return x * (x - 1.0) * exp(x);
}

/*
 * force: término de forzamiento (lado derecho de la ecuación).
 *   f(x) = -x * (x + 3) * e^x
 */
double force(double x) {
    return -x * (x + 3.0) * exp(x);
}

/* =========================================================
 * GESTIÓN DE MEMORIA
 * ========================================================= */

/*
 * crear_vector: reserva un arreglo de 'n' doubles inicializado en 0.
 */
double *crear_vector(int n) {
    double *v = (double *)calloc(n, sizeof(double));
    if (!v) {
        fprintf(stderr, "Error: no se pudo reservar memoria para vector de tamaño %d\n", n);
        exit(EXIT_FAILURE);
    }
    return v;
}

/*
 * crear_matriz: reserva una matriz densa n×n de doubles inicializada en 0.
 *   Almacenamiento por filas (row-major).
 */
double *crear_matriz(int n) {
    double *A = (double *)calloc((size_t)n * n, sizeof(double));
    if (!A) {
        fprintf(stderr, "Error: no se pudo reservar memoria para matriz %d×%d\n", n, n);
        exit(EXIT_FAILURE);
    }
    return A;
}

/*
 * Macro de acceso a la matriz plana: A[i][j] → A[i*n + j]
 */
#define MAT(A, i, j, n) ((A)[(i) * (n) + (j)])

/*
 * liberar_vector: libera la memoria de un vector.
 */
void liberar_vector(double *v) {
    free(v);
}

/*
 * liberar_matriz: libera la memoria de una matriz plana.
 */
void liberar_matriz(double *A) {
    free(A);
}

/* =========================================================
 * CONSTRUCCIÓN DEL SISTEMA
 * ========================================================= */

/*
 * construir_malla: genera los nk nodos uniformemente espaciados en [a, b].
 *   xk[j] = a + j * hk,  para j = 0, ..., nk-1
 */
void construir_malla(double *xk, int nk, double a, double b) {
    double hk = (b - a) / (double)(nk - 1);
    for (int j = 0; j < nk; j++) {
        xk[j] = a + j * hk;
    }
}

/*
 * construir_rhs: evalúa el término de forzamiento en cada nodo y aplica
 *   las condiciones de frontera de Dirichlet en los extremos.
 *
 *   fk[0]      = ua  (condición izquierda)
 *   fk[nk-1]   = ub  (condición derecha)
 *   fk[j]      = force(xk[j])  para j interior
 */
void construir_rhs(double *fk, double *xk, int nk, double ua, double ub) {
    for (int j = 0; j < nk; j++) {
        fk[j] = force(xk[j]);
    }
    fk[0]      = ua;
    fk[nk - 1] = ub;
}

/*
 * construir_matriz_A: ensambla la matriz tridiagonal del operador de Poisson
 *   discretizado con diferencias finitas centradas, dividida por hk^2.
 *
 *   Filas interiores (1 ≤ i ≤ nk-2):
 *     A[i][i-1] = -1/hk^2,  A[i][i] = 2/hk^2,  A[i][i+1] = -1/hk^2
 *   Filas de frontera:
 *     A[0][0] = 1,  A[nk-1][nk-1] = 1  (identidad → condición Dirichlet)
 *
 *   NOTA: No multiplicamos el RHS por hk^2 para mantener la coherencia
 *   entre distintos niveles de malla (ver artículo, sección 9).
 */
void construir_matriz_A(double *A, int nk, double hk) {
    double hk2 = hk * hk;

    /* Filas interiores */
    for (int i = 1; i < nk - 1; i++) {
        MAT(A, i, i - 1, nk) = -1.0 / hk2;
        MAT(A, i, i,     nk) =  2.0 / hk2;
        MAT(A, i, i + 1, nk) = -1.0 / hk2;
    }

    /* Filas de frontera (Dirichlet) */
    MAT(A, 0,      0,      nk) = 1.0;
    MAT(A, nk - 1, nk - 1, nk) = 1.0;
}

/* =========================================================
 * SOLUCIÓN DIRECTA (sustitución hacia adelante/atrás para
 * sistema tridiagonal — Thomas algorithm)
 * ========================================================= */

/*
 * resolver_directo: resuelve A*u = f para la matriz tridiagonal construida
 *   usando el algoritmo de Thomas (eliminación gaussiana para tridiagonales).
 *   Guarda la solución en ud[].
 *
 *   Complejidad: O(nk).
 */
void resolver_directo(double *A, double *f, double *ud, int nk) {
    double *c_prima = crear_vector(nk);
    double *d_prima = crear_vector(nk);

    /* Extraer diagonales de la matriz */
    /* a[i] = subdiagonal, b[i] = diagonal principal, c[i] = superdiagonal */

    /* Paso 1: barrido hacia adelante */
    double b0 = MAT(A, 0, 0, nk);
    c_prima[0] = (nk > 1) ? MAT(A, 0, 1, nk) / b0 : 0.0;
    d_prima[0] = f[0] / b0;

    for (int i = 1; i < nk; i++) {
        double ai = MAT(A, i, i - 1, nk);
        double bi = MAT(A, i, i,     nk);
        double ci = (i < nk - 1) ? MAT(A, i, i + 1, nk) : 0.0;

        double denom = bi - ai * c_prima[i - 1];
        c_prima[i] = ci / denom;
        d_prima[i] = (f[i] - ai * d_prima[i - 1]) / denom;
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

/*
 * norma_rms: calcula la norma RMS de un vector v de longitud n.
 *   ||v||_rms = ||v|| / sqrt(n)
 */
double norma_rms(double *v, int n) {
    double suma = 0.0;
    for (int i = 0; i < n; i++) {
        suma += v[i] * v[i];
    }
    return sqrt(suma / (double)n);
}

/*
 * calcular_residual: calcula r = A*u - f y lo guarda en r[].
 */
void calcular_residual(double *A, double *u, double *f, double *r, int nk) {
    for (int i = 0; i < nk; i++) {
        double Au_i = 0.0;
        for (int j = 0; j < nk; j++) {
            Au_i += MAT(A, i, j, nk) * u[j];
        }
        r[i] = Au_i - f[i];
    }
}

/*
 * jacobi: aplica la iteración de Jacobi sobre A*u = f hasta que la norma RMS
 *   del residual sea menor que tol.
 *
 *   Retorna el número de iteraciones realizadas.
 *
 *   Actualización por componente:
 *     u_nuevo[i] = (f[i] - sum_{j≠i} A[i][j]*u_viejo[j]) / A[i][i]
 *
 *   En forma vectorial:
 *     u_nuevo = D^{-1} * (f - (L+U) * u_viejo)
 *             = u_viejo + D^{-1} * (f - A * u_viejo)
 */
int jacobi(int nk, double *A, double *f, double *u, double tol) {
    double *u_viejo  = crear_vector(nk);
    double *residual = crear_vector(nk);

    int it = 0;

    printf("\n  Paso       Residual RMS     Cambio RMS\n");
    printf("  --------   --------------   --------------\n");

    while (1) {
        /* Guardar iterado anterior */
        for (int i = 0; i < nk; i++) {
            u_viejo[i] = u[i];
        }

        /* Actualización de Jacobi: u[i] = (f[i] - Σ_{j≠i} A[i][j]*u_viejo[j]) / A[i][i] */
        for (int i = 0; i < nk; i++) {
            double suma = f[i];
            for (int j = 0; j < nk; j++) {
                if (j != i) {
                    suma -= MAT(A, i, j, nk) * u_viejo[j];
                }
            }
            u[i] = suma / MAT(A, i, i, nk);
        }

        /* Calcular residual r = A*u - f */
        calcular_residual(A, u, f, residual, nk);

        /* Calcular cambio entre iterados */
        double *diff = crear_vector(nk);
        for (int i = 0; i < nk; i++) {
            diff[i] = u[i] - u_viejo[i];
        }

        double res_rms    = norma_rms(residual, nk);
        double cambio_rms = norma_rms(diff, nk);

        it++;

        /* Imprimir cada 100 iteraciones o en las primeras 5 */
        if (it <= 5 || it % 100 == 0) {
            printf("  %6d     %14.6e   %14.6e\n", it, res_rms, cambio_rms);
        }

        liberar_vector(diff);

        /* Criterio de convergencia */
        if (res_rms <= tol) {
            printf("  %6d     %14.6e   %14.6e  ← convergió\n", it, res_rms, cambio_rms);
            break;
        }
    }

    liberar_vector(u_viejo);
    liberar_vector(residual);

    return it;
}

/* =========================================================
 * IMPRESIÓN DE RESULTADOS
 * ========================================================= */

/*
 * imprimir_tabla: muestra la tabla comparativa de soluciones
 *   (exacta, directa, Jacobi) para cada nodo de la malla.
 */
void imprimir_tabla(double *xk, double *uek, double *udk, double *ujk, int nk) {
    printf("\n  %4s  %10s  %12s  %12s  %12s\n",
           "I", "X", "U_Exact", "U_Direct", "U_Jacobi");
    printf("  ----  ----------  ------------  ------------  ------------\n");
    for (int i = 0; i < nk; i++) {
        printf("  %4d  %10.4f  %12.4g  %12.4g  %12.4g\n",
               i + 1, xk[i], uek[i], udk[i], ujk[i]);
    }
}

/*
 * imprimir_errores: muestra estadísticas de error del sistema resuelto.
 */
void imprimir_errores(int k, int nk, double tol, int it_num,
                      double *udk, double *ujk, double *uek) {
    double *diff_jd = crear_vector(nk);
    double *diff_je = crear_vector(nk);

    for (int i = 0; i < nk; i++) {
        diff_jd[i] = udk[i] - ujk[i];
        diff_je[i] = uek[i] - ujk[i];
    }

    printf("\n  Índice de malla K             : %d\n", k);
    printf("  Tamaño del sistema NK         : %d\n", nk);
    printf("  Tolerancia residual RMS       : %g\n", tol);
    printf("  Iteraciones de Jacobi         : %d\n", it_num);
    printf("  Error RMS Jacobi vs Directo   : %g\n", norma_rms(diff_jd, nk));
    printf("  Error RMS Jacobi vs Exacto    : %g\n", norma_rms(diff_je, nk));

    liberar_vector(diff_jd);
    liberar_vector(diff_je);
}

/* =========================================================
 * PROGRAMA PRINCIPAL
 * ========================================================= */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <k>\n", argv[0]);
        fprintf(stderr, "  k : índice de malla (NK = 2^k + 1 nodos)\n");
        return EXIT_FAILURE;
    }

    int k = atoi(argv[1]);

    printf("\nJACOBI_POISSON_1D\n");
    printf("  Solución de la ecuación de Poisson 1D con iteración de Jacobi.\n");

    /* --- Parámetros del problema --- */
    double a  = 0.0, b  = 1.0;   /* Intervalo [a, b]          */
    double ua = 0.0, ub = 0.0;   /* Condiciones de frontera   */
    double tol = 1.0e-6;         /* Tolerancia del residual   */

    int nk = (1 << k) + 1;       /* NK = 2^k + 1              */
    double hk = (b - a) / (double)(nk - 1);

    /* --- Reserva de memoria --- */
    double *xk  = crear_vector(nk);   /* Nodos de la malla         */
    double *fk  = crear_vector(nk);   /* Término de forzamiento    */
    double *uek = crear_vector(nk);   /* Solución exacta continua  */
    double *udk = crear_vector(nk);   /* Solución directa          */
    double *ujk = crear_vector(nk);   /* Solución Jacobi           */
    double *A   = crear_matriz(nk);   /* Matriz del sistema        */

    /* --- Construcción del sistema --- */
    construir_malla(xk, nk, a, b);
    construir_rhs(fk, xk, nk, ua, ub);
    construir_matriz_A(A, nk, hk);

    /* --- Solución exacta del problema continuo --- */
    for (int i = 0; i < nk; i++) {
        uek[i] = exact(xk[i]);
    }

    /* --- Solución directa (Thomas) --- */
    resolver_directo(A, fk, udk, nk);

    /* --- Solución con Jacobi --- */
    /* ujk ya está en cero por calloc → punto de partida x0 = 0 */
    clock_t inicio = clock();
    int it_num = jacobi(nk, A, fk, ujk, tol);
    clock_t fin = clock();
    double tiempo = (double)(fin - inicio) / CLOCKS_PER_SEC;

    /* --- Resultados --- */
    imprimir_errores(k, nk, tol, it_num, udk, ujk, uek);
    printf("  Tiempo de Jacobi (s)          : %f\n", tiempo);
    imprimir_tabla(xk, uek, udk, ujk, nk);

    printf("\nJACOBI_POISSON_1D: fin normal.\n\n");

    /* --- Liberación de memoria --- */
    liberar_vector(xk);
    liberar_vector(fk);
    liberar_vector(uek);
    liberar_vector(udk);
    liberar_vector(ujk);
    liberar_matriz(A);

    return EXIT_SUCCESS;
}