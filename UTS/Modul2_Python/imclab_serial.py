#!/usr/bin/env python3
"""
MODUL 2 - Kontrol iMCLab (ESP32 + Motor DC) dari Python via Serial.

Pasangan firmware: imclab_naik_turun/imclab_naik_turun.ino

Mode:
  interaktif  : kirim perintah manual (1, 0, +, -, p<angka>, r, m, q)
  demo        : urutan otomatis ON -> naik -> turun -> set PWM -> OFF
  eksperimen  : profil duty naik-turun, log ke CSV, plot ke PNG

Contoh:
  python imclab_serial.py                       # interaktif, port auto-detect
  python imclab_serial.py --mode eksperimen     # profil naik-turun + grafik
  python imclab_serial.py --mode eksperimen --hold 8 --port COM11
"""

import argparse
import csv
import sys
import time
from datetime import datetime
from pathlib import Path

import serial
import serial.tools.list_ports

# ============================================================
# KONFIGURASI
# ============================================================
BAUDRATE = 115200

ESP32_KEYWORDS = [
    "cp210", "ch340", "ch341", "ch34x", "ftdi", "ft232",
    "usb-serial", "usb serial", "usb uart", "uart bridge",
    "silicon labs", "espressif", "esp32",
]
ESP32_VID_PID = {
    (0x10C4, 0xEA60), (0x10C4, 0xEA70),   # Silicon Labs CP210x
    (0x1A86, 0x7523), (0x1A86, 0x55D4),   # WCH CH340 / CH9102
    (0x0403, 0x6001), (0x0403, 0x6015),   # FTDI
    (0x303A, 0x1001),                     # Espressif USB JTAG/Serial
}

# Profil eksperimen: duty naik lalu turun
PROFIL_NAIK = [50, 100, 150, 200, 250]
PROFIL_TURUN = [200, 150, 100, 50, 0]


# ============================================================
# DETEKSI PORT
# ============================================================
def list_serial_ports():
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("Tidak ada port serial yang terdeteksi.")
        return []
    print("Port serial yang tersedia:")
    for p in ports:
        vp = ""
        if p.vid is not None and p.pid is not None:
            vp = f" [VID:PID={p.vid:04X}:{p.pid:04X}]"
        print(f"  - {p.device} : {p.description}{vp}")
    return list(ports)


def deteksi_port_esp32(verbose=True):
    """Return nama port ESP32 (skor tertinggi) atau None."""
    ports = serial.tools.list_ports.comports()
    kandidat = []
    for p in ports:
        teks = " ".join([(p.description or ""), (p.manufacturer or ""),
                         (p.hwid or "")]).lower()
        skor = 0
        if p.vid is not None and (p.vid, p.pid) in ESP32_VID_PID:
            skor += 100
        if any(kw in teks for kw in ESP32_KEYWORDS):
            skor += 50
        if "bluetooth" in teks:
            skor -= 200
        if skor > 0:
            kandidat.append((skor, p))

    if not kandidat:
        non_bt = [p for p in ports
                  if "bluetooth" not in (p.description or "").lower()]
        if len(non_bt) == 1:
            if verbose:
                print(f"[i] Hanya ada 1 port non-Bluetooth: {non_bt[0].device}")
            return non_bt[0].device
        if verbose:
            print("[!] Port ESP32 tidak terdeteksi. Gunakan --port COMx.")
        return None

    kandidat.sort(key=lambda x: x[0], reverse=True)
    if verbose:
        print("Kandidat port ESP32 (skor tertinggi = paling mungkin):")
        for skor, p in kandidat:
            print(f"  [{skor:>4}] {p.device} : {p.description}")
    return kandidat[0][1].device


