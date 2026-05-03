#!/bin/bash

# 1. Compilación con soporte para OpenMP y optimización O2
gcc -O2 -fopenmp traffic_parallel.c -o traffic_parallel

# 2. Inicialización del archivo de salida
echo "[" > resultados_paralelo.json

# 3. Definición de parámetros
VUELTAS=5
TAMANOS="10000 40000 80000"     # Tamaños de carretera (N)
DENSIDADES="0.1 0.5 0.9"        # Densidades representativas
HILOS="1 2 4 8"                 # Configuración de núcleos/hilos

echo "Iniciando pruebas de rendimiento..."

for vuelta in $(seq 1 $VUELTAS)
do
    echo "Ejecutando vuelta $vuelta de $VUELTAS..."
    for n in $TAMANOS
    do
        for d in $DENSIDADES
        do
            for h in $HILOS
            do
                # Exportamos la variable de entorno para que OpenMP la reconozca
                export OMP_NUM_THREADS=$h
                
                # Ejecutamos y redirigimos la salida al JSON
                ./traffic_parallel $n $d >> resultados_paralelo.json
            done
        done
    done
done

# 4. Limpieza del formato JSON
# Elimina la última coma de la última línea generada por el programa
sed -i '$ s/,$//' resultados_paralelo.json
echo "]" >> resultados_paralelo.json

echo "----------------------------------------------------------"
echo "Pruebas completadas exitosamente."
echo "Resultados guardados en: resultados_paralelo.json"