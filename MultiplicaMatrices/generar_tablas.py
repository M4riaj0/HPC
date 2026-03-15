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
hilos_counts = [2, 4, 8, 16, 32]

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
print('Generando tabla secuencial...')
render_table(
    sec_table, row_labels_sec, col_labels_sec,
    'Tabla 1: Resultados de la ejecución secuencial.',
    'tabla_secuencial.png'
)

# ---------------------------------------------------------------------------
# 2) Tablas de Hilos (one per thread count)
# ---------------------------------------------------------------------------
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
    p = entry['procesos']
    d = entry['n']
    procesos_by_count[p][d].append(entry['tiempo'])

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

print('\nTodas las tablas generadas exitosamente.')
