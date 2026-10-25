# Robotika — UTS

Empat modul percobaan dengan kit iMCLab (ESP32 + motor DC).

| Modul | Topik | Kode | Laporan |
|---|---|---|---|
| 1 | Pemrograman dasar (ON/OFF, PWM, RPM) | `Modul1_Dasar_Motor/on_off_pwm_motor/` | `Laporan_Modul1.md` |
| 2 | Python + serial (naik-turun) | `Modul2_Python/` | `Laporan_Modul2.md` |
| 3 | Berbasis Blynk | `Modul3_Blynk/imclab_blynk/` | `Laporan_Modul3.md` |
| 4 | OTA Blynk.Air | `Modul4_OTA/imclab_blynk_ota/` | `Laporan_Modul4.md` |

## Penting sebelum `git push`

Modul 3 dan 4 memakai `secrets.h` (token Blynk dan WiFi). Salin `secrets.example.h` menjadi `secrets.h` di folder sketch masing-masing. File ini sudah ada di `.gitignore`, jadi tidak ikut ter-commit.

## Checklist UTS

- [ ] Modul 1: upload, uji, isi tabel duty-RPM, screenshot, laporan
- [ ] Modul 2: jalankan `--mode eksperimen`, simpan CSV dan grafik, laporan
- [ ] Modul 3: dashboard Blynk, uji switch dan slider, screenshot, laporan
- [ ] Modul 4: OTA 1.0.0 → 1.1.0, log dan screenshot, laporan
- [ ] Semua `[ISI` di laporan sudah terisi
- [ ] Push ke GitHub di `Robotika/UTS/`, lalu tempel link ke spreadsheet pengumpulan
