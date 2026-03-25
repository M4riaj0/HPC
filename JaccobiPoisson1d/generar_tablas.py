import json
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import os

matplotlib.rcParams['font.family'] = 'serif'

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

def load_json(filename):
    with open(os.path.join(SCRIPT_DIR, filename), 'r') as f:
        return json.load(f)

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------
sec_data = load_json('resultados_sec.json')
hilos_data = load_json('resultados_hilos.json')
procesos_data = load_json('resultados_procesos.json')
compilador_data = load_json('resultados_compilador.json')

# ---------------------------------------------------------------------------
# Type selector (optional arg)
# ---------------------------------------------------------------------------
import sys
valid_types = {'all', 'secuencial', 'hilos', 'procesos', 'compilador', 'memoria'}
selected_types = set(sys.argv[1:]) if len(sys.argv) > 1 else {'all'}
if not selected_types.issubset(valid_types):
    print('Uso: python generar_tablas.py [all|secuencial|hilos|procesos|compilador|memoria]')
    sys.exit(1)

# ---------------------------------------------------------------------------
# Dimension mapping
# ---------------------------------------------------------------------------
dimensiones_sec = [7, 8, 9, 10, 11]
n_runs = 10
exp_to_dim = {7: 7, 8: 8, 9: 9, 10: 10, 11: 11}

def dim_from_exponente(k):
    return exp_to_dim.get(k, (k - 6) * 1000 if isinstance(k, int) else None)


def should_generate(tipo):
    return 'all' in selected_types or tipo in selected_types

# ---------------------------------------------------------------------------
# Secuencial: 10 runs x 5 dimensiones
# ---------------------------------------------------------------------------
sec_by_dim = {d: [] for d in dimensiones_sec}
for entry in sec_data:
    d = dim_from_exponente(entry.get('exponente_k'))
    if d is None:
        continue
    sec_by_dim[d].append(entry.get('tiempo_ejecucion_s'))


# Build table data: rows = runs 1..10 + Promedio
sec_table = []
for i in range(n_runs):
    row = [sec_by_dim[d][i] for d in dimensiones_sec]
    sec_table.append(row)
sec_promedios = [np.mean(sec_by_dim[d]) for d in dimensiones_sec]
sec_table.append(sec_promedios)

row_labels_sec = [str(i+1) for i in range(n_runs)] + ['Promedio']
col_labels_sec = [str(d) for d in dimensiones_sec]

# ---------------------------------------------------------------------------
# Hilos: 5 thread counts x 10 runs x 5 dimensiones
# ---------------------------------------------------------------------------
hilos_counts = [2, 4, 8, 16]

# Group: hilos -> dimension -> list of times
hilos_by_count = {h: {d: [] for d in dimensiones_sec} for h in hilos_counts}
for entry in hilos_data:
    h = entry.get('num_hilos')
    d = dim_from_exponente(entry.get('exponente_k'))
    t = entry.get('tiempo_ejecucion_s')
    if h is None or d is None or t is None:
        continue
    if h not in hilos_by_count:
        hilos_by_count[h] = {d2: [] for d2 in dimensiones_sec}
    hilos_by_count[h][d].append(t)


