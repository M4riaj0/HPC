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

    fig.text(0.05, 0.97, title, fontsize=11, fontstyle='italic',
             verticalalignment='top', fontfamily='serif')

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

    for (row, col), cell in table.get_celld().items():
        cell.set_edgecolor('#999999')
        cell.set_linewidth(0.5)

        if row == 0:
            cell.set_facecolor(header_color)
            cell.set_text_props(fontweight='bold')
        elif row == n_rows and not has_speedup:
            cell.set_facecolor(promedio_color)
            cell.set_text_props(fontweight='bold')
        elif has_speedup and row == n_rows - 1:
            cell.set_facecolor(promedio_color)
            cell.set_text_props(fontweight='bold')
        elif has_speedup and row == n_rows:
            cell.set_facecolor(speedup_color)
            cell.set_text_props(fontweight='bold')
        else:
            cell.set_facecolor('white')

        if col == -1:
            cell.set_facecolor(header_color)
            cell.set_text_props(fontweight='bold')
            if has_speedup and row == n_rows:
                cell.set_facecolor(speedup_color)
            elif (has_speedup and row == n_rows - 1) or (not has_speedup and row == n_rows):
                cell.set_facecolor(promedio_color)

    fig.text(0.05, 0.02, 'Fuente: Elaboracion propia.', fontsize=9,
             fontstyle='italic', fontfamily='serif')

    plt.tight_layout(rect=[0, 0.04, 1, 0.95])
    out_path = os.path.join(SCRIPT_DIR, filename)
    plt.savefig(out_path, dpi=200, bbox_inches='tight',
                facecolor='white', edgecolor='none')
    plt.close()
    print(f'  -> {out_path}')

# ---------------------------------------------------------------------------
# Configuracion: dimensiones y conteos
# k_values: exponentes de malla (k=11 ignorado segun instrucciones)
# Las columnas de la tabla muestran NK = 2^k + 1 para cada k
# ---------------------------------------------------------------------------
k_values    = [7, 8, 9, 10]          # k=11 excluido
hilos_counts = [2, 4, 8, 16]
n_runs      = 10

# Etiquetas de columna: mostrar NK en vez de k para mayor claridad
col_labels_hilos = [f'k={k}\nNK={2**k+1}' for k in k_values]

# ---------------------------------------------------------------------------
# Secuencial: necesario para calcular speedup
# Formato esperado: {"exponente_k": 7, "num_procesos": 1, "tiempo_ejecucion_s": 0.14}
# ---------------------------------------------------------------------------
sec_data = load_json('resultados_sec.json')

sec_by_k = {k: [] for k in k_values}
for entry in sec_data:
    k = entry['exponente_k']
    if k in k_values:
        sec_by_k[k].append(entry['tiempo_ejecucion_s'])

sec_promedios = [np.mean(sec_by_k[k]) for k in k_values]

# ---------------------------------------------------------------------------
# Hilos: formato {"exponente_k": 7, "num_hilos": 2, "tiempo_ejecucion_s": 0.09}
# k=11 se ignora automaticamente al filtrar por k_values
# ---------------------------------------------------------------------------
hilos_data = load_json('resultados_hilos.json')

hilos_by_count = {h: {k: [] for k in k_values} for h in hilos_counts}
for entry in hilos_data:
    h = entry['num_hilos']
    k = entry['exponente_k']
    if k in k_values and h in hilos_counts:
        hilos_by_count[h][k].append(entry['tiempo_ejecucion_s'])

# ---------------------------------------------------------------------------
# Generar tablas: una por cantidad de hilos
# ---------------------------------------------------------------------------
for idx, h in enumerate(hilos_counts):
    print(f'Generando tabla {h} hilos...')

    hilos_table = []
    for i in range(n_runs):
        row = [hilos_by_count[h][k][i] for k in k_values]
        hilos_table.append(row)

    promedios_h = [np.mean(hilos_by_count[h][k]) for k in k_values]
    speedups    = [sec_promedios[j] / promedios_h[j] for j in range(len(k_values))]

    hilos_table.append(promedios_h)
    hilos_table.append(speedups)

    row_labels_h = [str(i + 1) for i in range(n_runs)] + ['Promedio', 'Speedup']

    render_table(
        hilos_table, row_labels_h, col_labels_hilos,
        f'Tabla {idx + 2}: Resultados de la ejecucion con {h} hilos.',
        f'tabla_hilos_{h}.png',
        has_speedup=True
    )

