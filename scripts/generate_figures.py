#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Set style for professional publication-quality plots
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
os.makedirs('results/figures', exist_ok=True)

# -------------------------------------------------------------
# Figure 1: Topological Sort Performance (Structure A vs B)
# -------------------------------------------------------------
struct_csv = "results/raw/struct_comparison.csv"
if os.path.exists(struct_csv):
    df_struct = pd.read_csv(struct_csv)
    
    plt.figure(figsize=(9, 6))
    
    # Pick a representative vertex size (e.g., the largest or middle scale) for a clean 2D plot
    target_v = df_struct['V'].max()
    sub_df = df_struct[df_struct['V'] == target_v]
    
    # Group by SkewFactor and Structure to calculate mean & std across the 5 runs
    if 'MeanTimeMs' in sub_df.columns and 'SdTimeMs' in sub_df.columns:
        # If already aggregated in CSV
        grouped = sub_df.groupby(['Skew', 'Structure'])[['MeanTimeMs', 'SdTimeMs']].mean().reset_index()
        x_col, mean_col, err_col = 'Skew', 'MeanTimeMs', 'SdTimeMs'
    else:
        # If raw per-run times are logged
        grouped = sub_df.groupby(['SkewFactor', 'Structure'])['TimeMs'].agg(['mean', 'std']).reset_index()
        x_col, mean_col, err_col = 'SkewFactor', 'mean', 'std'

    colors = {'StructureA_FIFO': '#1f77b4', 'StructureA_Heap': '#ff7f0e', 'StructureB_DFS': '#2ca02c'}
    markers = {'StructureA_FIFO': 'o', 'StructureA_Heap': 's', 'StructureB_DFS': '^'}

    for struct_name in grouped['Structure'].unique():
        data = grouped[grouped['Structure'] == struct_name].sort_values(by=x_col)
        color = colors.get(struct_name, '#333333')
        marker = markers.get(struct_name, 'o')
        
        plt.errorbar(data[x_col], data[mean_col], yerr=data[err_col],
                     label=struct_name, fmt=f'-{marker}', color=color, 
                     capsize=5, elinewidth=1.5, linewidth=2)

    plt.yscale('log')
    plt.xlabel('In-Degree Skew Factor', fontsize=12, fontweight='bold')
    plt.ylabel('Execution Time (ms) [Log Scale]', fontsize=12, fontweight='bold')
    plt.title(f'Topological Sort Performance vs. In-Degree Skew (V = {target_v})', fontsize=14, pad=12)
    plt.legend(frameon=True, facecolor='white', edgecolor='none', fontsize=11)
    plt.grid(True, which="both", ls="--", alpha=0.6)
    plt.tight_layout()
    plt.savefig('results/figures/fig1_structure_comparison.png', dpi=300)
    plt.close()
    print("[Plots] Regenerated clean results/figures/fig1_structure_comparison.png")

# -------------------------------------------------------------
# Figure 2: Incremental Maintenance vs. Full Recomputation
# -------------------------------------------------------------
increm_csv = "results/raw/incremental_comparison.csv"
if os.path.exists(increm_csv):
    df_inc = pd.read_csv(increm_csv)
    
    plt.figure(figsize=(10, 6))
    
    if 'InsertionIndex' in df_inc.columns:
        x = df_inc['InsertionIndex']
        
        if 'IncrementalTimeMs' in df_inc.columns:
            plt.plot(x, df_inc['IncrementalTimeMs'], label='Incremental Maintenance (Pearce-Kelly)', 
                     color='#1f77b4', linewidth=2)
            
        if 'RecompTimeMs' in df_inc.columns:
            # Filter out zero/empty recomputation samples if using periodic sampling
            rec_data = df_inc[df_inc['RecompTimeMs'] > 0]
            plt.scatter(rec_data['InsertionIndex'], rec_data['RecompTimeMs'], color='#d62728', 
                        label='Full Recomputation (Sampled)', marker='x', s=50, zorder=5)
            plt.plot(df_inc['InsertionIndex'], df_inc['RecompTimeMs'], color='#d62728', 
                     linestyle='--', alpha=0.5, label='Full Recomputation Trend')

    plt.yscale('log')
    plt.xlabel('Edge Insertion Index', fontsize=12, fontweight='bold')
    plt.ylabel('Execution Time (ms) [Log Scale]', fontsize=12, fontweight='bold')
    plt.title('Incremental Order Maintenance vs. Full Recomputation Cost', fontsize=14, pad=12)
    plt.legend(frameon=True, facecolor='white', edgecolor='none', fontsize=11)
    plt.grid(True, which="both", ls="--", alpha=0.6)
    plt.tight_layout()
    plt.savefig('results/figures/fig2_incremental_vs_recomp.png', dpi=300)
    plt.close()
    print("[Plots] Regenerated clean results/figures/fig2_incremental_vs_recomp.png")