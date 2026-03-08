#!/bin/bash

echo "[" > resultados_compilador.json

for vuelta in {1..10}
do
    for i in 1000 2000 3000 4000 5000 
    do
        # Ejecutamos y guardamos la salida en el archivo
        # ./multi_secuencial $i >> resultados.json
        ./multi_compilador $i  >> resultados_compilador.json
    done
done

# Esto es un pequeño truco para quitar la última coma sobrante 
# y cerrar el arreglo de JSON
sed -i '$ s/,$//' resultados_compilador.json
echo "]" >> resultados_compilador.json
