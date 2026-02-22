#!/bin/bash

echo "Compilando multi_pthread.c..."
gcc -o multi_pthread multi_pthread.c -O2 -lpthread
if [ $? -ne 0 ]; then
    echo "Error al compilar. Abortando."
    exit 1
fi

echo "[" > resultados_pthread.json

for n in 500 1000 2000 4000 6000 8000
do
    for hilos in 2 4 8
    do
        ./multi_pthread $n $hilos >> resultados_pthread.json
    done
done

# Quitar la última coma sobrante y cerrar el arreglo JSON (compatible con macOS y Linux)
if sed --version 2>/dev/null | grep -q GNU; then
    sed -i '$ s/,$//' resultados_pthread.json
else
    sed -i '' '$ s/,$//' resultados_pthread.json
fi
echo "]" >> resultados_pthread.json

echo "Listo. Resultados en resultados_pthread.json"
