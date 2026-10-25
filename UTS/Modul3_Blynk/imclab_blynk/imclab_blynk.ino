/*********************************************************
 * MODUL 3 - iMCLab + ESP32 + Blynk IoT
 * Monitoring RPM dan kontrol PWM/ON-OFF dari aplikasi Blynk
 *
 * Datastream Blynk (sama dengan template "IMCLab Test"):
 *   V0 : RPM          (ESP32 -> Blynk)  Gauge / Chart
 *   V1 : Duty aktual  (ESP32 -> Blynk)  Label / Value display
 *   V2 : Slider PWM   (Blynk <-> ESP32) 0-255
 *   V3 : Switch ON/OFF(Blynk <-> ESP32) 0/1
 *
 * Kredensial TIDAK ditulis di sini. Salin secrets.example.h
 * menjadi secrets.h lalu isi token + WiFi (secrets.h tidak
 * ikut ke GitHub karena sudah masuk .gitignore).
 *********************************************************/

#define BLYNK_TEMPLATE_ID   "TMPL641PfBifp"
#define BLYNK_TEMPLATE_NAME "New Template"

#include "secrets.h"            // BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ---------------- Pin ----------------
const int  ledPin     = 2;
const int  motor1Pin1 = 27;
const int  motor1Pin2 = 26;
const int  enable1Pin = 12;
const byte pin_rpm    = 13;

// ---------------- PWM ----------------
const int freq       = 30000;
const int pwmChannel = 0;
const int resolution = 8;

int  dutyCycle = 0;
bool motorEnable = false;
const int DUTY_DEFAULT = 200;
const int DUTY_STEP    = 10;
const int DUTY_MIN     = 0;
const int DUTY_MAX     = 255;

// ---------------- Sensor RPM ----------------
const int HOLES = 2;
const unsigned long DEBOUNCE_US = 500;
volatile unsigned int  rev = 0;
volatile unsigned long lastPulseTime = 0;

float rpm = 0.0;
unsigned long oldTime = 0;

String inputString = "";
BlynkTimer timer;

// ============================================================
void IRAM_ATTR isr()
{
  unsigned long now = micros();
  if (now - lastPulseTime > DEBOUNCE_US)
  {
    rev++;
    lastPulseTime = now;
  }
}

// PWM kompatibel ESP32 core 2.x dan 3.x
void pwmSetup()
{
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(enable1Pin, freq, resolution);
#else
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(enable1Pin, pwmChannel);
#endif
}

void pwmWrite(int duty)
{
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(enable1Pin, duty);
#else
  ledcWrite(pwmChannel, duty);
#endif
}

// ============================================================
// Terapkan duty ke motor + sinkronkan widget Blynk
// ============================================================
void applyMotor()
{
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  pwmWrite(dutyCycle);
  digitalWrite(ledPin, dutyCycle > 0 ? HIGH : LOW);

  Blynk.virtualWrite(V1, dutyCycle);
  Blynk.virtualWrite(V2, dutyCycle);
  Blynk.virtualWrite(V3, dutyCycle > 0);
}

void tampilkanStatus(const char* aksi)
{
  Serial.print(aksi);
  Serial.print(" | PWM=");
  Serial.print(dutyCycle);
  Serial.print(" | RPM=");
  Serial.println(rpm, 1);
}

// ============================================================
// Hitung RPM tiap 1 detik
// ============================================================
void hitungRPM()
{
  unsigned long cur = millis();
  unsigned long dt = cur - oldTime;
  if (dt >= 1000)
  {
    noInterrupts();
    unsigned int pulsa = rev;
    rev = 0;
    interrupts();

    rpm = ((float)pulsa / HOLES) * (60000.0 / (float)dt);
    oldTime = cur;

    Serial.print("DATA,");
    Serial.print(cur);
    Serial.print(",");
    Serial.print(dutyCycle);
    Serial.print(",");
    Serial.println(rpm, 1);
  }
}

