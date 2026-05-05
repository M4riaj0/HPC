import json
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import os

matplotlib.rcParams['font.family'] = 'serif'
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

def load_json(filename):
    path = os.path.join(SCRIPT_DIR, filename)
    if not os.path.exists(path):
        print(f"Advertencia: No se encontró {path}")
        return []
    with open(path, 'r') as f:
        return json.load(f)

# --- CONFIGURACIÓN DE PARÁMETROS (Basados en tus Bash) ---
TAMANOS = [500000, 1000000, 2000000, 4000000]
DENSIDADES = [0.1, 0.3, 0.5, 0.7, 0.9]
HILOS = [2, 4, 8, 16]
N_RUNS = 10

def process_data(data, is_parallel=False):
    if is_parallel:
        processed = {h: {n: [] for n in TAMANOS} for h in HILOS}
        for e in data:
            # Ajuste de llaves a 'threads' y 'time' según tu JSON paralelo
            h, n, t = e.get('threads'), e.get('n'), e.get('time')
            if h in HILOS and n in TAMANOS and t is not None:
                processed[h][n].append(t)
    else:
        processed = {n: [] for n in TAMANOS}
        for e in data:
            # Ajuste de llaves a 'n' y 'time' (o 'tiempo') para secuencial
            n, t = e.get('n'), e.get('time') or e.get('tiempo')
            if n in TAMANOS and t is not None:
                processed[n].append(t)
    return processed

# Cargar y procesar
sec_pc1 = process_data(load_json('PC1/resultados_secuencial.json'))
sec_pc2 = process_data(load_json('PC2/resultados_secuencial.json'))
par_pc1 = process_data(load_json('PC1/resultados_paralelo.json'), True)
par_pc2 = process_data(load_json('PC2/resultados_paralelo.json'), True)

def render_table(table_data, row_labels, col_labels, title, filename):
    n_rows, n_cols = len(table_data), len(col_labels)
    fig, ax = plt.subplots(figsize=(2.0 + n_cols * 1.5, 1.0 + n_rows * 0.4))
    ax.axis('off')
    fig.text(0.05, 0.95, title, fontsize=10, fontstyle='italic', fontfamily='serif')

    cell_text = [[f"{v:.6f}" if isinstance(v, (float, np.float64)) else v for v in row] for row in table_data]
    
    table = ax.table(cellText=cell_text, rowLabels=row_labels, colLabels=col_labels, loc='center', cellLoc='center')
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1.0, 1.3)

    for (row, col), cell in table.get_celld().items():
        if row == 0 or col == -1: cell.set_facecolor('#D5C4A1')
        elif row == n_rows: cell.set_facecolor('#7EC8E3')
    
    plt.savefig(os.path.join(SCRIPT_DIR, filename), dpi=200, bbox_inches='tight')
    plt.close()

# --- GENERACIÓN DE TABLAS SECUENCIALES ---
for name, data in [("PC1", sec_pc1), ("PC2", sec_pc2)]:
    if not data[TAMANOS[0]]: continue
    rows = [[np.mean(data[n][i*5 : (i+1)*5]) for n in TAMANOS] for i in range(N_RUNS)]
    rows.append([np.mean(data[n]) for n in TAMANOS])
    labels = [f"Corrida {i+1}" for i in range(N_RUNS)] + ["PROMEDIO"]
    render_table(rows, labels, [f"N={n}" for n in TAMANOS], f"Tabla: Resultados Secuenciales {name}", f"tabla_sec_{name}.png")

# --- GENERACIÓN DE TABLAS PARALELAS (Una por cada Hilo/PC) ---
for pc_label, pc_data in [("PC1", par_pc1), ("PC2", par_pc2)]:
    for h in HILOS:
        if not pc_data[h][TAMANOS[0]]: continue
        # Cada N tiene 5 densidades por corrida (10 corridas * 5 = 50 datos)
        rows = [[np.mean(pc_data[h][n][i*5 : (i+1)*5]) for n in TAMANOS] for i in range(N_RUNS)]
        rows.append([np.mean(pc_data[h][n]) for n in TAMANOS])
        labels = [f"Corrida {i+1}" for i in range(N_RUNS)] + ["PROMEDIO"]
        render_table(rows, labels, [f"N={n}" for n in TAMANOS], f"Tabla: Resultados OpenMP {pc_label} - {h} Hilos", f"tabla_par_{pc_label}_{h}h.png")

# --- GRÁFICAS COMPARATIVAS ---
def quick_plot(n_list, y1, y2, title, label1, label2, fname):
    plt.figure(figsize=(8, 5))
    plt.plot(n_list, y1, 'o-', label=label1, color='#C0504D')
    plt.plot(n_list, y2, 's-', label=label2, color='#4BACC6')
    plt.title(title, fontsize=12, fontweight='bold')
    plt.xlabel('N (Celdas)')
    plt.ylabel('Tiempo (s)')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.savefig(os.path.join(SCRIPT_DIR, fname), dpi=200)
    plt.close()

# Comparar Secuencial PC1 vs PC2
y_sec1 = [np.mean(sec_pc1[n]) for n in TAMANOS] if sec_pc1[TAMANOS[0]] else []
y_sec2 = [np.mean(sec_pc2[n]) for n in TAMANOS] if sec_pc2[TAMANOS[0]] else []
if y_sec1 and y_sec2:
    quick_plot(TAMANOS, y_sec1, y_sec2, "Comparativa Secuencial: PC1 vs PC2", "PC1", "PC2", "grafica_comp_sec.png")

# Comparar Paralelo (16 hilos) PC1 vs PC2
y_par1 = [np.mean(par_pc1[16][n]) for n in TAMANOS] if par_pc1[16][TAMANOS[0]] else []
y_par2 = [np.mean(par_pc2[16][n]) for n in TAMANOS] if par_pc2[16][TAMANOS[0]] else []
if y_par1 and y_par2:
    quick_plot(TAMANOS, y_par1, y_par2, "Comparativa OpenMP (16 Hilos): PC1 vs PC2", "PC1", "PC2", "grafica_comp_par.png")

print("Tablas y gráficas generadas con éxito.")