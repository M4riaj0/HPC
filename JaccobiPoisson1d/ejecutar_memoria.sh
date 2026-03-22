#!/bin/bash

echo "[" > resultados_memoria.json

for vuelta in {1..10}
do
    for i in 7 8 9 10 11 
    do
        ./jacobi_poisson_memoria $i  >> resultados_memoria.json
    done
done

sed -i '$ s/,$//' resultados_memoria.json
echo "]" >> resultados_memoria.json
