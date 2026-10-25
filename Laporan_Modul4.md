# Laporan Riset Modul 4 — Pemrograman OTA Berbasis Blynk

> Cari teks `[ISI` (Ctrl+F) untuk menemukan bagian yang masih harus kamu isi sendiri.
> Token Blynk dan email pada screenshot di folder `gambar/` sudah disensor. Password WiFi tidak muncul di screenshot mana pun.

| Nama / NIM | Effandy Perwira Dwi Putra/23081010308 |
|---|---|
| Kelompok | 1 |
| Tanggal percobaan | 26 Oktober 2026 (pukul ±00.31–00.37) |
| Kode program | `ota_blynk_sederhana/ota_blynk_sederhana.ino` |

## 1. Tujuan

1. Memahami konsep pembaruan firmware jarak jauh (Over-The-Air).
2. Menerapkan OTA pada ESP32 iMCLab menggunakan Blynk.Air.
3. Membuktikan firmware berpindah dari versi 1.0.0 ke 1.1.0 tanpa kabel USB.

## 2. Dasar Teori (ringkas)

- **OTA** — Over-The-Air (OTA) adalah pembaruan firmware perangkat melalui jaringan nirkabel tanpa menyambungkan kabel USB. Manfaatnya terutama untuk perangkat yang tersebar atau sulit dijangkau secara fisik: perbaikan bug dan fitur baru dapat dikirim dari jarak jauh, ke banyak perangkat sekaligus, tanpa mendatangi tiap perangkat.
- **Alur OTA ESP32** — firmware baru diunduh lewat jaringan, ditulis ke partisi aplikasi cadangan (`ota_0`/`ota_1`), lalu bootloader berpindah ke partisi baru saat reboot.
- **Blynk.Air** — layanan Blynk untuk mengirim firmware ke perangkat dari jarak jauh. Firmware (`.bin`) diunggah ke konsol, lalu dibuat *shipment* (pengiriman) ke template, segmen device, dan waktu tertentu, dengan syarat versi tertentu. `BLYNK_FIRMWARE_VERSION` adalah string versi yang ada di dalam firmware dan dilaporkan perangkat ke server, sehingga server dapat membandingkan versi perangkat dengan versi di shipment. `InternalPinOTA` adalah virtual pin internal yang menerima URL unduhan firmware dari server; sketch menangkapnya dengan `BLYNK_WRITE(InternalPinOTA)`.
- **Partition scheme** — skema "no OTA" tidak punya ruang untuk update. Skema yang dipakai harus memiliki dua partisi aplikasi; karena OTA pada percobaan ini berhasil, skema yang dipakai mendukung OTA. [ISI: nama skema persis dari Tools → Partition Scheme, mis. *Default 4MB with spiffs*]
- Referensi:
  1. Blynk Documentation (Blynk.Air / OTA). https://docs.blynk.io
  2. Espressif, *ESP-IDF OTA Updates API*. https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html
  3. Arduino-ESP32, library `Update`. https://github.com/espressif/arduino-esp32/tree/master/libraries/Update

## 3. Alat dan Bahan

| No | Alat/Bahan | Keterangan |
|---|---|---|
| 1 | Kit iMCLab (ESP32 + motor DC) | DOIT ESP32 DEVKIT V1 |
| 2 | Akun Blynk dengan Blynk.Air | Terdapat menu Blynk.Air pada akun Blynk |
| 3 | Arduino IDE | versi IDE 2.3.10 dan board manager esp32 by espressif versi 2.0.11 |

## 4. Prosedur

1. Siapkan template dan device Blynk (sama dengan Modul 3), salin `secrets.example.h` menjadi `secrets.h` lalu isi token dan WiFi. Atur Partition Scheme yang mendukung OTA (mis. *Default 4MB with spiffs*).
2. Upload `ota_blynk_sederhana.ino` **via USB** dengan `BLYNK_FIRMWARE_VERSION "1.0.0"` dan `BLINK_MS = 1000`. Amati LED berkedip lambat dan catat log Serial Monitor.
3. Ubah kode: `BLYNK_FIRMWARE_VERSION "1.1.0"` dan `BLINK_MS = 200`.
4. Arduino IDE: **Sketch → Export Compiled Binary**, ambil file `.bin` (jangan yang `bootloader`/`partitions`).
5. Di Blynk Console, buka menu Blynk.Air (nama menu dapat sedikit berbeda antar versi konsol), unggah `.bin`, buat pengiriman (shipment) ke device.
6. Amati Serial Monitor (log `[OTA]`), tunggu reboot, lalu cek versi dan kedipan LED.

## 5. Hasil Percobaan

### 5.1 Versi awal (USB)

![Dashboard Blynk: Firmware Version 1.0.0, device Online](gambar/01_versi_1_0_0.png)

