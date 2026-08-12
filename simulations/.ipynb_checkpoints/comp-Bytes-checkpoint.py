import pandas as pd
import matplotlib.pyplot as plt
import os

# ================================
# Chemins
# ================================
BASE = os.path.dirname(os.path.abspath(__file__))

PATH_0B = os.path.join(BASE, "purealohaSansRetransmissionUnconfirmed/results/florasat_scalars_only.csv")
PATH_59B = os.path.join(BASE, "purealohaSansRetransmissionUnconfirmed59B/results/florasat_scalars_only.csv")
PATH_250B = os.path.join(BASE, "purealohaSansRetransmissionUnconfirmed250B/results/florasat_scalars_only.csv")


# ================================
# Fonction générique (PDR, etc.)
# ================================
def get_results(csv_path, scalar_name, module_filter=None):
    df = pd.read_csv(csv_path)

    # Nettoyage
    for col in ["name", "module", "run"]:
        df[col] = df[col].astype(str).str.strip()

    df["value"] = pd.to_numeric(df["value"], errors="coerce")

    # Extraction N
    itervars = df[df["type"] == "itervar"]
    N_vals = itervars[itervars["attrname"] == "N"][["run", "attrvalue"]].copy()
    N_vals["N"] = pd.to_numeric(N_vals["attrvalue"], errors="coerce")

    # Filtrage scalaire
    mask = df["name"] == scalar_name
    if module_filter:
        mask &= df["module"].str.contains(module_filter, na=False)

    sub = df[mask].copy()

    if sub.empty:
        print(f"[WARN] Aucune donnée pour '{scalar_name}' dans {csv_path}")
        return pd.DataFrame(columns=["N", "value"])

    # Moyenne par run
    per_run = sub.groupby("run")["value"].mean()

    # Ajout de N
    wide = per_run.reset_index().merge(N_vals[["run", "N"]], on="run")

    # Moyenne finale par N
    return wide.groupby("N", as_index=False)["value"].mean().sort_values("N")


# ================================
# Energie par noeud
# ================================
def get_energy_per_node(csv_path):
    df = pd.read_csv(csv_path)

    for col in ["name", "module", "run"]:
        df[col] = df[col].astype(str).str.strip()

    df["value"] = pd.to_numeric(df["value"], errors="coerce")

    # Extraction N
    itervars = df[df["type"] == "itervar"]
    N_vals = itervars[itervars["attrname"] == "N"][["run", "attrvalue"]].copy()
    N_vals["N"] = pd.to_numeric(N_vals["attrvalue"], errors="coerce")
    N_dict = dict(zip(N_vals["run"], N_vals["N"]))

    # Filtre énergie
    mask = (df["name"] == "totalEnergyConsumed") & \
           (df["module"].str.contains("loRaNodes", na=False))

    sub = df[mask].copy()

    if sub.empty:
        print(f"[WARN] Aucune énergie trouvée dans {csv_path}")
        return pd.DataFrame(columns=["N", "value"])

    # Extraction index noeud
    sub["node_idx"] = sub["module"].str.extract(r"loRaNodes\[(\d+)\]").astype(float)

    sub["N"] = sub["run"].map(N_dict)
    sub = sub.dropna(subset=["N", "node_idx"])

    # Garder uniquement les noeuds valides
    sub = sub[sub["node_idx"] < sub["N"]]

    # Moyenne par run puis par N
    mean_per_run = sub.groupby(["run", "N"])["value"].mean().reset_index()
    result = mean_per_run.groupby("N", as_index=False)["value"].mean().sort_values("N")

    return result


# ================================
# Extraction des données
# ================================
der_0B = get_results(PATH_0B, "LoRa_NS_DER", module_filter="networkServer")
der_59B = get_results(PATH_59B, "LoRa_NS_DER", module_filter="networkServer")
der_250B = get_results(PATH_250B, "LoRa_NS_DER", module_filter="networkServer")

# fallback si module_filter ne marche pas
if der_0B.empty:
    der_0B = get_results(PATH_0B, "LoRa_NS_DER")
if der_59B.empty:
    der_59B = get_results(PATH_59B, "LoRa_NS_DER")
if der_250B.empty:
    der_250B = get_results(PATH_250B, "LoRa_NS_DER")

energy_0B = get_energy_per_node(PATH_0B)
energy_59B = get_energy_per_node(PATH_59B)
energy_250B = get_energy_per_node(PATH_250B)


# ================================
# Debug affichage
# ================================
print("\n--- PDR 0B ---")
print(der_0B)

print("\n--- PDR 59B ---")
print(der_59B)

print("\n--- PDR 250B ---")
print(der_250B)

print("\n--- Energie 0B ---")
print(energy_0B)

print("\n--- Energie 59B ---")
print(energy_59B)

print("\n--- Energie 250B ---")
print(energy_250B)


# ================================
# Figure 1 : PDR
# ================================
fig1, ax1 = plt.subplots(figsize=(9, 5))

if not der_0B.empty:
    ax1.plot(der_0B["N"], der_0B["value"], marker="o", linewidth=2, label="0B")

if not der_59B.empty:
    ax1.plot(der_59B["N"], der_59B["value"], marker="s", linestyle="--", linewidth=2, label="59B")

if not der_250B.empty:
    ax1.plot(der_250B["N"], der_250B["value"], marker="^", linestyle=":", linewidth=2, label="250B")

ax1.set_xlabel("Nombre de noeuds (N)")
ax1.set_ylabel("PDR")
ax1.set_title("PDR vs taille du payload (Pure ALOHA Unconfirmed)")
ax1.set_ylim(0, 1.05)

ax1.legend()
ax1.grid(True, linestyle="--", alpha=0.6)

plt.tight_layout()
plt.savefig("pdr_payload_comparison.png", dpi=300)
plt.show()


# ================================
# Figure 2 : Energie
# ================================
fig2, ax2 = plt.subplots(figsize=(9, 5))

if not energy_0B.empty:
    ax2.plot(energy_0B["N"], energy_0B["value"], marker="o", linewidth=2, label="0B")

if not energy_59B.empty:
    ax2.plot(energy_59B["N"], energy_59B["value"], marker="s", linestyle="--", linewidth=2, label="59B")

if not energy_250B.empty:
    ax2.plot(energy_250B["N"], energy_250B["value"], marker="^", linestyle=":", linewidth=2, label="250B")

ax2.set_xlabel("Nombre de noeuds (N)")
ax2.set_ylabel("Energie moyenne par noeud (J)")
ax2.set_title("Energie vs taille du payload (Pure ALOHA Unconfirmed)")

ax2.legend()
ax2.grid(True, linestyle="--", alpha=0.6)

plt.tight_layout()
plt.savefig("energy_payload_comparison.png", dpi=300)
plt.show()

print("[OK] Figures générées : pdr_payload_comparison.png & energy_payload_comparison.png")
