#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define MAT(A, i, j, n) ((A)[(i) * (n) + (j)])

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int total, llegados, generacion;
} Barrera;

void barrera_init(Barrera *b, int n) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->total = n; b->llegados = 0; b->generacion = 0;
}
void barrera_esperar(Barrera *b) {
    pthread_mutex_lock(&b->mutex);
    int gen = b->generacion;
    b->llegados++;
    if (b->llegados == b->total) {
        b->llegados = 0; b->generacion++;
        pthread_cond_broadcast(&b->cond);
    } else {
        while (b->generacion == gen)
            pthread_cond_wait(&b->cond, &b->mutex);
    }
    pthread_mutex_unlock(&b->mutex);
}
void barrera_destroy(Barrera *b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

typedef struct {
    int nk, num_hilos, iteracion, convergido;
    double tol, res_rms;
    double *A, *f, *u, *u_viejo, *sumas_parciales;
    Barrera barrera;
} DatosCompartidos;

typedef struct {
    int id, inicio, fin;
    DatosCompartidos *datos;
} DatosHilo;

double exact(double x) { return x * (x - 1.0) * exp(x); }
double force(double x) { return -x * (x + 3.0) * exp(x); }

double *crear_vector(int n) {
    double *v = (double *)calloc(n, sizeof(double));
    if (!v) { fprintf(stderr, "Error memoria\n"); exit(1); }
    return v;
}
double *crear_matriz(int n) {
    double *A = (double *)calloc((size_t)n * n, sizeof(double));
    if (!A) { fprintf(stderr, "Error memoria\n"); exit(1); }
    return A;
}
void liberar_vector(double *v) { free(v); }
void liberar_matriz(double *A) { free(A); }

void construir_malla(double *xk, int nk, double a, double b) {
    double hk = (b - a) / (double)(nk - 1);
    for (int j = 0; j < nk; j++) xk[j] = a + j * hk;
}
void construir_rhs(double *fk, double *xk, int nk, double ua, double ub) {
    for (int j = 0; j < nk; j++) fk[j] = force(xk[j]);
    fk[0] = ua; fk[nk-1] = ub;
}
void construir_matriz_A(double *A, int nk, double hk) {
    double hk2 = hk * hk;
    for (int i = 1; i < nk-1; i++) {
        MAT(A,i,i-1,nk) = -1.0/hk2;
        MAT(A,i,i,  nk) =  2.0/hk2;
        MAT(A,i,i+1,nk) = -1.0/hk2;
    }
    MAT(A,0,0,nk) = 1.0; MAT(A,nk-1,nk-1,nk) = 1.0;
}
void *jacobi_hilo(void *arg) {
    DatosHilo *dh = (DatosHilo *)arg;
    DatosCompartidos *d = dh->datos;
    int inicio = dh->inicio, fin = dh->fin, id = dh->id, nk = d->nk;

    while (!d->convergido) {
        /* Paso 1: Jacobi — mismo calculo que el secuencial, dividido por filas */
        for (int i = inicio; i < fin; i++) {
            double suma = d->f[i];
            for (int j = 0; j < nk; j++)
                if (j != i) suma -= MAT(d->A,i,j,nk) * d->u_viejo[j];
            d->u[i] = suma / MAT(d->A,i,i,nk);
        }

        /* Barrera 1: todos escribieron u[] */
        barrera_esperar(&d->barrera);

        /* Paso 2: copiar u→u_viejo y acumular residual parcial */
        double suma_local = 0.0;
        for (int i = inicio; i < fin; i++) {
            d->u_viejo[i] = d->u[i];
            double Au_i = 0.0;
            for (int j = 0; j < nk; j++)
                Au_i += MAT(d->A,i,j,nk) * d->u[j];
            double r_i = Au_i - d->f[i];
            suma_local += r_i * r_i;
        }
        d->sumas_parciales[id] = suma_local;

        /* Barrera 2: todos calcularon su suma parcial */
        barrera_esperar(&d->barrera);

        /* Solo hilo 0 suma y decide convergencia */
        if (id == 0) {
            double total = 0.0;
            for (int t = 0; t < d->num_hilos; t++) total += d->sumas_parciales[t];
            d->res_rms = sqrt(total / (double)nk);
            d->iteracion++;
            if (d->res_rms <= d->tol) d->convergido = 1;
        }

        /* Barrera 3: hilo 0 actualizó convergido */
        barrera_esperar(&d->barrera);
    }
    pthread_exit(NULL);
}

int jacobi_paralelo(int nk, double *A, double *f, double *u, double tol, int num_hilos) {
    DatosCompartidos datos;
    datos.nk = nk; datos.tol = tol; datos.A = A; datos.f = f; datos.u = u;
    datos.u_viejo = crear_vector(nk);
    datos.sumas_parciales = crear_vector(num_hilos);
    datos.res_rms = 1.0; datos.iteracion = 0; datos.convergido = 0;
    datos.num_hilos = num_hilos;
    for (int i = 0; i < nk; i++) datos.u_viejo[i] = u[i];
    barrera_init(&datos.barrera, num_hilos);

    pthread_t *hilos = malloc(sizeof(pthread_t) * num_hilos);
    DatosHilo *args  = malloc(sizeof(DatosHilo)  * num_hilos);
    int por_hilo = nk / num_hilos, residuo = nk % num_hilos;

    for (int i = 0; i < num_hilos; i++) {
        args[i].id    = i; args[i].datos = &datos;
        args[i].inicio = i*por_hilo + (i<residuo?i:residuo);
        args[i].fin    = args[i].inicio + por_hilo + (i<residuo?1:0);
        pthread_create(&hilos[i], NULL, jacobi_hilo, &args[i]);
    }
    for (int i = 0; i < num_hilos; i++) pthread_join(hilos[i], NULL);

    int it = datos.iteracion;
    barrera_destroy(&datos.barrera);
    liberar_vector(datos.u_viejo);
    liberar_vector(datos.sumas_parciales);
    free(hilos); free(args);
    return it;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <k> [num_hilos]\n", argv[0]);
        return EXIT_FAILURE;
    }
    int k = atoi(argv[1]);
    int num_hilos = (argc >= 3) ? atoi(argv[2]) : 4;
    if (num_hilos < 1) { fprintf(stderr, "Error: num_hilos >= 1\n"); return EXIT_FAILURE; }

    double a=0.0,b=1.0,ua=0.0,ub=0.0,tol=1.0e-6;
    int nk = (1<<k)+1;
    double hk = (b-a)/(double)(nk-1);

    double *xk=crear_vector(nk), *fk=crear_vector(nk);
    double *ujk=crear_vector(nk);
    double *A=crear_matriz(nk);

    construir_malla(xk,nk,a,b);
    construir_rhs(fk,xk,nk,ua,ub);
    construir_matriz_A(A,nk,hk);

    struct timespec ini,fin_t;

    clock_gettime(CLOCK_MONOTONIC,&ini);
    int it_num = jacobi_paralelo(nk,A,fk,ujk,tol,num_hilos);
    clock_gettime(CLOCK_MONOTONIC,&fin_t);
    
    double tiempo=(fin_t.tv_sec-ini.tv_sec)+(fin_t.tv_nsec-ini.tv_nsec)/1e9;

    printf("{\"exponente_k\":%d,\"num_hilos\":%d,\"tiempo_ejecucion_s\":%.6f}\n",
           k,num_hilos,tiempo);

    liberar_vector(xk);liberar_vector(fk);
    liberar_vector(ujk);
    liberar_matriz(A);
    return EXIT_SUCCESS;
}