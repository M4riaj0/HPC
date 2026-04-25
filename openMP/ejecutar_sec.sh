#!/bin/bash

echo "[" > resultados_sec.json

for vuelta in {1..10}
do
    for i in 1000 2000 3000 4000 5000 
    do
        ./multi_secuencial $i  >> resultados_sec.json
    done
done

sed -i '$ s/,$//' resultados_sec.json
echo "]" >> resultados_sec.json
