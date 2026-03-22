#!/bin/bash

echo "[" > resultados_sec.json

for vuelta in {1..10}
do
    for i in 7 8 9 10 11 
    do
        ./jacobi_poisson_secuencial $i  >> resultados_sec.json
    done
done

sed -i '$ s/,$//' resultados_sec.json
echo "]" >> resultados_sec.json
