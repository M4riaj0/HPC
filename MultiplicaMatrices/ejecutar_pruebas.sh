#!/bin/bash

echo "[" > resultados.json

for i in 500 1000 2000 4000 6000 8000
do
    # Ejecutamos y guardamos la salida en el archivo
    # ./multi_secuencial $i >> resultados.json
    ./multi_hilos $i >> resultados_hilos.json
done

# Esto es un pequeño truco para quitar la última coma sobrante 
# y cerrar el arreglo de JSON
sed -i '$ s/,$//' resultados.json
echo "]" >> resultados.json
