/*********************************************************
 * MODUL 4 (versi sederhana) - OTA via Blynk.Air
 * ESP32 DevKit V1 (kit iMCLab)
 *
 * Fokus: membuktikan firmware bisa diperbarui lewat WiFi.
 * Tidak ada kontrol motor / RPM di sini.
 *
 * Perubahan yang terlihat untuk membuktikan OTA berhasil:
 *   Versi 1.0.0 -> LED (GPIO 2) berkedip LAMBAT (1000 ms)
 *   Versi 1.1.0 -> LED berkedip CEPAT  (200 ms)
 * Untuk membuat versi 1.1.0: ubah BLYNK_FIRMWARE_VERSION
 * dan BLINK_MS di bawah, lalu Export Compiled Binary.
 *
 * Kredensial ada di secrets.h (salin dari secrets.example.h)
 *********************************************************/

// ====== BAGIAN YANG DIUBAH ANTAR VERSI ======
#define BLYNK_FIRMWARE_VERSION "1.0.0"    // 1.0.0 => 1.1.0 
const unsigned long BLINK_MS = 1000;      // 1000 => 200
// ============================================

#define BLYNK_TEMPLATE_ID   "TMPL6aEYbvyrN"
#define BLYNK_TEMPLATE_NAME "New Template 1"

#include "secrets.h"            // BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <BlynkSimpleEsp32.h>

const int ledPin = 2;
unsigned long lastBlink = 0;
bool ledState = false;

// ------------------------------------------------------------
// Handler OTA: dipanggil otomatis saat firmware dikirim dari
// Blynk.Air. URL berisi token, jadi tidak dicetak ke Serial.
// ------------------------------------------------------------
void otaGagal(const char* alasan)
{
  Serial.print("[OTA] GAGAL: ");
  Serial.println(alasan);
  Blynk.connect();
}

BLYNK_WRITE(InternalPinOTA)
{
  Serial.println("[OTA] Perintah update diterima");
  String url = param.asString();

  Blynk.disconnect();                 // jangan ganggu proses unduh

  HTTPClient http;
  WiFiClientSecure secureClient;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (url.startsWith("https"))
  {
    secureClient.setInsecure();       // cukup untuk praktikum
    http.begin(secureClient, url);
  }
  else
  {
    http.begin(url);
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    Serial.print("[OTA] HTTP code = ");
    Serial.println(httpCode);
    http.end();
    otaGagal("HTTP bukan 200");
    return;
  }

  int contentLength = http.getSize();
  Serial.print("[OTA] Ukuran firmware (byte) = ");
  Serial.println(contentLength);
  if (contentLength <= 0)
  {
    http.end();
    otaGagal("ukuran firmware tidak valid");
    return;
  }

  if (!Update.begin(contentLength))
  {
    Update.printError(Serial);
    http.end();
    otaGagal("partisi OTA tidak cukup / skema partisi tanpa OTA");
    return;
  }

  Serial.println("[OTA] Mengunduh dan menulis firmware...");
  size_t written = Update.writeStream(*http.getStreamPtr());

  if (written != (size_t)contentLength)
  {
    Update.printError(Serial);
    http.end();
    otaGagal("byte tertulis tidak sama dengan ukuran firmware");
    return;
  }
  if (!Update.end() || !Update.isFinished())
  {
    Update.printError(Serial);
    http.end();
    otaGagal("finalisasi update gagal");
    return;
  }

  http.end();
  Serial.println("[OTA] Update berhasil. Reboot...");
  delay(500);
  ESP.restart();
}

BLYNK_CONNECTED()
{
  Serial.println("[Blynk] Terhubung ke server");
  Blynk.virtualWrite(V4, BLYNK_FIRMWARE_VERSION);   // opsional: datastream String V4
}

// ------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  Serial.println();
  Serial.print("Firmware versi ");
  Serial.println(BLYNK_FIRMWARE_VERSION);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Menyambung WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000)
  {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("WiFi terhubung, IP: ");
    Serial.println(WiFi.localIP());
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(10000);
  }
  else
  {
    Serial.println("WiFi GAGAL - OTA tidak bisa dilakukan");
  }
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED) Blynk.run();

  // Kedip LED tanpa delay() supaya Blynk tetap responsif
  if (millis() - lastBlink >= BLINK_MS)
  {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
  }
}
