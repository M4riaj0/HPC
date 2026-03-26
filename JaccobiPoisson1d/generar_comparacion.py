import json
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import os

matplotlib.rcParams['font.family'] = 'serif'
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_json(filename):
    with open(os.path.join(SCRIPT_DIR, filename), 'r') as f:
        return json.load(f)

def render_table(table_data, row_labels, col_labels, title, filename,
                 header_color='#D5C4A1', promedio_color='#7EC8E3',
                 speedup_color='#7EC8E3', has_speedup=False):
    n_rows = len(table_data)
    n_cols = len(col_labels)
    fig_width = 1.8 + n_cols * 1.9
    fig_height = 0.9 + n_rows * 0.45
    fig, ax = plt.subplots(figsize=(fig_width, fig_height))
    ax.axis('off')
    fig.text(0.05, 0.97, title, fontsize=11, fontstyle='italic',
             verticalalignment='top', fontfamily='serif')
    cell_text = []
    for row in table_data:
        formatted = []
        for v in row:
            if isinstance(v, str):
                formatted.append(v)
            elif abs(v) < 10:
                formatted.append(f'{v:.6f}')
            elif abs(v) < 100:
                formatted.append(f'{v:.4f}')
            elif abs(v) < 10000:
                formatted.append(f'{v:.3f}')
            else:
                formatted.append(f'{v:.1f}')
        cell_text.append(formatted)
    table = ax.table(cellText=cell_text, rowLabels=row_labels,
                     colLabels=col_labels, loc='center', cellLoc='center')
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1.0, 1.4)
    for (row, col), cell in table.get_celld().items():
        cell.set_edgecolor('#999999')
        cell.set_linewidth(0.5)
        if row == 0:
            cell.set_facecolor(header_color)
            cell.set_text_props(fontweight='bold')
        elif has_speedup and row == n_rows - 1:
            cell.set_facecolor(promedio_color)
            cell.set_text_props(fontweight='bold')
        elif has_speedup and row == n_rows:
            cell.set_facecolor(speedup_color)
            cell.set_text_props(fontweight='bold')
        elif not has_speedup and row == n_rows:
            cell.set_facecolor(promedio_color)
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
# Configuracion: k limitado a 10
# ---------------------------------------------------------------------------
k_values     = [7, 8, 9, 10]
workers      = [2, 4, 8, 16]
n_runs       = 10
col_labels   = [f'k={k} (NK={2**k+1})' for k in k_values]
row_labels   = [str(i+1) for i in range(n_runs)] + ['Promedio']
row_labels_s = [str(i+1) for i in range(n_runs)] + ['Promedio', 'Speedup']

# ---------------------------------------------------------------------------
# 1. SECUENCIAL
# ---------------------------------------------------------------------------
print('Cargando secuencial...')
sec_data = load_json('resultados_sec.json')
sec_by_k = {k: [] for k in k_values}
for e in sec_data:
    k = e['exponente_k']
    if k in k_values:
        sec_by_k[k].append(e['tiempo_ejecucion_s'])

sec_table = [[sec_by_k[k][i] for k in k_values] for i in range(n_runs)]
sec_prom   = [np.mean(sec_by_k[k]) for k in k_values]
sec_table.append(sec_prom)

render_table(sec_table, row_labels, col_labels,
             'Tabla 1: Resultados de la ejecucion secuencial.',
             'tabla_secuencial.png')

# ---------------------------------------------------------------------------
# 2. HILOS
# ---------------------------------------------------------------------------
print('Cargando hilos...')
hilos_data = load_json('resultados_hilos.json')
hilos_by = {h: {k: [] for k in k_values} for h in workers}
for e in hilos_data:
    h = e['num_hilos']
    k = e['exponente_k']
    if k in k_values and h in workers:
        hilos_by[h][k].append(e['tiempo_ejecucion_s'])

for idx, h in enumerate(workers):
    tbl = [[hilos_by[h][k][i] for k in k_values] for i in range(n_runs)]
    prom = [np.mean(hilos_by[h][k]) for k in k_values]
    spd  = [sec_prom[j] / prom[j] for j in range(len(k_values))]
    tbl.append(prom); tbl.append(spd)
    render_table(tbl, row_labels_s, col_labels,
                 f'Tabla {idx+2}: Resultados de la ejecucion con {h} hilos.',
                 f'tabla_hilos_{h}.png', has_speedup=True)

