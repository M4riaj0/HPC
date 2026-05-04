#!/bin/bash
# Profiling de cell_automation_openMP en macOS (arm64).
#
# En macOS arm64 no hay valgrind ni gprof reales, así que reemplazamos las
# herramientas del profiling secuencial (Linux) por sus equivalentes nativos:
#   - gprof / perf  -> sample, xctrace (Instruments)
#   - valgrind --leak-check -> leaks
#   - valgrind massif -> heap + MallocStackLogging
#   - Específico de paralelo: estudio de escalado con OMP_NUM_THREADS variable.

set -euo pipefail

cd "$(dirname "$0")"
OUT=profiling_paralelo
mkdir -p "$OUT"

# Tamaño y densidad usados para los perfiles "puntuales".
N=1000000
D=0.5
# Hilos por defecto para los perfiles puntuales (sample, leaks, heap).
THREADS_PROFILE=8
# Hilos para el estudio de escalado.
ESCALADO_HILOS="1 2 4 8 14"
# Pasos para que las corridas duren lo suficiente como para que `sample`
# capture algo útil sin ser eterno.
PASOS=1000

echo "=========================================================="
echo "1) Compilación con símbolos de depuración (-g) y -O2 -fopenmp"
echo "=========================================================="
gcc-15 -O2 -g -fopenmp cell_automation_openMP.c -o cell_automation_openMP
echo "   -> binario: ./cell_automation_openMP"

echo
echo "=========================================================="
echo "2) CPU profiling con 'sample' (equivalente macOS de gprof/perf)"
echo "   N=$N  density=$D  threads=$THREADS_PROFILE"
echo "=========================================================="
export OMP_NUM_THREADS=$THREADS_PROFILE
# Lanza el binario en background y muestrea su PID por DURACIÓN segundos.
# sample reporta el call-tree por hilo con tiempo aproximado en cada función.
./cell_automation_openMP $N $D $PASOS &
PID=$!
# Muestrea durante 10s a 1ms (10000 muestras). Si el programa termina antes,
# sample se detiene solo.
sample $PID 10 1 -file "$OUT/sample_paralelo.txt" -mayDie >/dev/null 2>&1 || true
wait $PID 2>/dev/null || true
echo "   -> $OUT/sample_paralelo.txt"

echo
echo "=========================================================="
echo "3) Detección de fugas con 'leaks' (equivalente a valgrind --leak-check)"
echo "   N=$N  density=$D  threads=$THREADS_PROFILE"
echo "=========================================================="
# MallocStackLogging hace que leaks pueda mostrar el stack de cada asignación.
# leaks --atExit ejecuta el binario y reporta fugas justo antes de salir.
MallocStackLogging=1 \
    leaks --atExit -- ./cell_automation_openMP $N $D $PASOS \
    > "$OUT/memoria_leaks_par.txt" 2>&1 || true
echo "   -> $OUT/memoria_leaks_par.txt"

echo
echo "=========================================================="
echo "4) Uso de heap con 'heap' (equivalente a valgrind --tool=massif)"
echo "   Capturamos el heap mientras el programa corre con $THREADS_PROFILE hilos"
echo "=========================================================="
export OMP_NUM_THREADS=$THREADS_PROFILE
# Aumentamos los pasos para garantizar que el proceso siga vivo cuando heap
# tome el snapshot (con -O2 paralelo, $PASOS pasos terminan en ~0.1s).
PASOS_HEAP=$((PASOS * 200))
MallocStackLogging=1 ./cell_automation_openMP $N $D $PASOS_HEAP &
PID=$!
sleep 1   # dar tiempo a que aloque road/next_road
heap $PID > "$OUT/reporte_memoria_par.txt" 2>&1 || true
# Ya tenemos el snapshot: matamos el proceso para no esperar los 200k pasos.
kill $PID 2>/dev/null || true
wait $PID 2>/dev/null || true
echo "   -> $OUT/reporte_memoria_par.txt"

echo
echo "=========================================================="
echo "5) Estudio de escalado (lo más importante para OpenMP)"
echo "   Mide tiempo con distintos OMP_NUM_THREADS y calcula speedup/eficiencia"
echo "   N=$N  density=$D  hilos=$ESCALADO_HILOS  repeticiones=3"
echo "=========================================================="
ESC="$OUT/escalado_openMP.txt"
{
    echo "Estudio de escalado fuerte para cell_automation_openMP"
    echo "N=$N  density=$D  pasos=$PASOS"
    echo "Cada tiempo es el promedio de 3 corridas (segundos, wall clock)."
    echo
    printf "%-8s %-12s %-12s %-14s\n" "Hilos" "T_avg(s)" "Speedup" "Eficiencia"
    printf "%-8s %-12s %-12s %-14s\n" "-----" "--------" "-------" "----------"
} > "$ESC"

# Calcula T(1) primero para usarlo como base del speedup.
T1=""
for h in $ESCALADO_HILOS; do
    export OMP_NUM_THREADS=$h
    SUMA=0
    for r in 1 2 3; do
        # El binario imprime una línea JSON con "time": X. Extraemos X.
        T=$(./cell_automation_openMP $N $D $PASOS \
            | sed -E 's/.*"time": *([0-9.]+).*/\1/')
        SUMA=$(awk -v a="$SUMA" -v b="$T" 'BEGIN{printf "%.6f", a+b}')
    done
    AVG=$(awk -v s="$SUMA" 'BEGIN{printf "%.6f", s/3}')
    if [ -z "$T1" ]; then T1=$AVG; fi
    SP=$(awk -v t1="$T1" -v t="$AVG" 'BEGIN{printf "%.3f", t1/t}')
    EFF=$(awk -v sp="$SP" -v h="$h" 'BEGIN{printf "%.3f", sp/h}')
    printf "%-8s %-12s %-12s %-14s\n" "$h" "$AVG" "$SP" "$EFF" >> "$ESC"
    echo "   hilos=$h  t_avg=${AVG}s  speedup=${SP}  eficiencia=${EFF}"
done
echo "   -> $ESC"

echo
echo "=========================================================="
echo "Listo. Archivos generados en: $OUT/"
echo "=========================================================="
ls -la "$OUT"
