import pandas as pd
import matplotlib.pyplot as plt
import os

BASE = os.getcwd()

PATH_DEFAULT = os.path.join(BASE, "pureAlohaAvec2RetransmissionUnconfirmed/results/florasat_scalars_only.csv")
PATH_59B     = os.path.join(BASE, "purealohaAvec2RetransmissionUnconfirmed59B/results/florasat_scalars_only.csv")
PATH_28B     = os.path.join(BASE, "purealohaAvec2RetransmissionUnconfirmed28B/results/florasat_scalars_only.csv")

def get_results(csv_path, scalar_name, module_filter=None):
    df = pd.read_csv(csv_path)
    for col in ["name", "module", "run"]:
        df[col] = df[col].astype(str).str.strip()
    df["value"] = pd.to_numeric(df["value"], errors="coerce")
    itervars = df[df["type"] == "itervar"]
    N_vals = itervars[itervars["attrname"] == "N"][["run", "attrvalue"]].copy()
    N_vals["N"] = pd.to_numeric(N_vals["attrvalue"], errors="coerce")
    mask = df["name"] == scalar_name
    if module_filter:
        mask &= df["module"].str.contains(module_filter, na=False)
    sub = df[mask].copy()
    if sub.empty:
        print(f"[WARN] Aucune donnee pour '{scalar_name}' dans {csv_path}")
        return pd.DataFrame(columns=["N", "value"])
    per_run = sub.groupby("run")["value"].mean()
    wide = per_run.reset_index().merge(N_vals[["run", "N"]], on="run")
    return wide.groupby("N", as_index=False)["value"].mean().sort_values("N")

def get_energy_per_node(csv_path):
    df = pd.read_csv(csv_path)
    for col in ["name", "module", "run"]:
        df[col] = df[col].astype(str).str.strip()
    df["value"] = pd.to_numeric(df["value"], errors="coerce")
    itervars = df[df["type"] == "itervar"]
    N_vals = itervars[itervars["attrname"] == "N"][["run", "attrvalue"]].copy()
    N_vals["N"] = pd.to_numeric(N_vals["attrvalue"], errors="coerce")
    N_dict = dict(zip(N_vals["run"], N_vals["N"]))
    mask = (df["name"] == "totalEnergyConsumed") & \
           (df["module"].str.contains("loRaNodes", na=False))
    sub = df[mask].copy()
    if sub.empty:
        print(f"[WARN] Aucune energie trouvee dans {csv_path}")
        return pd.DataFrame(columns=["N", "value"])
    sub["node_idx"] = sub["module"].str.extract(r"loRaNodes\[(\d+)\]").astype(float)
    sub["N"] = sub["run"].map(N_dict)
    sub = sub.dropna(subset=["N", "node_idx"])
    sub = sub[sub["node_idx"] < sub["N"]]
    mean_per_run = sub.groupby(["run", "N"])["value"].mean().reset_index()
    result = mean_per_run.groupby("N", as_index=False)["value"].mean().sort_values("N")
    return result

def load_der(path):
    der = get_results(path, "LoRa_NS_DER", module_filter="networkServer")
    if der.empty:
        der = get_results(path, "LoRa_NS_DER")
    return der

der_default = load_der(PATH_DEFAULT)
der_59b     = load_der(PATH_59B)
der_28b     = load_der(PATH_28B)

energy_default = get_energy_per_node(PATH_DEFAULT)
energy_59b     = get_energy_per_node(PATH_59B)
energy_28b     = get_energy_per_node(PATH_28B)

# ================================
# Affichage console
# ================================
print("\n--- 10B : LoRa_NS_DER ---")
print(der_default)
print("\n--- 59B : LoRa_NS_DER ---")
print(der_59b)
print("\n--- 28B : LoRa_NS_DER ---")
print(der_28b)

print("\n--- 10B : énergie par nœud (J) ---")
print(energy_default)
print("\n--- 59B : énergie par nœud (J) ---")
print(energy_59b)
print("\n--- 28B : énergie par nœud (J) ---")
print(energy_28b)

# ================================
# Figure 1 : PDR
# ================================
fig1, ax1 = plt.subplots(figsize=(9, 5))

if not der_default.empty:
    ax1.plot(der_default["N"], der_default["value"],
             marker="o", color="steelblue", linewidth=2,
             label="Payload 10B")
if not der_59b.empty:
    ax1.plot(der_59b["N"], der_59b["value"],
             marker="s", color="darkorange", linewidth=2, linestyle="--",
             label="Payload 59B")
if not der_28b.empty:
    ax1.plot(der_28b["N"], der_28b["value"],
             marker="^", color="green", linewidth=2, linestyle=":",
             label="Payload 28B")

ax1.set_xlabel("Nombre de nœuds (N)", fontsize=12)
ax1.set_ylabel("PDR (LoRa_NS_DER)", fontsize=12)
ax1.set_title("PDR du cas Unconfirmed, Sans Retransmission",
              fontsize=12, fontweight="bold")
ax1.set_ylim(0, 1.05)
ax1.yaxis.set_major_locator(plt.MultipleLocator(0.1))
ax1.xaxis.set_major_locator(plt.MultipleLocator(100))
ax1.legend(fontsize=11)
ax1.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("pdr_payload_comparison.png", dpi=300)
plt.show()
print("[OK] Figure sauvegardée : pdr_payload_comparison.png")

# ================================
# Figure 2 : Énergie — histogrammes par taille de payload
# ================================
mean_10b = energy_default["value"].mean() if not energy_default.empty else 0
mean_59b = energy_59b["value"].mean()     if not energy_59b.empty     else 0
mean_28b = energy_28b["value"].mean()     if not energy_28b.empty     else 0

labels = ["10B", "28B", "59B"]
values = [mean_10b, mean_28b, mean_59b]
colors = ["steelblue", "green", "darkorange"]

fig2, ax2 = plt.subplots(figsize=(7, 5))
bars = ax2.bar(labels, values, color=colors, width=0.5, edgecolor="black")

for bar, val in zip(bars, values):
    ax2.text(bar.get_x() + bar.get_width() / 2,
             bar.get_height() + 0.001 * max(values),
             f"{val:.4f} J",
             ha="center", va="bottom", fontsize=10)

ax2.set_xlabel("Taille du payload", fontsize=12)
ax2.set_ylabel("Énergie moyenne par nœud (J)", fontsize=12)
ax2.set_title("Cas Unconfirmed, Sans Retransmission",
              fontsize=12, fontweight="bold")
ax2.grid(True, axis="y", linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("energy_payload_comparison.png", dpi=300)
plt.show()
print("[OK] Figure sauvegardée : energy_payload_comparison.png")
