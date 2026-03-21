#!/bin/bash

echo "[" > resultados_compilador.json

for vuelta in {1..10}
do
    for i in  7 8 9 10 11 
    do
        # Ejecutamos y guardamos la salida en el archivo
        # ./multi_secuencial $i >> resultados.json
        ./jacobi_poisson_compilador $i  >> resultados_compilador.json
    done
done

# Esto es un pequeño truco para quitar la última coma sobrante 
# y cerrar el arreglo de JSON
sed -i '$ s/,$//' resultados_compilador.json
echo "]" >> resultados_compilador.json