Setelah upload via USB, device berstatus *Online* dan widget Firmware Version (V4) menampilkan 1.0.0. Nilai ini dikirim sketch lewat `Blynk.virtualWrite(V4, BLYNK_FIRMWARE_VERSION)` di `BLYNK_CONNECTED()`.

| Item | Nilai |
|---|---|
| Versi | 1.0.0 |
| Kedipan LED | lambat (1000 ms) |
| Ukuran sketch (dari IDE) | [ISI: angka "Sketch uses ... bytes" dari output compile; sebagai pembanding, file .bin v1.1.0 berukuran 910.128 byte] |

### 5.2 Persiapan firmware baru

![Export Compiled Binary / file .bin](gambar/02_export_bin.png)

Hasil *Export Compiled Binary* berada di folder `build/esp32.esp32.esp32doit-devkit-v1` dan berisi lima file. Yang diunggah ke Blynk.Air hanya `ota_blynk_sederhana.ino.bin`; `bootloader.bin` (18 KB) dan `partitions.bin` (3 KB) tidak dipakai.

| Item | Nilai |
|---|---|
| Versi | 1.1.0 |
| Perubahan | `BLINK_MS` 1000 → 200 |
| Nama file `.bin` | `ota_blynk_sederhana.ino.bin` |
| Ukuran `.bin` | 889 KB di File Explorer (910.128 byte menurut Blynk.Air) |
| Waktu build | 26/10/2026 00.31 |

### 5.3 Pengiriman via Blynk.Air

![Halaman Blynk.Air: upload dan shipment](gambar/03_blynk_air.png)

Memilih template yang akan di update, memilih device yang akan di update, drag & drop file '.bin' hasil export 'Export Compiled Binary' ke Blynk Air.

Pengaturan shipment pada gambar: template *New Template 1*, device segment *All devices*, preferred time *Any*, dan syarat *Apply update if the device has: Lower firmware version*. Blynk.Air membaca metadata firmware yang diunggah: Version 1.1.0, Hardware ESP32, Firmware Type MCU, ukuran 888 KB (910.128 byte), build 12:31 AM. Device yang dipilih (*New Template 1*, status online) masih berversi 1.0.0. Pengiriman dimulai dengan tombol *Start Shipping*.

### 5.4 Log OTA di Serial Monitor

![Update details dari Blynk.Air](gambar/04_log_ota.png)

Gambar di atas adalah riwayat *update details* dari Blynk.Air (waktu mengikuti jam server/konsol), bukan keluaran Serial Monitor. Pada firmware, tahapan yang sama dicetak di Serial sebagai `[OTA] Perintah update diterima`, `[OTA] Mengunduh dan menulis firmware...`, dan `[OTA] Update berhasil. Reboot...`.

| Waktu | Kejadian |
|---|---|
| 00.36.19 | *Firmware Shipment Process Started* dan *Request sent*: shipment dimulai, perintah dikirim ke device |
| 00.36.20 | *New Firmware Requested*: perintah update diterima, device meminta firmware (±1 detik setelah shipment dimulai) |
| 00.36.43 | *Firmware Downloaded*: unduh & tulis firmware selesai (±23 detik) |
| 00.36.43–00.36.46 | Finalisasi dan reboot (tidak dicatat terpisah oleh Blynk.Air; selang ±3 detik) |
| 00.36.46 | *Success*: versi baru aktif dan dilaporkan ke server |

Total waktu OTA: 27 detik (00.36.19 → 00.36.46). Tahap terlama adalah unduh dan tulis firmware (±23 detik untuk 910.128 byte, sekitar 40 KB/s).

### 5.5 Verifikasi versi baru

![Dashboard Blynk: Firmware Version 1.1.0](gambar/05_versi_1_1_0.png)

| Item | Sebelum | Sesudah |
|---|---|---|
| Versi | 1.0.0 | 1.1.0 |
| Kedipan LED | lambat (1000 ms) | cepat (200 ms) |
| Device Online di Blynk dan V4 menampilkan versi? | ya | ya |

## 6. Analisis

