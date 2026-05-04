# Perfilado de `cell_automation_openMP` (versión paralela con OpenMP)

## Contexto: macOS arm64

El profiling secuencial del reto se hizo en Linux/WSL2 con `valgrind` y `gprof`.
En macOS arm64 (Apple Silicon) ninguna de esas dos herramientas está disponible:

- `valgrind` no soporta macOS arm64.
- `gprof` fue descontinuado por Apple; el `gcc-15` de Homebrew tampoco lo trae.
- `perf` solo existe en Linux.

Por eso se usaron las equivalencias nativas de macOS.

## Equivalencias Linux → macOS

| Linux                              | macOS arm64                          | Para qué sirve                          |
|------------------------------------|--------------------------------------|------------------------------------------|
| `gprof` / `perf`                   | `sample`                             | CPU profiling por función / call tree    |
| `valgrind --leak-check=full`       | `leaks --atExit`                     | Detección de fugas de memoria            |
| `valgrind --tool=massif` + `ms_print` | `heap` + `MallocStackLogging=1`   | Uso del heap a lo largo del tiempo       |
| —                                  | `xctrace record` (Instruments)       | Profiling más detallado (alternativa)    |

## Herramientas utilizadas

- **GCC 15 (Homebrew):** compilador con soporte real de OpenMP (`brew install gcc`).
  Apple clang sí soporta OpenMP, pero requiere `libomp` por separado.
- **`sample`:** muestreador de stack incluido en macOS (`/usr/bin/sample`).
- **`leaks`:** detector de fugas incluido en macOS (`/usr/bin/leaks`).
- **`heap`:** snapshot del heap de un proceso vivo (`/usr/bin/heap`).
- **`MallocStackLogging`:** variable de entorno que habilita el stack de cada
  malloc para que `leaks` y `heap` puedan reportarlo.

---

## Compilación

```bash
gcc-15 -O2 -g -fopenmp cell_automation_openMP.c -o cell_automation_openMP
```

| Flag        | Descripción                                              |
|-------------|----------------------------------------------------------|
| `-O2`       | Optimización nivel 2                                     |
| `-g`        | Símbolos de depuración (necesarios para que `sample` muestre nombres de función) |
| `-fopenmp`  | Habilita OpenMP en `gcc-15`                              |

> En macOS, `gcc` apunta a Apple clang y *no* compila `-fopenmp` directamente.
> Por eso se usa `gcc-15` instalado vía Homebrew.

---

## CPU profiling con `sample`

```bash
export OMP_NUM_THREADS=8
./cell_automation_openMP 1000000 0.5 1000 &
PID=$!
sample $PID 10 1 -file sample_paralelo.txt -mayDie
wait $PID
```

`sample $PID 10 1` muestrea el proceso `$PID` durante 10 segundos cada 1 ms.
El reporte resultante (`sample_paralelo.txt`) muestra el árbol de llamadas por
hilo con el tiempo aproximado gastado en cada función — el equivalente práctico
del reporte plano + call graph de `gprof`.

---

## Fugas de memoria con `leaks`

```bash
MallocStackLogging=1 leaks --atExit -- \
    ./cell_automation_openMP 1000000 0.5 1000 > memoria_leaks_par.txt
```

`leaks --atExit` ejecuta el binario y, justo antes de que salga, escanea el
heap buscando bloques inalcanzables. Equivale a
`valgrind --leak-check=full`.

---

## Uso de heap con `heap`

```bash
export OMP_NUM_THREADS=8
MallocStackLogging=1 ./cell_automation_openMP 1000000 0.5 1000 &
PID=$!
sleep 1                    # dejar que aloque road / next_road
heap $PID > reporte_memoria_par.txt
wait $PID
```

`heap` toma un snapshot del estado del heap del proceso vivo: total alocado,
número de bloques, distribución por tamaño y, gracias a `MallocStackLogging`,
quién pidió la memoria. Reemplaza al par massif/ms_print.

---

## Estudio de escalado (específico de OpenMP)

Esta parte no tiene equivalente en el secuencial: es lo que justifica haber
paralelizado. Se mide el tiempo wall-clock con distintos valores de
`OMP_NUM_THREADS` y se calcula:

- **Speedup**  S(p) = T(1) / T(p)
- **Eficiencia**  E(p) = S(p) / p

```bash
for h in 1 2 4 8 14; do
    export OMP_NUM_THREADS=$h
    ./cell_automation_openMP 1000000 0.5 1000
done
```

El script `profile_openMP.sh` automatiza esto promediando 3 corridas por
configuración y deja el resultado en `escalado_openMP.txt`.

---

## Cómo ejecutar todo

```bash
chmod +x profile_openMP.sh
./profile_openMP.sh
```

## Archivos generados

| Archivo                      | Herramienta             | Contenido                                  |
|------------------------------|-------------------------|--------------------------------------------|
| `sample_paralelo.txt`        | `sample`                | Call tree por hilo con tiempos             |
| `memoria_leaks_par.txt`      | `leaks`                 | Reporte de fugas de memoria                |
| `reporte_memoria_par.txt`    | `heap`                  | Snapshot del heap del proceso              |
| `escalado_openMP.txt`        | binario + bash          | Tabla hilos / tiempo / speedup / eficiencia|