# ---------------------------------------------------------------------------
# Render a styled table matching the reference images
# ---------------------------------------------------------------------------
def render_table(table_data, row_labels, col_labels, title, filename,
                 header_color='#D5C4A1', promedio_color='#7EC8E3',
                 speedup_color='#7EC8E3', has_speedup=False):
    n_rows = len(table_data)
    n_cols = len(col_labels)

    fig_width = 1.8 + n_cols * 1.6
    fig_height = 0.9 + n_rows * 0.45

    fig, ax = plt.subplots(figsize=(fig_width, fig_height))
    ax.axis('off')

    # Title (italic, left-aligned)
    fig.text(0.05, 0.97, title, fontsize=11, fontstyle='italic',
             verticalalignment='top', fontfamily='serif')

    # Format cell text
    cell_text = []
    for i, row in enumerate(table_data):
        formatted = []
        for v in row:
            if isinstance(v, str):
                formatted.append(v)
            elif abs(v) < 10:
                formatted.append(f'{v:.6f}')
            elif abs(v) < 100:
                formatted.append(f'{v:.4f}')
            else:
                formatted.append(f'{v:.3f}')

        cell_text.append(formatted)

    table = ax.table(
        cellText=cell_text,
        rowLabels=row_labels,
        colLabels=col_labels,
        loc='center',
        cellLoc='center',
    )

    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1.0, 1.4)

    # Style cells
    for (row, col), cell in table.get_celld().items():
        cell.set_edgecolor('#999999')
        cell.set_linewidth(0.5)

        if row == 0:
            # Header row
            cell.set_facecolor(header_color)
            cell.set_text_props(fontweight='bold')
        elif row == n_rows and not has_speedup:
            # Promedio row (last row when no speedup)
            cell.set_facecolor(promedio_color)
            cell.set_text_props(fontweight='bold')
        elif has_speedup and row == n_rows - 1:
            # Promedio row (second to last when speedup exists)
            cell.set_facecolor(promedio_color)
            cell.set_text_props(fontweight='bold')
        elif has_speedup and row == n_rows:
            # Speedup row
            cell.set_facecolor(speedup_color)
            cell.set_text_props(fontweight='bold')
        else:
            cell.set_facecolor('white')

        # Row label column
        if col == -1:
            cell.set_facecolor(header_color)
            cell.set_text_props(fontweight='bold')
            if has_speedup and row == n_rows:
                cell.set_facecolor(speedup_color)
            elif (has_speedup and row == n_rows - 1) or (not has_speedup and row == n_rows):
                cell.set_facecolor(promedio_color)

    # Footer
    fig.text(0.05, 0.02, 'Fuente: Elaboración propia.', fontsize=9,
             fontstyle='italic', fontfamily='serif')

    plt.tight_layout(rect=[0, 0.04, 1, 0.95])
    out_path = os.path.join(SCRIPT_DIR, filename)
    plt.savefig(out_path, dpi=200, bbox_inches='tight',
                facecolor='white', edgecolor='none')
    plt.close()
    print(f'  -> {out_path}')

# ---------------------------------------------------------------------------
# 1) Tabla Secuencial
# ---------------------------------------------------------------------------
if should_generate('secuencial'):
    print('Generando tabla secuencial...')
    render_table(
        sec_table, row_labels_sec, col_labels_sec,
        'Tabla 1: Resultados de la ejecución secuencial.',
        'tabla_secuencial.png'
    )

# ---------------------------------------------------------------------------
# 2) Tablas de Hilos (one per thread count)
# ---------------------------------------------------------------------------
if should_generate('hilos'):
    for idx, h in enumerate(hilos_counts):
        print(f'Generando tabla {h} hilos...')
        hilos_table = []
        for i in range(n_runs):
            row = [hilos_by_count[h][d][i] for d in dimensiones_sec]
            hilos_table.append(row)

        promedios_h = [np.mean(hilos_by_count[h][d]) for d in dimensiones_sec]
        speedups = [sec_promedios[j] / promedios_h[j] for j in range(len(dimensiones_sec))]

        hilos_table.append(promedios_h)
        hilos_table.append(speedups)

        row_labels_h = [str(i+1) for i in range(n_runs)] + ['Promedio', 'Speedup']
        col_labels_h = col_labels_sec

        render_table(
            hilos_table, row_labels_h, col_labels_h,
            f'Tabla {idx+2}: Resultados de la ejecución con {h} hilos.',
            f'tabla_hilos_{h}.png',
            has_speedup=True
        )

# ---------------------------------------------------------------------------
# 3) Tablas de Procesos (one per process count)
# ---------------------------------------------------------------------------
procesos_data = load_json('resultados_procesos.json')
procesos_counts = [2, 4, 8, 16, 32]

procesos_by_count = {p: {d: [] for d in dimensiones_sec} for p in procesos_counts}
for entry in procesos_data:
    p = entry.get('num_procesos')
    d = dim_from_exponente(entry.get('exponente_k'))
    t = entry.get('tiempo_ejecucion_s')
    if p is None or d is None or t is None:
        continue
    if p not in procesos_by_count:
        procesos_by_count[p] = {d2: [] for d2 in dimensiones_sec}
    procesos_by_count[p][d].append(t)

