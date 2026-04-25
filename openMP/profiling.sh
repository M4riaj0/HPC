#!/bin/bash

echo "=== PROFILING OpenMP Multiplicación de Matrices ==="
echo ""

SIZE=3000
THREADS=4

echo "1. MEMORY PROFILING con valgrind (memory leaks)"
echo "=================================================="
echo "Ejecutando: valgrind --leak-check=full ./multi_openMP $SIZE $THREADS"
echo ""
valgrind --leak-check=full --show-leak-kinds=all ./multi_openMP $SIZE $THREADS 2>&1 | tee memoria_leaks.txt

echo ""
echo "2. MEMORY PROFILING con massif (uso de memoria)"
echo "==============================================="
echo "Ejecutando: valgrind --tool=massif ./multi_openMP $SIZE $THREADS"
valgrind --tool=massif --massif-out-file=massif.out ./multi_openMP $SIZE $THREADS 2>&1
echo ""
echo "Resultados de massif guardados en massif.out"
echo ""

echo "=== Archivos generados ==="
ls -lh memoria_leaks.txt massif.out 2>/dev/null || echo "Archivos en proceso..."
