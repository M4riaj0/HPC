#!/bin/bash

# Configuración
MATRIZ_SIZE=2000  # Puedes cambiar este tamaño
ARCHIVO_C="multi_secuencial.c"
RESULTADOS="comparativa_compilador.json"

echo "[" > $RESULTADOS

# Lista de niveles de optimización a probar
NIVELES=("O0" "O1" "O2" "O3")

for i in "${!NIVELES[@]}"; do
    OPT=${NIVELES[$i]}
    EXE="exe_$OPT"
    
    gcc -$OPT $ARCHIVO_C -o $EXE
    
    SALIDA=$(./$EXE $MATRIZ_SIZE)
    
    # Quitar la coma final de la salida del programa de C para controlarla aquí
    LIMPIA=$(echo $SALIDA | sed 's/,$//')
    
    # Escribir en el JSON
    if [ $i -eq $(( ${#NIVELES[@]} - 1 )) ]; then
        echo "  $LIMPIA" >> $RESULTADOS  # Último elemento sin coma
    else
        echo "  $LIMPIA," >> $RESULTADOS # Elementos intermedios con coma
    fi
    
    # Limpiar el ejecutable para no llenar la carpeta
    rm $EXE
done

echo "]" >> $RESULTADOS
cat $RESULTADOS