# ============================================================
# KELAS KONTROLER
# ============================================================
class IMCLab:
    """Pembungkus komunikasi serial ke firmware imclab_naik_turun.ino."""

    def __init__(self, ser):
        self.ser = ser

    @classmethod
    def buka(cls, port, baudrate=BAUDRATE):
        try:
            ser = serial.Serial(port, baudrate, timeout=1)
        except serial.SerialException as e:
            print(f"Gagal membuka port {port}: {e}")
            return None
        time.sleep(2)                 # ESP32 reset saat port dibuka
        ser.reset_input_buffer()
        print(f"Terhubung ke ESP32 di {port} @ {baudrate} baud")
        return cls(ser)

    def tutup(self):
        self.ser.close()
        print("Koneksi serial ditutup.")

    # ---------- low level ----------
    def baca_baris(self):
        """Ambil semua baris yang sudah masuk (tidak menunggu)."""
        baris = []
        while self.ser.in_waiting > 0:
            raw = self.ser.readline().decode("utf-8", errors="ignore").strip()
            if raw:
                baris.append(raw)
        return baris

    def kirim(self, perintah, tunggu=0.15, tampil=True):
        """Kirim perintah + newline, kembalikan semua baris balasan."""
        self.ser.write((perintah + "\n").encode("utf-8"))
        self.ser.flush()
        time.sleep(tunggu)
        balasan = self.baca_baris()
        if tampil:
            for b in balasan:
                if not b.startswith("DATA,"):
                    print(f"ESP32: {b}")
        return balasan

    # ---------- perintah ----------
    def motor_on(self, **kw):
        print(">> Motor ON");       return self.kirim("1", **kw)

    def motor_off(self, **kw):
        print(">> Motor OFF");      return self.kirim("0", **kw)

    def naik(self, **kw):
        print(">> Kecepatan +10");  return self.kirim("+", **kw)

    def turun(self, **kw):
        print(">> Kecepatan -10");  return self.kirim("-", **kw)

    def set_pwm(self, duty, **kw):
        duty = max(0, min(255, int(duty)))
        print(f">> Set PWM = {duty}")
        return self.kirim(f"PWM:{duty}", **kw)

    # ---------- parsing ----------
    @staticmethod
    def parse_data(baris):
        """'DATA,ms,duty,rpm' -> (ms, duty, rpm) atau None."""
        if not baris.startswith("DATA,"):
            return None
        try:
            _, ms, duty, rpm = baris.split(",")
            return int(ms), int(duty), float(rpm)
        except ValueError:
            return None

    def monitor(self, detik=5):
        """Tampilkan data RPM selama beberapa detik."""
        akhir = time.time() + detik
        while time.time() < akhir:
            for b in self.baca_baris():
                d = self.parse_data(b)
                if d:
                    print(f"  t={d[0] / 1000:7.1f}s  duty={d[1]:3d}  rpm={d[2]:7.1f}")
            time.sleep(0.05)


# ============================================================
# MODE: DEMO
# ============================================================
def mode_demo(lab):
    print("\n--- Demo urutan kontrol motor ---")
    lab.motor_on();  time.sleep(2)
    for _ in range(3):
        lab.naik();  time.sleep(1)
    for _ in range(2):
        lab.turun(); time.sleep(1)
    lab.set_pwm(150); time.sleep(2)
    lab.motor_off()
    print("--- Demo selesai ---\n")


# ============================================================
# MODE: INTERAKTIF
# ============================================================
def mode_interaktif(lab):
    print("\nMode interaktif:")
    print("  1 = ON | 0 = OFF | + = naik | - = turun")
    print("  p<angka> = set PWM (mis. p150) | r = status | m = monitor 5 dtk")
    print("  q = keluar")
    while True:
        cmd = input("Perintah: ").strip()
        if cmd == "q":
            break
        elif cmd == "1":
            lab.motor_on()
        elif cmd == "0":
            lab.motor_off()
        elif cmd == "+":
            lab.naik()
        elif cmd == "-":
            lab.turun()
        elif cmd.lower() == "r":
            for b in lab.kirim("R", tampil=False):
                d = lab.parse_data(b)
                if d:
                    print(f"  duty={d[1]}  rpm={d[2]:.1f}")
        elif cmd.lower() == "m":
            lab.monitor(5)
        elif cmd.lower().startswith("p") and cmd[1:].isdigit():
            lab.set_pwm(int(cmd[1:]))
        else:
            print("Perintah tidak valid! Gunakan 1/0/+/-/p<angka>/r/m/q.")


# ============================================================
# MODE: EKSPERIMEN (log CSV + grafik)
# ============================================================
def jalankan_eksperimen(lab, hold, outdir):
    """Jalankan profil naik-turun, kembalikan list baris data."""
    langkah = [(d, "naik") for d in PROFIL_NAIK] + \
              [(d, "turun") for d in PROFIL_TURUN]
    total = len(langkah) * hold
    print(f"\n--- Eksperimen naik-turun: {len(langkah)} langkah x {hold} detik "
          f"= {total} detik ---")

    data = []
    t0 = time.time()

    def catat(baris_list, target, fase):
        for b in baris_list:
            d = lab.parse_data(b)
            if d:
                data.append({
                    "t_s": round(time.time() - t0, 2),
                    "esp_ms": d[0],
                    "target_duty": target,
                    "fase": fase,
                    "duty": d[1],
                    "rpm": d[2],
                })

    lab.ser.reset_input_buffer()
    for target, fase in langkah:
        catat(lab.set_pwm(target, tunggu=0.1), target, fase)
        akhir = time.time() + hold
        while time.time() < akhir:
            catat(lab.baca_baris(), target, fase)
            time.sleep(0.05)

    lab.motor_off()
    return data


def simpan_csv(data, outdir):
    outdir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    path = outdir / f"eksperimen_naik_turun_{stamp}.csv"
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["t_s", "esp_ms", "target_duty",
                                          "fase", "duty", "rpm"])
        w.writeheader()
        w.writerows(data)
    print(f"Data tersimpan: {path}")
    return path


