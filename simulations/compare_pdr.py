import pandas as pd
import matplotlib.pyplot as plt
import re
import os

BASE = os.path.dirname(os.path.abspath(__file__))
PATH_UNCONFIRMED = os.path.join(BASE, "purealohaSansRetransmissionUnconfirmed/results/florasat_scalars_only.csv")
PATH_CONFIRMED   = os.path.join(BASE, "purealohaSansRetransmissionConfirmed/results/florasat_scalars_only.csv")

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



def get_energy_per_node(csv_path):
    df = pd.read_csv(csv_path)
    for col in ["name", "module", "run"]:
        df[col] = df[col].astype(str).str.strip()
    df["value"] = pd.to_numeric(df["value"], errors="coerce")

    # Extraire N par run
    itervars = df[df["type"] == "itervar"]
    N_vals = itervars[itervars["attrname"] == "N"][["run", "attrvalue"]].copy()
    N_vals["N"] = pd.to_numeric(N_vals["attrvalue"], errors="coerce")
    N_dict = dict(zip(N_vals["run"], N_vals["N"]))

    # Filtrer les lignes énergie des loRaNodes
    mask = (df["name"] == "totalEnergyConsumed") & \
           (df["module"].str.contains("loRaNodes", na=False))
    sub = df[mask].copy()

    if sub.empty:
        print(f"[WARN] Aucune energie trouvee dans {csv_path}")
        return pd.DataFrame(columns=["N", "value"])

    # Extraire l'index du nœud depuis le module : loRaNodes[i]
    sub["node_idx"] = sub["module"].str.extract(r"loRaNodes\[(\d+)\]").astype(float)

    # Pour chaque run, ne garder que les nœuds dont l'index < N du run
    sub["N"] = sub["run"].map(N_dict)
    sub = sub.dropna(subset=["N", "node_idx"])
    sub = sub[sub["node_idx"] < sub["N"]]

    # Moyenne de l'énergie par nœud, par run → puis moyenne sur répétitions → par N
    mean_per_run = sub.groupby(["run", "N"])["value"].mean().reset_index()
    result = mean_per_run.groupby("N", as_index=False)["value"].mean().sort_values("N")
    return result


# ================================
# PDR
# ================================
der_unconf = get_results(PATH_UNCONFIRMED, "LoRa_NS_DER", module_filter="networkServer")
if der_unconf.empty:
    der_unconf = get_results(PATH_UNCONFIRMED, "LoRa_NS_DER")

ack_conf = get_results(PATH_CONFIRMED, "receivedAckPackets")
sent = get_results(PATH_CONFIRMED, "sentPackets")
if not sent.empty:
        merged = ack_conf.merge(sent, on="N", suffixes=("_ack", "_sent"))
        ack_conf = merged[["N"]].copy()
        ack_conf["value"] = merged["value_ack"] / merged["value_sent"]
        print("[INFO] receivedAckPackets normalise.")

# ================================
# Energie
# ================================
energy_unconf = get_energy_per_node(PATH_UNCONFIRMED)
energy_conf   = get_energy_per_node(PATH_CONFIRMED)

print("\n--- Unconfirmed : energie par noeud (J) ---")
print(energy_unconf)
print("\n--- Confirmed : energie par noeud (J) ---")
print(energy_conf)

# ================================
# Figure 1 : PDR
# ================================
fig1, ax1 = plt.subplots(figsize=(9, 5))
if not der_unconf.empty:
    ax1.plot(der_unconf["N"], der_unconf["value"],
             marker="o", color="steelblue", linewidth=2,
             label="Unconfirmed – LoRa NS DER")
if not ack_conf.empty:
    ax1.plot(ack_conf["N"], ack_conf["value"],
             marker="s", color="darkorange", linewidth=2, linestyle="--",
             label="Confirmed – Received ACK ratio")
ax1.set_xlabel("Nombre de noeuds (N)", fontsize=12)
ax1.set_ylabel("PDR (Packet Delivery Ratio)", fontsize=12)
ax1.set_title("PDR : Pure ALOHA sans retransmission — Unconfirmed vs Confirmed",
              fontsize=12, fontweight="bold")
ax1.set_ylim(0, 1.05)
ax1.yaxis.set_major_locator(plt.MultipleLocator(0.1))
ax1.xaxis.set_major_locator(plt.MultipleLocator(100))
ax1.legend(fontsize=11)
ax1.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("pdr_unconf_vs_conf.png", dpi=300)
plt.show()
print("[OK] Figure sauvegardee : pdr_unconf_vs_conf.png")

# ================================
# Figure 2 : Energie
# ================================
# Figure 2 : Energie
fig2, ax2 = plt.subplots(figsize=(9, 5))
if not energy_unconf.empty:
    eu = energy_unconf[energy_unconf["N"] >= 100]
    ax2.plot(eu["N"], eu["value"],
             marker="o", color="steelblue", linewidth=2,
             label="Unconfirmed")
if not energy_conf.empty:
    ec = energy_conf[energy_conf["N"] > 100]
    ax2.plot(ec["N"], ec["value"],
             marker="s", color="darkorange", linewidth=2, linestyle="--",
             label="Confirmed")
ax2.set_ylim(0.8, 1.2)
ax2.set_xlabel("Nombre de noeuds (N)", fontsize=12)
ax2.set_ylabel("Energie moyenne par noeud (J)", fontsize=12)
ax2.set_title("Energie consommee : Pure ALOHA sans retransmission — Unconfirmed vs Confirmed",
              fontsize=12, fontweight="bold")
ax2.xaxis.set_major_locator(plt.MultipleLocator(100))
ax2.legend(fontsize=11)
ax2.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("energy_unconf_vs_conf.png", dpi=300)
plt.show()
print("[OK] Figure sauvegardee : energy_unconf_vs_conf.png")
