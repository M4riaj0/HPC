# Perfilado de `cell_automation_sec`

## Herramientas utilizadas

- **Valgrind 3.22.0:** Framework de instrumentación para análisis dinámico de memoria
- **gprof:** Herramienta de profiling de CPU incluida en el toolchain de GCC
- **GCC:** Compilador con soporte para instrumentación de profiling (`-pg`)

## Instalación

```bash
sudo apt install valgrind
```

> **Nota:** `perf` no es compatible con el kernel de WSL2 (`5.15.x-microsoft`), por lo que se utilizó `gprof` como alternativa para el profiling de CPU.

---

## Compilación

```bash
# Compilar con símbolos de depuración e instrumentación de profiling
gcc -O2 -g -pg cell_automation_sec.c -o cell_automation_sec
```

| Flag | Descripción |
|------|-------------|
| `-O2` | Optimización de nivel 2 |
| `-g`  | Incluir símbolos de depuración |
| `-pg` | Instrumentación para profiling con gprof |

---

## Análisis de memoria con Valgrind

### 1. Detección de fugas de memoria

```bash
valgrind --leak-check=full --log-file=memoria_leaks_sec.txt ./cell_automation_sec 100000 0.5
```

Genera el archivo `memoria_leaks_sec.txt` con el reporte completo de fugas.

### 2. Profiling del heap (uso de memoria dinámica)

```bash
valgrind --tool=massif --massif-out-file=massif_sec.out ./cell_automation_sec 100000 0.5
ms_print massif_sec.out > reporte_memoria_sec.txt
```

Genera el archivo `reporte_memoria_sec.txt` con el uso del heap a lo largo del tiempo.

---

## Profiling de CPU con gprof

```bash
# Ejecutar el programa (genera gmon.out automáticamente)
./cell_automation_sec 500000 0.5 5000

# Generar reporte legible
gprof cell_automation_sec gmon.out > perf_cpu_sec.txt
```

Genera el archivo `perf_cpu_sec.txt` con el tiempo de ejecución por función y el call graph.

---

## Archivos generados

| Archivo                   | Herramienta               | Contenido |
|-------------------------  |----------                 |-----------------------------|
| `memoria_leaks_sec.txt`   | Valgrind                  | Reporte de fugas de memoria |
| `massif_sec.out`          | Valgrind Massif           | Datos crudos de uso del heap|
| `reporte_memoria_sec.txt` | ms_print                  | Reporte legible del heap |
| `gmon.out`                | gprof                     | Datos crudos de profiling de CPU |
| `perf_cpu_sec.txt`        | gprof                     | Reporte de tiempos por función |