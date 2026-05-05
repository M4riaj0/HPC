#!/bin/bash

gcc -O2 cell_automation_sec.c -o cell_automation_sec

echo "[" > resultados_secuencial.json

# Realizamos 5 vueltas para promediar después
for vuelta in {1..10}
do
    # tamaños de carretera (N)
    for n in 500000 1000000 2000000 4000000
    do
        # densidades (0.1 a 0.9)[cite: 1]
        for d in 0.1 0.3 0.5 0.7 0.9
        do
            ./cell_automation_sec $n $d >> resultados_secuencial.json
        done
    done
done

# Limpieza del JSON
sed -i '$ s/,$//' resultados_secuencial.json
echo "]" >> resultados_secuencial.json

echo "Pruebas completadas. Resultados guardados en resultados_secuencial.json"