def buat_grafik(data, csv_path, tampilkan=False):
    try:
        import matplotlib
        if not tampilkan:
            matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("[!] matplotlib belum terpasang: pip install matplotlib")
        return []

    t = [r["t_s"] for r in data]
    duty = [r["duty"] for r in data]
    rpm = [r["rpm"] for r in data]

    # Grafik 1: duty & RPM terhadap waktu
    fig, ax1 = plt.subplots(figsize=(9, 4.5))
    ax1.step(t, duty, where="post", color="tab:blue", label="Duty (PWM)")
    ax1.set_xlabel("Waktu (s)")
    ax1.set_ylabel("Duty cycle (0-255)", color="tab:blue")
    ax2 = ax1.twinx()
    ax2.plot(t, rpm, color="tab:red", marker="o", ms=3, label="RPM")
    ax2.set_ylabel("RPM", color="tab:red")
    ax1.set_title("Respons kecepatan motor DC: PWM naik-turun")
    ax1.grid(alpha=0.3)
    fig.tight_layout()
    p1 = csv_path.with_name(csv_path.stem + "_waktu.png")
    fig.savefig(p1, dpi=150)

    # Grafik 2: RPM vs duty, pisahkan fase naik dan turun
    fig2, ax = plt.subplots(figsize=(6, 4.5))
    for fase, warna in (("naik", "tab:green"), ("turun", "tab:orange")):
        xs = [r["duty"] for r in data if r["fase"] == fase]
        ys = [r["rpm"] for r in data if r["fase"] == fase]
        ax.scatter(xs, ys, s=18, color=warna, label=f"Fase {fase}")
    ax.set_xlabel("Duty cycle (0-255)")
    ax.set_ylabel("RPM")
    ax.set_title("RPM vs Duty (naik vs turun)")
    ax.grid(alpha=0.3)
    ax.legend()
    fig2.tight_layout()
    p2 = csv_path.with_name(csv_path.stem + "_rpm_vs_duty.png")
    fig2.savefig(p2, dpi=150)

    print(f"Grafik tersimpan: {p1}\n                  {p2}")
    if tampilkan:
        plt.show()
    return [p1, p2]


def ringkasan(data):
    """Rata-rata RPM per langkah (mengabaikan 1 sampel pertama = transien)."""
    print("\nRingkasan per langkah (rata-rata RPM, sampel pertama diabaikan):")
    print(f"{'Fase':<6} {'Duty':>5} {'N':>3} {'RPM rata-rata':>14}")
    kunci = []
    for r in data:
        k = (r["fase"], r["target_duty"])
        if not kunci or kunci[-1] != k:
            kunci.append(k)
    for fase, duty in kunci:
        nilai = [r["rpm"] for r in data
                 if r["fase"] == fase and r["target_duty"] == duty]
        pakai = nilai[1:] if len(nilai) > 1 else nilai
        if pakai:
            print(f"{fase:<6} {duty:>5} {len(pakai):>3} "
                  f"{sum(pakai) / len(pakai):>14.1f}")


def mode_eksperimen(lab, hold, outdir, tampilkan):
    data = jalankan_eksperimen(lab, hold, outdir)
    if not data:
        print("[!] Tidak ada baris DATA yang diterima. Pastikan firmware "
              "imclab_naik_turun.ino yang ter-upload.")
        return
    path = simpan_csv(data, outdir)
    ringkasan(data)
    buat_grafik(data, path, tampilkan)


# ============================================================
# MAIN
# ============================================================
def main():
    ap = argparse.ArgumentParser(description="Kontrol iMCLab via Serial")
    ap.add_argument("--mode", choices=["interaktif", "demo", "eksperimen"],
                    default="interaktif")
    ap.add_argument("--port", help="mis. COM11 atau /dev/ttyUSB0 "
                                   "(default: auto-detect)")
    ap.add_argument("--baud", type=int, default=BAUDRATE)
    ap.add_argument("--hold", type=float, default=5.0,
                    help="lama tiap langkah eksperimen (detik)")
    ap.add_argument("--outdir", default="data", help="folder output CSV/PNG")
    ap.add_argument("--show", action="store_true",
                    help="tampilkan grafik di layar")
    args = ap.parse_args()

    print("=" * 50)
    list_serial_ports()
    print("=" * 50)

    port = args.port or deteksi_port_esp32()
    if port is None:
        sys.exit("Program dihentikan: port ESP32 tidak ditemukan.")
    print(f"\n>> Port terpilih: {port}")

    lab = IMCLab.buka(port, args.baud)
    if lab is None:
        sys.exit("Koneksi gagal. Periksa PORT dan kabel USB.")

    try:
        if args.mode == "demo":
            mode_demo(lab)
        elif args.mode == "eksperimen":
            mode_eksperimen(lab, args.hold, Path(args.outdir), args.show)
        else:
            mode_interaktif(lab)
    except KeyboardInterrupt:
        print("\nDihentikan oleh pengguna.")
    finally:
        try:
            lab.motor_off()
        except Exception:
            pass
        lab.tutup()


if __name__ == "__main__":
    main()
