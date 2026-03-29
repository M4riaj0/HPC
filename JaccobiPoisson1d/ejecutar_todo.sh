#!/bin/bash

echo "========================================="
echo "  Compilando y ejecutando pruebas Jacobi"
echo "========================================="

echo ""
echo "[0/5] Compilando binarios..."

# Secuencial (sin optimización)
gcc -o jacobi_poisson_secuencial jac_poisson_secuencial.c -lm || { echo "Error compilando secuencial"; exit 1; }
echo "       -> jacobi_poisson_secuencial compilado"

# Compilador (con optimización -O2)
gcc -O2 -o jacobi_poisson_compilador jac_poisson_secuencial.c -lm || { echo "Error compilando compilador"; exit 1; }
echo "       -> jacobi_poisson_compilador compilado"

# Memoria (optimización de almacenamiento)
gcc -O2 -o jacobi_poisson_memoria jac_poisson_memoria.c -lm || { echo "Error compilando memoria"; exit 1; }
echo "       -> jacobi_poisson_memoria compilado"

# Hilos (pthreads)
gcc -O2 -o jacobi_poisson_hilos jac_poisson_hilos.c -lm -lpthread || { echo "Error compilando hilos"; exit 1; }
echo "       -> jacobi_poisson_hilos compilado"

# Procesos (fork + mmap)
gcc -O2 -o jacobi_poisson_procesos jac_poisson_procesos.c -lm || { echo "Error compilando procesos"; exit 1; }
echo "       -> jacobi_poisson_procesos compilado"

echo ""
echo "[1/5] Ejecutando pruebas secuencial..."
echo "[" > resultados_sec.json
for vuelta in {1..10}
do
    for i in 7 8 9 10 11
    do
        ./jacobi_poisson_secuencial $i >> resultados_sec.json
    done
done
sed -i '' '$ s/,$//' resultados_sec.json
echo "]" >> resultados_sec.json
echo "       -> resultados_sec.json generado"

echo "[2/5] Ejecutando pruebas compilador..."
echo "[" > resultados_compilador.json
for vuelta in {1..10}
do
    for i in 7 8 9 10 11
    do
        ./jacobi_poisson_compilador $i >> resultados_compilador.json
    done
done
sed -i '' '$ s/,$//' resultados_compilador.json
echo "]" >> resultados_compilador.json
echo "       -> resultados_compilador.json generado"

echo "[3/5] Ejecutando pruebas memoria..."
echo "[" > resultados_memoria.json
for vuelta in {1..10}
do
    for i in 7 8 9 10 11
    do
        ./jacobi_poisson_memoria $i >> resultados_memoria.json
    done
done
sed -i '' '$ s/,$//' resultados_memoria.json
echo "]" >> resultados_memoria.json
echo "       -> resultados_memoria.json generado"

echo "[4/5] Ejecutando pruebas hilos..."
echo "[" > resultados_hilos.json
for vuelta in {1..10}
do
    for n in 2 4 8 16
    do
        for i in 7 8 9 10 11
        do
            ./jacobi_poisson_hilos $i $n >> resultados_hilos.json
        done
    done
done
sed -i '' '$ s/,$//' resultados_hilos.json
echo "]" >> resultados_hilos.json
echo "       -> resultados_hilos.json generado"

echo "[5/5] Ejecutando pruebas procesos..."
echo "[" > resultados_procesos.json
for vuelta in {1..10}
do
    for n in 2 4 8 16
    do
        for i in 7 8 9 10 11
        do
            ./jacobi_poisson_procesos $i $n >> resultados_procesos.json
        done
    done
done
sed -i '' '$ s/,$//' resultados_procesos.json
echo "]" >> resultados_procesos.json
echo "       -> resultados_procesos.json generado"

echo ""
echo "========================================="
echo "  Todas las pruebas finalizaron"
echo "========================================="
echo "Archivos generados:"
echo "  - resultados_sec.json"
echo "  - resultados_compilador.json"
echo "  - resultados_memoria.json"
echo "  - resultados_hilos.json"
echo "  - resultados_procesos.json"
