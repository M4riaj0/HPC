#!/bin/bash

echo "--- INICIANDO EXPERIMENTO COMPLETO ---"

# Llamamos a cada script uno por uno
bash ejecutar_sec.sh
bash ejecutar_memoria.sh
bash ejecutar_compilador.sh
bash ejecutar_hilos.sh
bash ejecutar_procesos.sh

echo "--- TODOS LOS EXPERIMENTOS HAN TERMINADO ---"
echo "Revisa tus archivos .json para ver los resultados."