# ---------------------------------------------------------------------------
# 3. PROCESOS
# ---------------------------------------------------------------------------
print('Cargando procesos...')
proc_data = load_json('resultados_procesos.json')
proc_by = {p: {k: [] for k in k_values} for p in workers}
for e in proc_data:
    p = e['num_procesos']
    k = e['exponente_k']
    if k in k_values and p in workers:
        proc_by[p][k].append(e['tiempo_ejecucion_s'])

for idx, p in enumerate(workers):
    tbl = [[proc_by[p][k][i] for k in k_values] for i in range(n_runs)]
    prom = [np.mean(proc_by[p][k]) for k in k_values]
    spd  = [sec_prom[j] / prom[j] for j in range(len(k_values))]
    tbl.append(prom); tbl.append(spd)
    render_table(tbl, row_labels_s, col_labels,
                 f'Tabla {idx+6}: Resultados de la ejecucion con {p} procesos.',
                 f'tabla_procesos_{p}.png', has_speedup=True)

# ---------------------------------------------------------------------------
# 4. COMPILADOR (NUEVO)
# ---------------------------------------------------------------------------
print('Cargando resultados del compilador...')
comp_data = load_json('resultados_compilador.json')
comp_by_k = {k: [] for k in k_values}
for e in comp_data:
    k = e['exponente_k']
    if k in k_values:
        comp_by_k[k].append(e['tiempo_ejecucion_s'])

comp_tbl = [[comp_by_k[k][i] for k in k_values] for i in range(n_runs)]
comp_prom = [np.mean(comp_by_k[k]) for k in k_values]
comp_spd  = [sec_prom[j] / comp_prom[j] for j in range(len(k_values))]
comp_tbl.append(comp_prom); comp_tbl.append(comp_spd)

render_table(comp_tbl, row_labels_s, col_labels,
             'Tabla 10: Resultados de la optimizacion por compilador.',
             'tabla_compilador.png', has_speedup=True)

# ---------------------------------------------------------------------------
# 5. MEMORIA
# ---------------------------------------------------------------------------
print('Cargando memoria...')
mem_data = load_json('resultados_memoria.json')
mem_by_k = {k: [] for k in k_values}
for e in mem_data:
    k = e['exponente_k']
    if k in k_values:
        mem_by_k[k].append(e['tiempo_ejecucion_s'])

mem_tbl  = [[mem_by_k[k][i] for k in k_values] for i in range(n_runs)]
mem_prom = [np.mean(mem_by_k[k]) for k in k_values]
mem_spd  = [sec_prom[j] / mem_prom[j] for j in range(len(k_values))]
mem_tbl.append(mem_prom); mem_tbl.append(mem_spd)

render_table(mem_tbl, row_labels_s, col_labels,
             'Tabla 11: Resultados de la optimizacion de memoria.',
             'tabla_memoria.png', has_speedup=True)

# ---------------------------------------------------------------------------
# 6. GRAFICO COMPARATIVO FINAL (Actualizado)
# ---------------------------------------------------------------------------
print('Generando grafico comparativo final...')
nk = [2**k + 1 for k in k_values]
fig, ax = plt.subplots(figsize=(10, 6))
colors_c = ['#C0504D', '#9BBB59', '#7F6084', '#4BACC6', '#FFC000', '#4472C4', '#2ECC71']

# Mejores Hilos y Procesos
ax.plot(nk, [sec_prom[j]/np.mean(hilos_by[16][k]) for j, k in enumerate(k_values)], 
        marker='o', label='16 Hilos', color=colors_c[0])
ax.plot(nk, [sec_prom[j]/np.mean(proc_by[16][k]) for j, k in enumerate(k_values)], 
        marker='s', label='16 Procesos', color=colors_c[2])

# Optimizaciones
ax.plot(nk, mem_spd, marker='D', label='Memoria (tridiagonal)', color=colors_c[4])
ax.plot(nk, comp_spd, marker='^', label='Compilador (Flags)', color=colors_c[6])

ax.set_xlabel('Numero de nodos NK', fontsize=11)
ax.set_ylabel('Speedup', fontsize=11)
ax.set_xticks(nk); ax.set_xticklabels([str(n) for n in nk])
ax.grid(True, linestyle='-', alpha=0.3)
ax.legend(loc='best', fontsize=9)
fig.text(0.05, 0.98, 'Grafico Comparativo: Mejores Speedups segun tecnica utilizada.', 
         fontsize=11, fontstyle='italic')

plt.tight_layout(rect=[0, 0, 1, 0.95])
plt.savefig(os.path.join(SCRIPT_DIR, 'grafico_comparativo_final.png'), dpi=200)
plt.close()

print('\nAnálisis completado para k hasta 10 e inclusión de compilador.')