# ---------------------------------------------------------------------------
# Grafico: Secuencial vs Hilos (promedios por k)
# ---------------------------------------------------------------------------
print('Generando grafico secuencial vs hilos...')

fig, ax = plt.subplots(figsize=(10, 6))

nk_labels = [2**k + 1 for k in k_values]

ax.plot(nk_labels, sec_promedios, marker='s', linewidth=2.5,
        markersize=6, label='Secuencial', color='#4472C4')

colors_hilos = ['#C0504D', '#9BBB59', '#7F6084', '#4BACC6']
for h, color in zip(hilos_counts, colors_hilos):
    promedios_h = [np.mean(hilos_by_count[h][k]) for k in k_values]
    ax.plot(nk_labels, promedios_h, marker='s', linewidth=2,
            markersize=5, label=f'{h} Hilos', color=color)

ax.set_title('Ejecucion secuencial vs paralela (hilos)', fontsize=14,
             fontweight='bold', fontfamily='serif', pad=15)
ax.set_xlabel('Numero de nodos NK (2^k + 1)', fontsize=11, fontfamily='serif')
ax.set_ylabel('Tiempo de ejecucion (s)', fontsize=11, fontfamily='serif')
ax.set_xticks(nk_labels)
ax.set_xticklabels([str(n) for n in nk_labels])
ax.grid(True, axis='y', linestyle='-', alpha=0.3)
ax.legend(loc='upper left', fontsize=9, frameon=True)

fig.text(0.05, 0.98,
         'Grafico 1. Tiempo de ejecucion en funcion del numero de nodos.',
         fontsize=11, fontstyle='italic', verticalalignment='top', fontfamily='serif')

plt.tight_layout(rect=[0, 0, 1, 0.95])
out_path = os.path.join(SCRIPT_DIR, 'grafico_sec_vs_hilos.png')
plt.savefig(out_path, dpi=200, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()
print(f'  -> {out_path}')

# ---------------------------------------------------------------------------
# Grafico: Speedup de Hilos
# ---------------------------------------------------------------------------
print('Generando grafico speedup hilos...')

fig, ax = plt.subplots(figsize=(10, 6))

colors_speedup = ['#4472C4', '#C0504D', '#A5A5A5', '#FFC000']
for h, color in zip(hilos_counts, colors_speedup):
    promedios_h = [np.mean(hilos_by_count[h][k]) for k in k_values]
    speedups_h  = [sec_promedios[j] / promedios_h[j] for j in range(len(k_values))]
    ax.plot(nk_labels, speedups_h, marker='o', linewidth=2,
            markersize=6, label=f'{h} Hilos', color=color)

ax.set_xlabel('Numero de nodos NK (2^k + 1)', fontsize=11, fontfamily='serif')
ax.set_ylabel('Speedup', fontsize=11, fontfamily='serif')
ax.set_xticks(nk_labels)
ax.set_xticklabels([str(n) for n in nk_labels])
ax.grid(True, linestyle='-', alpha=0.3)
ax.legend(loc='best', fontsize=9, frameon=True)

fig.text(0.05, 0.98,
         'Grafico 2. Speedup de hilos en funcion del numero de nodos.',
         fontsize=11, fontstyle='italic', verticalalignment='top', fontfamily='serif')

plt.tight_layout(rect=[0, 0, 1, 0.95])
out_path = os.path.join(SCRIPT_DIR, 'grafico_speedup_hilos.png')
plt.savefig(out_path, dpi=200, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()
print(f'  -> {out_path}')

print('\nTablas y graficos de hilos generados exitosamente.')