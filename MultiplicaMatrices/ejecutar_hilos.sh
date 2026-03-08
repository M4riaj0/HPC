#!/bin/bash

echo "[" > resultados_hilos.json

for vuelta in {1..10}
do
    for n in 2 4 8 16 32
    do
        for i in 1000 2000 3000 4000 5000 
        do
        ./multi_hilos $i $n>> resultados_hilos.json
        done
    done
done

# Esto es un pequeño truco para quitar la última coma sobrante 
# y cerrar el arreglo de JSON
sed -i '$ s/,$//' resultados_hilos.json
echo "]" >> resultados_hilos.json
