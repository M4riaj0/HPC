#!/bin/bash

echo "[" > resultados_openMP.json

for vuelta in {1..10}
do
    for n in 2 4 8 16 32
    do
        for i in 1000 2000 3000 4000 5000 
        do
        ./multi_openMP $i $n>> resultados_openMP.json
        done
    done
done

sed -i '$ s/,$//' resultados_openMP.json
echo "]" >> resultados_openMP.json