1. **Mengapa nomor versi harus dinaikkan? Apa yang terjadi bila sama?** Nomor versi menjadi identitas firmware yang berjalan: perangkat melaporkannya ke server dan Blynk.Air membandingkannya dengan versi di shipment. Pada percobaan ini syaratnya *Lower firmware version*, sehingga hanya device berversi lebih rendah dari 1.1.0 yang menerima update. Bila versi tidak dinaikkan (sama), device tidak memenuhi syarat sehingga tidak diperbarui, dan kita juga tidak dapat membuktikan versi mana yang sedang berjalan karena V4 menampilkan angka yang sama sebelum dan sesudah update.
2. **Mengapa koneksi Blynk diputus (`Blynk.disconnect()`) sebelum firmware diunduh, dan mengapa LED dikedipkan tanpa `delay()`?** Unduhan memakai koneksi HTTP(S) terpisah dari koneksi Blynk. Memutus Blynk membebaskan bandwidth dan memori agar unduhan tidak terganggu (sesuai komentar kode), dan karena handler OTA berjalan di dalam callback Blynk yang memblokir `loop()` selama unduh, koneksi Blynk yang dibiarkan hidup tidak dapat dilayani sehingga rawan terputus. Jika unduhan gagal, `otaGagal()` memanggil `Blynk.connect()` untuk menyambung lagi. LED dikedipkan dengan `millis()`, bukan `delay()`, karena `delay()` memblokir `loop()` sehingga `Blynk.run()` jarang dipanggil, yang dapat membuat koneksi Blynk terlambat dilayani dan perintah OTA terlewat. Dengan `millis()`, `Blynk.run()` tetap dipanggil terus.
3. **Apa risiko OTA dan bagaimana ESP32 menguranginya?** Risiko utama: jaringan putus saat unduh, daya mati saat menulis, dan firmware yang salah atau rusak. ESP32 menulis firmware ke partisi aplikasi cadangan sementara firmware lama di partisi aktif tidak disentuh. Pindah partisi baru dilakukan hanya setelah penulisan selesai dan image divalidasi, sehingga jika daya mati di tengah penulisan perangkat tetap boot ke firmware lama. Jika jaringan putus, sketch ini mendeteksinya (byte tertulis ≠ ukuran firmware), membatalkan update, dan menyambung ulang ke Blynk. Risiko yang tidak ditangani sketch ini adalah firmware baru yang berhasil terpasang tetapi tidak bisa tersambung (mis. WiFi salah) karena tidak ada mekanisme *rollback* otomatis; dalam kasus itu perangkat harus diperbaiki lewat USB.
4. **Bila gagal, di tahap mana?** Berdasarkan kode, ada lima titik gagal. Pada percobaan ini semuanya terlewati (riwayat Blynk.Air berakhir *Success*).

   | Tahap | Penyebab umum | Pesan di Serial |
   |---|---|---|
   | HTTP | URL kedaluwarsa, server atau jaringan bermasalah | `HTTP code = ...`, `HTTP bukan 200` |
   | Ukuran | respons server tidak menyertakan ukuran | `ukuran firmware tidak valid` |
   | `Update.begin` | partisi OTA tidak cukup atau skema partisi tanpa OTA | `partisi OTA tidak cukup / skema partisi tanpa OTA` |
   | Penulisan | jaringan putus atau timeout 15 detik saat streaming | `byte tertulis tidak sama dengan ukuran firmware` |
   | Finalisasi | image rusak atau tidak valid | `finalisasi update gagal` |

5. **Bandingkan OTA dengan upload USB dari sisi waktu dan kepraktisan.** OTA lebih praktis ketika perangkat berada jauh dari developer atau jumlahnya banyak, karena developer tidak perlu mendatangi perangkat. Dari sisi waktu, OTA pada percobaan ini memerlukan 27 detik untuk mengirim 910 KB, ditambah langkah manual (export `.bin`, upload ke konsol, membuat shipment). Waktu upload USB tidak diukur pada percobaan ini; untuk satu kit di meja, USB umumnya sama cepat atau lebih cepat. Keunggulan OTA bukan kecepatan, melainkan tanpa akses fisik dan bisa diterapkan ke banyak device sekaligus.

## 7. Kendala dan Solusi

| Kendala | Solusi |
|---|---|
| Folder `build` berisi beberapa file (`.bin`, `bootloader.bin`, `partitions.bin`, `.elf`, `.map`) sehingga mudah salah memilih file | Hanya `ota_blynk_sederhana.ino.bin` yang diunggah ke Blynk.Air |
| [ISI: kendala lain yang kamu alami selama praktikum, jika ada] | [ISI] |

## 8. Kesimpulan

Firmware iMCLab berhasil diperbarui dari versi 1.0.0 ke 1.1.0 melalui Blynk.Air tanpa kabel USB, dibuktikan oleh riwayat update yang berakhir *Success* dan widget Firmware Version yang berubah menjadi 1.1.0. Seluruh proses memakan waktu 27 detik, dengan unduh dan tulis firmware (910.128 byte) sebagai tahap terlama. Kenaikan nomor versi dan syarat *Lower firmware version* menentukan device mana yang menerima update, sedangkan ESP32 menulis ke partisi cadangan sehingga firmware lama tetap aman bila update gagal. OTA paling bermanfaat untuk perangkat yang jauh atau banyak, meski sketch ini belum memiliki mekanisme rollback.

## 9. Lampiran

- Kode: [ISI: link GitHub] (tanpa `secrets.h`)
- Berkas `.bin` (opsional): `ota_blynk_sederhana.ino.bin` (889 KB) [ISI: link jika diunggah]
