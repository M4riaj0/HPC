#!/bin/bash

echo "[" > resultados_memoria.json

for vuelta in {1..10}
do
    for i in 1000 2000 3000 4000 5000 
    do
        ./multi_memoria $i  >> resultados_memoria.json
    done
done

sed -i '$ s/,$//' resultados_memoria.json
echo "]" >> resultados_memoria.json
