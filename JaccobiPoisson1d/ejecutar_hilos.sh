#!/bin/bash

echo "[" > resultados_hilos.json

for vuelta in {1..10}
do
    for n in 2 4 8 16
    do
        for i in  7 8 9 10 11  
        do
           ././jacobi_poisson_hilos $i $n>> resultados_hilos.json
        done
    done
done    


sed -i '$ s/,$//' resultados_hilos.json
echo "]" >> resultados_hilos.json