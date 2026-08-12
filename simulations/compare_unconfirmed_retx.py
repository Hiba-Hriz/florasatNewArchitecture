import pandas as pd
import matplotlib.pyplot as plt
import re
import os

BASE = os.path.dirname(os.path.abspath(__file__))

PATH_0RETX = os.path.join(BASE, "purealohaSansRetransmissionUnconfirmed/results/florasat_scalars_only.csv")
PATH_2RETX = os.path.join(BASE, "pureAlohaAvec2RetransmissionUnconfirmed/results/florasat_scalars_only.csv")
PATH_7RETX = os.path.join(BASE, "pureAlohaAvec7RetransmissionUnconfirmed/results/florasat_scalars_only.csv")
# ================================
# Fonction générique (PDR, etc.)
# ================================
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


# ================================
# Fonction énergie par noeud
# ================================
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


# ================================
# PDR — LoRa_NS_DER
# ================================
der_0retx = get_results(PATH_0RETX, "LoRa_NS_DER", module_filter="networkServer")
if der_0retx.empty:
    der_0retx = get_results(PATH_0RETX, "LoRa_NS_DER")

der_2retx = get_results(PATH_2RETX, "LoRa_NS_DER", module_filter="networkServer")
if der_2retx.empty:
    der_2retx = get_results(PATH_2RETX, "LoRa_NS_DER")
der_7retx = get_results(PATH_7RETX, "LoRa_NS_DER", module_filter="networkServer")
if der_7retx.empty:
    der_7retx = get_results(PATH_7RETX, "LoRa_NS_DER")

energy_7retx = get_energy_per_node(PATH_7RETX)
# ================================
# Energie
# ================================
energy_0retx = get_energy_per_node(PATH_0RETX)
energy_2retx = get_energy_per_node(PATH_2RETX)

print("\n--- Sans retransmission : LoRa_NS_DER ---")
print(der_0retx)
print("\n--- 2 retransmissions : LoRa_NS_DER ---")
print(der_2retx)
print("\n--- Sans retransmission : energie par noeud (J) ---")
print(energy_0retx)
print("\n--- 2 retransmissions : energie par noeud (J) ---")
print(energy_2retx)

# ================================
# Figure 1 : PDR (LoRa_NS_DER)
# ================================
fig1, ax1 = plt.subplots(figsize=(9, 5))
if not der_0retx.empty:
    ax1.plot(der_0retx["N"], der_0retx["value"],
             marker="o", color="steelblue", linewidth=2,
             label="Sans retransmission - LoRa_NS_DER")
if not der_2retx.empty:
    ax1.plot(der_2retx["N"], der_2retx["value"],
             marker="s", color="darkorange", linewidth=2, linestyle="--",
             label="2 retransmissions - LoRa_NS_DER")
if not der_7retx.empty:
    ax1.plot(der_7retx["N"], der_7retx["value"],
             marker="^", color="green", linewidth=2, linestyle=":",
             label="7 retransmissions - LoRa_NS_DER")

ax1.set_xlabel("Nombre de noeuds (N)", fontsize=12)
ax1.set_ylabel("PDR", fontsize=12)
ax1.set_title("PDR : Pure ALOHA Unconfirmed — Sans vs 2 retransmissions",
              fontsize=12, fontweight="bold")
ax1.set_ylim(0, 1.05)
ax1.yaxis.set_major_locator(plt.MultipleLocator(0.1))
ax1.xaxis.set_major_locator(plt.MultipleLocator(100))
ax1.legend(fontsize=11)
ax1.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("pdr_unconfirmed_0vs2retx.png", dpi=300)
plt.show()
print("[OK] Figure sauvegardee : pdr_unconfirmed_0vs2retx.png")

# ================================
# Figure 2 : Energie
# ================================
fig2, ax2 = plt.subplots(figsize=(9, 5))
if not energy_0retx.empty:
    ax2.plot(energy_0retx["N"], energy_0retx["value"],
             marker="o", color="steelblue", linewidth=2,
             label="Sans retransmission - LoRa_NS_DER")
if not energy_2retx.empty:
    ax2.plot(energy_2retx["N"], energy_2retx["value"],
             marker="s", color="darkorange", linewidth=2, linestyle="--",
             label="2 retransmissions - LoRa_NS_DER")
if not energy_7retx.empty:
    ax2.plot(energy_7retx["N"], energy_7retx["value"],
             marker="^", color="green", linewidth=2, linestyle=":",
             label="7 retransmissions - LoRa_NS_DER")

ax2.set_xlabel("Nombre de noeuds (N)", fontsize=12)
ax2.set_ylabel("Energie moyenne par noeud (J)", fontsize=12)
ax2.set_title("Energie consommee : Pure ALOHA Unconfirmed — Sans vs 2 retransmissions",
              fontsize=12, fontweight="bold")
ax2.xaxis.set_major_locator(plt.MultipleLocator(100))
ax2.legend(fontsize=11)
ax2.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("energy_unconfirmed_0vs2retx.png", dpi=300)
plt.show()
print("[OK] Figure sauvegardee : energy_unconfirmed_0vs2retx.png")
