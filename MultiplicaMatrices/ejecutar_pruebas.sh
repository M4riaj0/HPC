#!/bin/bash

# Compilar el programa en esta máquina (evita "cannot execute binary file")
echo "Compilando multi_secuencial.c..."
gcc -o multi_secuencial multi_secuencial.c -O2
if [ $? -ne 0 ]; then
    echo "Error al compilar. Abortando."
    exit 1
fi

echo "[" > resultados.json

for i in 500 1000 2000 4000 6000 8000
do
    # Ejecutamos y guardamos la salida en el archivo
    # ./multi_secuencial $i >> resultados.json
    ./multi_hilos $i >> resultados_hilos.json
done

# Quitar la última coma sobrante y cerrar el arreglo JSON (compatible con macOS y Linux)
if sed --version 2>/dev/null | grep -q GNU; then
    sed -i '$ s/,$//' resultados.json
else
    sed -i '' '$ s/,$//' resultados.json
fi
echo "]" >> resultados.json

echo "Listo. Resultados en resultados.json"