if should_generate('procesos'):
    for idx, p in enumerate(procesos_counts):
        print(f'Generando tabla {p} procesos...')
        proc_table = []
        for i in range(n_runs):
            row = [procesos_by_count[p][d][i] for d in dimensiones_sec]
            proc_table.append(row)

        promedios_p = [np.mean(procesos_by_count[p][d]) for d in dimensiones_sec]
        speedups = [sec_promedios[j] / promedios_p[j] for j in range(len(dimensiones_sec))]

        proc_table.append(promedios_p)
        proc_table.append(speedups)

        row_labels_p = [str(i+1) for i in range(n_runs)] + ['Promedio', 'Speedup']

        render_table(
            proc_table, row_labels_p, col_labels_sec,
            f'Tabla {idx+7}: Resultados de la ejecución con {p} procesos.',
            f'tabla_procesos_{p}.png',
            has_speedup=True
        )

# ---------------------------------------------------------------------------
# 3b) Tabla Compilador (optimización con flags del compilador)
# ---------------------------------------------------------------------------
compilador_data = load_json('resultados_compilador.json')

compilador_by_dim = {d: [] for d in dimensiones_sec}
for entry in compilador_data:
    d = dim_from_exponente(entry.get('exponente_k'))
    t = entry.get('tiempo_ejecucion_s')
    if d is None or t is None:
        continue
    compilador_by_dim[d].append(t)

if should_generate('compilador'):
    print('Generando tabla compilador...')
    compilador_table = []
    for i in range(n_runs):
        row = [compilador_by_dim[d][i] for d in dimensiones_sec]
        compilador_table.append(row)

    compilador_promedios = [np.mean(compilador_by_dim[d]) for d in dimensiones_sec]
    compilador_speedups = [sec_promedios[j] / compilador_promedios[j] for j in range(len(dimensiones_sec))]

    compilador_table.append(compilador_promedios)
    compilador_table.append(compilador_speedups)

    row_labels_compilador = [str(i+1) for i in range(n_runs)] + ['Promedio', 'Speedup']

    render_table(
        compilador_table, row_labels_compilador, col_labels_sec,
        'Tabla 12: Resultados de la ejecución con optimización del compilador.',
        'tabla_compilador.png',
        has_speedup=True
    )

# ---------------------------------------------------------------------------
# 3c) Tabla Memoria Compartida
# ---------------------------------------------------------------------------
memoria_data = load_json('resultados_memoria.json')

memoria_by_dim = {d: [] for d in dimensiones_sec}
for entry in memoria_data:
    d = dim_from_exponente(entry.get('exponente_k'))
    t = entry.get('tiempo_ejecucion_s')
    if d is None or t is None:
        continue
    memoria_by_dim[d].append(t)

if should_generate('memoria'):
    print('Generando tabla memoria compartida...')
    memoria_table = []
    for i in range(n_runs):
        row = [memoria_by_dim[d][i] for d in dimensiones_sec]
        memoria_table.append(row)

    memoria_promedios = [np.mean(memoria_by_dim[d]) for d in dimensiones_sec]
    memoria_speedups = [sec_promedios[j] / memoria_promedios[j] for j in range(len(dimensiones_sec))]

    memoria_table.append(memoria_promedios)
    memoria_table.append(memoria_speedups)

    row_labels_memoria = [str(i+1) for i in range(n_runs)] + ['Promedio', 'Speedup']

    render_table(
        memoria_table, row_labels_memoria, col_labels_sec,
        'Tabla 13: Resultados de la ejecución con memoria compartida.',
        'tabla_memoria.png',
        has_speedup=True
    )

# ---------------------------------------------------------------------------
# 4) Gráfico: Secuencial vs Hilos (promedios)
# ---------------------------------------------------------------------------
if should_generate('secuencial') or should_generate('hilos'):
    print('Generando gráfico secuencial vs hilos...')

    fig, ax = plt.subplots(figsize=(10, 6))

    # Secuencial
    ax.plot(dimensiones_sec, sec_promedios, marker='s', linewidth=2.5,
            markersize=6, label='Secuencial', color='#4472C4')

