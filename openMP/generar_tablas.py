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
sec_data = load_json('resultados_sec_pc2.json')
hilos_data = load_json('resultados_openMP_pc2.json')

# ---------------------------------------------------------------------------
# Secuencial: 10 runs x 5 dimensions
# ---------------------------------------------------------------------------
dimensiones_sec = [1000, 2000, 3000, 4000, 5000]
n_runs = 10

# Group by dimension preserving order
sec_by_dim = {d: [] for d in dimensiones_sec}
for entry in sec_data:
    sec_by_dim[entry['n']].append(entry['tiempo'])

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
# Hilos: 5 thread counts x 10 runs x 5 dimensions
# ---------------------------------------------------------------------------
dimensiones_sec = [1000, 2000, 3000, 4000, 5000]
hilos_counts = [2, 4, 8, 16, 32]
n_runs = 10

# Group: hilos -> dimension -> list of times
hilos_by_count = {h: {d: [] for d in dimensiones_sec} for h in hilos_counts}
for entry in hilos_data:
    h = entry['hilos']
    d = entry['n']
    hilos_by_count[h][d].append(entry['tiempo'])

# ---------------------------------------------------------------------------
# Render a styled table matching the reference images
# ---------------------------------------------------------------------------
def render_table(table_data, row_labels, col_labels, title, filename,
                 header_color='#D5C4A1', promedio_color='#7EC8E3'):
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
        elif row == n_rows:
            # Promedio row
            cell.set_facecolor(promedio_color)
            cell.set_text_props(fontweight='bold')
        else:
            cell.set_facecolor('white')

        # Row label column
        if col == -1:
            cell.set_facecolor(header_color)
            cell.set_text_props(fontweight='bold')
            if row == n_rows:
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
# Tablas de Hilos (one per thread count)
# ---------------------------------------------------------------------------
col_labels = [str(d) for d in dimensiones_sec]

for idx, h in enumerate(hilos_counts):
    print(f'Generando tabla {h} hilos...')
    hilos_table = []
    for i in range(n_runs):
        row = [hilos_by_count[h][d][i] for d in dimensiones_sec]
        hilos_table.append(row)

    promedios_h = [np.mean(hilos_by_count[h][d]) for d in dimensiones_sec]
    hilos_table.append(promedios_h)

    row_labels = [str(i+1) for i in range(n_runs)] + ['Promedio']

    render_table(
        hilos_table, row_labels, col_labels,
        f'Tabla {idx+1}: Resultados de la ejecución con {h} hilos en pc2.',
        f'tabla_hilos_{h}.png'
    )

# ---------------------------------------------------------------------------
# Gráfico: Tiempo de ejecución para diferentes números de hilos
# ---------------------------------------------------------------------------
print('Generando gráfico de tiempo de ejecución para hilos...')

fig, ax = plt.subplots(figsize=(10, 6))

# Hilos
colors_hilos = ['#C0504D', '#9BBB59', '#7F6084', '#4BACC6', '#F79646']
for h, color in zip(hilos_counts, colors_hilos):
    promedios_h = [np.mean(hilos_by_count[h][d]) for d in dimensiones_sec]
    ax.plot(dimensiones_sec, promedios_h, marker='s', linewidth=2,
            markersize=5, label=f'{h} Hilos', color=color)

ax.set_title('Tiempo de ejecución con OpenMP', fontsize=14, fontweight='bold',
             fontfamily='serif', pad=15)
ax.set_xlabel('Dimensión de la matriz cuadrada (NxN)', fontsize=11, fontfamily='serif')
ax.set_ylabel('Tiempo de ejecución (s)', fontsize=11, fontfamily='serif')
ax.set_xticks(dimensiones_sec)
ax.set_xticklabels([str(d) for d in dimensiones_sec])
ax.grid(True, axis='y', linestyle='-', alpha=0.3)
ax.legend(loc='upper left', fontsize=9, frameon=True)

# Title above figure (italic, like reference)
fig.text(0.05, 0.98,
         'Gráfico 1. Tiempo de ejecución en función de las dimensiones de la matriz con OpenMP para PC2.',
         fontsize=11, fontstyle='italic', verticalalignment='top', fontfamily='serif')

plt.tight_layout(rect=[0, 0, 1, 0.95])
out_path = os.path.join(SCRIPT_DIR, 'grafico_openmp_tiempo.png')
plt.savefig(out_path, dpi=200, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()
print(f'  -> {out_path}')

print('\nTodas las tablas y el gráfico generados exitosamente.')
