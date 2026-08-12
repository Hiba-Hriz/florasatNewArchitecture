import pandas as pd
import matplotlib.pyplot as plt
import os

# ================================
# Chemins vers les deux dossiers
# ================================
BASE = os.path.dirname(os.path.abspath(__file__))

PATH_2RETX = os.path.join(BASE, "pureAlohaAvec2RetransmissionConfirmed/results/florasat_scalars_only.csv")
PATH_7RETX = os.path.join(BASE, "pureAlohaAvec7RetransmissionConfirmed/results/florasat_scalars_only.csv")
PATH_0RETX = os.path.join(BASE, "purealohaSansRetransmissionConfirmed/results/florasat_scalars_only.csv")
# ================================
# Fonction générique d'extraction
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
# PDR — receivedAckPackets
# ================================
ack_2retx = get_results(PATH_2RETX, "receivedAckPackets")
ack_7retx = get_results(PATH_7RETX, "receivedAckPackets")
# --- Courbe de référence : 0 retransmission ---
ack_0retx = get_results(PATH_0RETX, "receivedAckPackets")
sent_0 = get_results(PATH_0RETX, "sentPackets")
if not sent_0.empty:
    merged = ack_0retx.merge(sent_0, on="N", suffixes=("_ack", "_sent"))
    ack_0retx = merged[["N"]].copy()
    ack_0retx["value"] = merged["value_ack"] / merged["value_sent"]

# Normalisation
for label, ack_df, path in [("2 retx", ack_2retx, PATH_2RETX), ("7 retx", ack_7retx, PATH_7RETX)]:
    sent = get_results(path, "sentPackets")
    if not sent.empty:
        merged = ack_df.merge(sent, on="N", suffixes=("_ack", "_sent"))
        result = merged[["N"]].copy()
        result["value"] = merged["value_ack"] / merged["value_sent"]
        if label == "2 retx":
            ack_2retx = result
        else:
            ack_7retx = result
        print(f"[INFO] receivedAckPackets normalise pour {label}.")
    else:
        print(f"[WARN] Impossible de normaliser pour {label} — utilise tel quel.")

# ================================
# Energie — totalEnergyConsumed
# ================================
energy_2retx = get_results(PATH_2RETX, "totalEnergyConsumed", module_filter="loRaNodes")
if energy_2retx.empty:
    energy_2retx = get_results(PATH_2RETX, "totalEnergyConsumed")

energy_7retx = get_results(PATH_7RETX, "totalEnergyConsumed", module_filter="loRaNodes")
if energy_7retx.empty:
    energy_7retx = get_results(PATH_7RETX, "totalEnergyConsumed")
energy_0retx_c = get_results(PATH_0RETX, "totalEnergyConsumed", module_filter="loRaNodes")

# ================================
# Figure 1 : PDR
# ================================
fig1, ax1 = plt.subplots(figsize=(9, 5))
if not ack_2retx.empty:
    ax1.plot(ack_2retx["N"], ack_2retx["value"],
             marker="o", color="steelblue", linewidth=2,
             label="Confirmed – 2 retransmissions (Received ACK ratio)")
if not ack_7retx.empty:
    ax1.plot(ack_7retx["N"], ack_7retx["value"],
             marker="s", color="darkorange", linewidth=2, linestyle="--",
             label="Confirmed – 7 retransmissions (Received ACK ratio)")
if not ack_0retx.empty:
    ax1.plot(ack_0retx["N"], ack_0retx["value"],
             marker="^", color="green", linewidth=2, linestyle=":",
             label="Confirmed – Sans retransmission ")

ax1.set_xlabel("Nombre de noeuds (N)", fontsize=12)
ax1.set_ylabel("PDR (Packet Delivery Ratio)", fontsize=12)
ax1.set_title("PDR : Pure ALOHA Confirmed — 2 vs 7 retransmissions",
              fontsize=12, fontweight="bold")
ax1.set_ylim(0, 1.05)
ax1.yaxis.set_major_locator(plt.MultipleLocator(0.1))
ax1.xaxis.set_major_locator(plt.MultipleLocator(100))
ax1.legend(fontsize=11)
ax1.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("pdr_2vs7retx.png", dpi=300)
plt.show()
print("[OK] Figure sauvegardee : pdr_2vs7retx.png")

# ================================
# Figure 2 : Energie
# ================================
fig2, ax2 = plt.subplots(figsize=(9, 5))
if not energy_2retx.empty:
    ax2.plot(energy_2retx["N"], energy_2retx["value"],
             marker="o", color="steelblue", linewidth=2,
             label="2 retransmissions")
if not energy_7retx.empty:
    ax2.plot(energy_7retx["N"], energy_7retx["value"],
             marker="s", color="darkorange", linewidth=2, linestyle="--",
             label="7 retransmissions")
ax2.plot(energy_0retx_c["N"], energy_0retx_c["value"],
         marker="^", color="green", linewidth=2, linestyle=":",
         label="Sans retransmission ")

ax2.set_xlabel("Nombre de noeuds (N)", fontsize=12)
ax2.set_ylabel("Energie moyenne par noeud (J)", fontsize=12)
ax2.set_title("Energie consommee : Pure ALOHA Confirmed — 2 vs 7 retransmissions",
              fontsize=12, fontweight="bold")
ax2.xaxis.set_major_locator(plt.MultipleLocator(100))
ax2.legend(fontsize=11)
ax2.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("energy_2vs7retx.png", dpi=300)
plt.show()
print("[OK] Figure sauvegardee : energy_2vs7retx.png")