// Kirim RPM + duty ke Blynk tiap 1 detik
// (V2/V3 sengaja tidak dikirim berkala agar tidak "melawan" slider)
void kirimKeBlynk()
{
  Blynk.virtualWrite(V0, rpm);
  Blynk.virtualWrite(V1, dutyCycle);
}

// ============================================================
// Callback Blynk
// ============================================================
BLYNK_CONNECTED()
{
  Serial.println("[Blynk] Terhubung ke server");
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V3);
}

// Slider PWM (V2)
BLYNK_WRITE(V2)
{
  dutyCycle = constrain(param.asInt(), DUTY_MIN, DUTY_MAX);
  motorEnable = (dutyCycle > 0);
  applyMotor();

  Serial.print("PWM dari Blynk = ");
  Serial.println(dutyCycle);
}

// Tombol ON/OFF (V3)
BLYNK_WRITE(V3)
{
  motorEnable = param.asInt();

  if (motorEnable)
  {
    if (dutyCycle == 0) dutyCycle = DUTY_DEFAULT;
  }
  else
  {
    dutyCycle = 0;
  }
  applyMotor();

  Serial.print("Motor ");
  Serial.println(motorEnable ? "ON" : "OFF");
}

// ============================================================
// Perintah Serial (opsional, untuk debugging)
// ============================================================
void prosesPerintah(String cmd)
{
  cmd.trim();
  if (cmd.length() == 0) return;

  String up = cmd;
  up.toUpperCase();

  if (up.startsWith("PWM:"))
  {
    dutyCycle = constrain(cmd.substring(4).toInt(), DUTY_MIN, DUTY_MAX);
    motorEnable = (dutyCycle > 0);
    applyMotor();
    tampilkanStatus("Set PWM");
  }
  else if (cmd == "1")
  {
    if (dutyCycle == 0) dutyCycle = DUTY_DEFAULT;
    motorEnable = true;
    applyMotor();
    tampilkanStatus("Motor ON");
  }
  else if (cmd == "0")
  {
    dutyCycle = 0;
    motorEnable = false;
    applyMotor();
    tampilkanStatus("Motor OFF");
  }
  else if (cmd == "+")
  {
    dutyCycle = constrain(dutyCycle + DUTY_STEP, DUTY_MIN, DUTY_MAX);
    motorEnable = (dutyCycle > 0);
    applyMotor();
    tampilkanStatus("PWM NAIK");
  }
  else if (cmd == "-")
  {
    dutyCycle = constrain(dutyCycle - DUTY_STEP, DUTY_MIN, DUTY_MAX);
    motorEnable = (dutyCycle > 0);
    applyMotor();
    tampilkanStatus("PWM TURUN");
  }
  else if (up == "R")
  {
    tampilkanStatus("Status");
  }
  else
  {
    Serial.print("Perintah tidak dikenal: ");
    Serial.println(cmd);
  }
}

// ============================================================
// WiFi (dengan batas waktu supaya tidak menggantung)
// ============================================================
bool sambungWiFi(unsigned long batasMs)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Menyambung WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < batasMs)
  {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("WiFi terhubung, IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("WiFi GAGAL (motor tetap bisa dikontrol lewat Serial)");
  return false;
}

// ============================================================
void setup()
{
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  pinMode(pin_rpm, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pin_rpm), isr, FALLING);

  pwmSetup();
  dutyCycle = 0;
  applyMotor();                 // motor mati saat start
  oldTime = millis();

  if (sambungWiFi(15000))
  {
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(10000);
  }

  timer.setInterval(1000L, kirimKeBlynk);

  Serial.println("================================");
  Serial.println("iMCLab + Blynk siap");
  Serial.println("Serial: 1=ON 0=OFF +/- PWM:xxx R=status");
  Serial.println("================================");
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Blynk.run();
  }
  timer.run();
  hitungRPM();

  while (Serial.available() > 0)
  {
    char c = Serial.read();
    if (c == '\n' || c == '\r')
    {
      prosesPerintah(inputString);
      inputString = "";
    }
    else
    {
      inputString += c;
    }
  }
}