# Hilos
colors_hilos = ['#C0504D', '#9BBB59', '#7F6084', '#4BACC6', '#F79646']
for h, color in zip(hilos_counts, colors_hilos):
    promedios_h = [np.mean(hilos_by_count[h][d]) for d in dimensiones_sec]
    ax.plot(dimensiones_sec, promedios_h, marker='s', linewidth=2,
            markersize=5, label=f'{h} Hilos', color=color)

ax.set_title('Ejecución secuencial vs concurrente', fontsize=14, fontweight='bold',
             fontfamily='serif', pad=15)
ax.set_xlabel('Dimensión de la matriz cuadrada (NxN)', fontsize=11, fontfamily='serif')
ax.set_ylabel('Tiempo de ejecución (s)', fontsize=11, fontfamily='serif')
ax.set_xticks(dimensiones_sec)
ax.set_xticklabels([str(d) for d in dimensiones_sec])
ax.grid(True, axis='y', linestyle='-', alpha=0.3)
ax.legend(loc='upper left', fontsize=9, frameon=True)

# Title above figure (italic, like reference)
fig.text(0.05, 0.98,
         'Gráfico 1. Tiempo de ejecución en función de las dimensiones de la matriz.',
         fontsize=11, fontstyle='italic', verticalalignment='top', fontfamily='serif')

