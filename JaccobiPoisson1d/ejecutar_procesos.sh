#!/bin/bash

echo "========================================="
echo "  Compilando y ejecutando: procesos"
echo "========================================="

echo ""
echo "Compilando..."
gcc -O2 -o jacobi_poisson_procesos jac_poisson_procesos.c -lm || { echo "Error compilando procesos"; exit 1; }
echo "       -> jacobi_poisson_procesos compilado"

echo ""
echo "Ejecutando pruebas procesos..."
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
echo "  Pruebas de procesos finalizadas"
echo "========================================="
