#!/bin/bash

echo "[" > resultados_procesos.json

for vuelta in {1..10}
do
    for n in 2 4 8 16 32
    do
        for i in 1000 2000 3000 4000 5000 
        do
           ./multi_procesos $i $n>> resultados_procesos.json
        done
    done
done    


sed -i '$ s/,$//' resultados_procesos.json
echo "]" >> resultados_procesos.json