plt.tight_layout(rect=[0, 0, 1, 0.95])
out_path = os.path.join(SCRIPT_DIR, 'grafico_sec_vs_hilos.png')
plt.savefig(out_path, dpi=200, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()
print(f'  -> {out_path}')

# ---------------------------------------------------------------------------
# 5) Gráfico: Speedup de Hilos en función de las dimensiones
# ---------------------------------------------------------------------------
if should_generate('hilos'):
    print('Generando gráfico speedup hilos...')

    fig, ax = plt.subplots(figsize=(10, 6))

colors_speedup = ['#4472C4', '#C0504D', '#A5A5A5', '#FFC000', '#4472C4']
markers_speedup = ['o', 'o', 'o', 'o', 'o']

for h, color in zip(hilos_counts, colors_speedup):
    promedios_h = [np.mean(hilos_by_count[h][d]) for d in dimensiones_sec]
    speedups_h = [sec_promedios[j] / promedios_h[j] for j in range(len(dimensiones_sec))]
    ax.plot(dimensiones_sec, speedups_h, marker='o', linewidth=2,
            markersize=6, label=f'{h} Hilos', color=color)

ax.set_xlabel('Dimensión de la matriz cuadrada (NxN)', fontsize=11, fontfamily='serif')
ax.set_ylabel('Speedup', fontsize=11, fontfamily='serif')
ax.set_xticks(dimensiones_sec)
ax.set_xticklabels([str(d) for d in dimensiones_sec])
ax.grid(True, linestyle='-', alpha=0.3)
ax.legend(loc='lower right', fontsize=9, frameon=True)

fig.text(0.05, 0.98,
         'Gráfico 2. Valor de Speed Up en función de las dimensiones de la matriz.',
         fontsize=11, fontstyle='italic', verticalalignment='top', fontfamily='serif')

plt.tight_layout(rect=[0, 0, 1, 0.95])
out_path = os.path.join(SCRIPT_DIR, 'grafico_speedup_hilos.png')
plt.savefig(out_path, dpi=200, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()
print(f'  -> {out_path}')

# ---------------------------------------------------------------------------
# 6) Gráfico: Speedup de Procesos en función de las dimensiones
# ---------------------------------------------------------------------------
if should_generate('procesos'):
    print('Generando gráfico speedup procesos...')

    fig, ax = plt.subplots(figsize=(10, 6))

colors_proc = ['#4472C4', '#C0504D', '#A5A5A5', '#FFC000', '#4472C4']

for p, color in zip(procesos_counts, colors_proc):
    promedios_p = [np.mean(procesos_by_count[p][d]) for d in dimensiones_sec]
    speedups_p = [sec_promedios[j] / promedios_p[j] for j in range(len(dimensiones_sec))]
    ax.plot(dimensiones_sec, speedups_p, marker='o', linewidth=2,
            markersize=6, label=f'{p} Procesos', color=color)

ax.set_xlabel('Dimensión de la matriz cuadrada (NxN)', fontsize=11, fontfamily='serif')
ax.set_ylabel('Speedup', fontsize=11, fontfamily='serif')
ax.set_xticks(dimensiones_sec)
ax.set_xticklabels([str(d) for d in dimensiones_sec])
ax.grid(True, linestyle='-', alpha=0.3)
ax.legend(loc='lower right', fontsize=9, frameon=True)

fig.text(0.05, 0.98,
         'Gráfico 3. Valor de Speed Up de procesos en función de las dimensiones de la matriz.',
         fontsize=11, fontstyle='italic', verticalalignment='top', fontfamily='serif')

plt.tight_layout(rect=[0, 0, 1, 0.95])
out_path = os.path.join(SCRIPT_DIR, 'grafico_speedup_procesos.png')
plt.savefig(out_path, dpi=200, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()
print(f'  -> {out_path}')

# ---------------------------------------------------------------------------
# 7) Gráfico: Comparación de mejores speedups (hilos, procesos, compilador, memoria)
# ---------------------------------------------------------------------------
if should_generate('hilos') or should_generate('procesos') or should_generate('compilador') or should_generate('memoria'):
    print('Generando gráfico comparativo de mejores speedups...')

    fig, ax = plt.subplots(figsize=(10, 6))

# Best 2 hilos: 4 and 8
best_hilos = [4, 8]
colors_best = ['#C0504D', '#A5A5A5', '#9BBB59', '#FFC000', '#7F6084', '#4BACC6']

for h, color in zip(best_hilos, colors_best[:2]):
    promedios_h = [np.mean(hilos_by_count[h][d]) for d in dimensiones_sec]
    speedups_h = [sec_promedios[j] / promedios_h[j] for j in range(len(dimensiones_sec))]
    ax.plot(dimensiones_sec, speedups_h, marker='o', linewidth=2,
            markersize=6, label=f'{h} Hilos', color=color)

# Best 2 procesos: 4 and 16
best_procesos = [4, 16]
for p, color in zip(best_procesos, colors_best[2:4]):
    promedios_p = [np.mean(procesos_by_count[p][d]) for d in dimensiones_sec]
    speedups_p = [sec_promedios[j] / promedios_p[j] for j in range(len(dimensiones_sec))]
    ax.plot(dimensiones_sec, speedups_p, marker='s', linewidth=2,
            markersize=6, label=f'{p} Procesos', color=color)

# Compilador
compilador_speedups_plot = [sec_promedios[j] / compilador_promedios[j] for j in range(len(dimensiones_sec))]
ax.plot(dimensiones_sec, compilador_speedups_plot, marker='^', linewidth=2,
        markersize=7, label='Compilador', color=colors_best[4])

# Memoria compartida
memoria_speedups_plot = [sec_promedios[j] / memoria_promedios[j] for j in range(len(dimensiones_sec))]
ax.plot(dimensiones_sec, memoria_speedups_plot, marker='D', linewidth=2,
        markersize=6, label='Memoria compartida', color=colors_best[5])

ax.set_xlabel('Dimensión de la matriz cuadrada (NxN)', fontsize=11, fontfamily='serif')
ax.set_ylabel('Speedup', fontsize=11, fontfamily='serif')
ax.set_xticks(dimensiones_sec)
ax.set_xticklabels([str(d) for d in dimensiones_sec])
ax.grid(True, linestyle='-', alpha=0.3)
ax.legend(loc='best', fontsize=9, frameon=True)

fig.text(0.05, 0.98,
         'Gráfico 4. Comparación de Speed Up: mejores hilos, procesos, compilador y memoria.',
         fontsize=11, fontstyle='italic', verticalalignment='top', fontfamily='serif')

plt.tight_layout(rect=[0, 0, 1, 0.95])
out_path = os.path.join(SCRIPT_DIR, 'grafico_speedup_comparativo.png')
plt.savefig(out_path, dpi=200, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()
print(f'  -> {out_path}')

print('\nTodas las tablas y gráficos generados exitosamente.')