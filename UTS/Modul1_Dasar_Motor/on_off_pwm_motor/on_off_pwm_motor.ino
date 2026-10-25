/*********************************************************
 * MODUL 1 - Pemrograman Dasar Robotika dengan Kit iMCLab
 * ESP32 + Motor DC (driver H-bridge) + sensor RPM
 *
 * Pengembangan dari on_off_Motor.ino:
 *   - ON/OFF (seperti sebelumnya)
 *   - PWM bertingkat (LOW/MID/HIGH/FULL) untuk melihat
 *     pengaruh duty cycle terhadap kecepatan motor
 *   - Pembacaan RPM dari sensor (pin 13)
 *   - Mode SWEEP otomatis -> tabel duty vs RPM untuk laporan
 *
 * Perintah Serial (tanpa perlu newline, 115200 baud):
 *   1 = Motor ON (duty 205)      0 = Motor OFF
 *   L = duty 100   M = duty 150   H = duty 200   F = duty 255
 *   S = SWEEP otomatis           R = tampilkan status
 *   ? = bantuan
 *********************************************************/

// ---------------- Pin ----------------
const int  ledPin     = 2;
const int  motor1Pin1 = 27;   // arah motor
const int  motor1Pin2 = 26;   // arah motor
const int  enable1Pin = 12;   // PWM kecepatan
const byte pin_rpm    = 13;   // sensor RPM (encoder/optocoupler)

// ---------------- PWM ----------------
const int freq       = 30000;
const int pwmChannel = 0;     // dipakai pada ESP32 core 2.x
const int resolution = 8;     // duty 0-255

const int DUTY_ON   = 205;
const int DUTY_LOW  = 100;
const int DUTY_MID  = 150;
const int DUTY_HIGH = 200;
const int DUTY_FULL = 255;

int dutyCycle = 0;

// ---------------- Sensor RPM ----------------
const int HOLES = 2;                       // jumlah lubang piringan encoder
const unsigned long DEBOUNCE_US = 500;
volatile unsigned int  rev = 0;
volatile unsigned long lastPulseTime = 0;

float rpm = 0.0;
unsigned long oldTime = 0;

// ---------------- Mode SWEEP ----------------
const int SWEEP_LEVELS[] = {50, 100, 150, 200, 255};
const int SWEEP_COUNT = sizeof(SWEEP_LEVELS) / sizeof(SWEEP_LEVELS[0]);
const unsigned long SWEEP_STEP_MS = 5000;  // lama tiap level
bool sweepActive = false;
int  sweepIndex = 0;
unsigned long sweepStart = 0;

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

// ============================================================
// PWM: kompatibel ESP32 Arduino core 2.x dan 3.x
// ============================================================
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
void setMotor(int duty)
{
  dutyCycle = constrain(duty, 0, 255);
  digitalWrite(motor1Pin1, HIGH);   // arah maju
  digitalWrite(motor1Pin2, LOW);
  pwmWrite(dutyCycle);
  digitalWrite(ledPin, dutyCycle > 0 ? HIGH : LOW);
}

void tampilkanStatus(const char* aksi)
{
  Serial.print(aksi);
  Serial.print(" | duty=");
  Serial.print(dutyCycle);
  Serial.print(" (");
  Serial.print(dutyCycle * 100.0 / 255.0, 1);
  Serial.print("%) | RPM=");
  Serial.println(rpm, 1);
}

void tampilkanBantuan()
{
  Serial.println("=== Modul 1: Kontrol Dasar Motor ===");
  Serial.println("1=ON  0=OFF  L/M/H/F=duty 100/150/200/255");
  Serial.println("S=sweep otomatis  R=status  ?=bantuan");
}

// ============================================================
// Hitung RPM tiap 1 detik
// Format log: LOG,<millis>,<duty>,<rpm>
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

    if (dutyCycle > 0 || rpm > 0)
    {
      Serial.print("LOG,");
      Serial.print(cur);
      Serial.print(",");
      Serial.print(dutyCycle);
      Serial.print(",");
      Serial.println(rpm, 1);
    }
  }
}

// ============================================================
// SWEEP otomatis: tiap level ditahan SWEEP_STEP_MS
// Hasil tiap level dicetak sebagai: DATA,<duty>,<rpm>
// ============================================================
void startSweep()
{
  sweepActive = true;
  sweepIndex = 0;
  sweepStart = millis();
  setMotor(SWEEP_LEVELS[0]);
  Serial.println("SWEEP mulai (tiap level 5 detik). Kirim '0' untuk membatalkan.");
  Serial.println("DATA,duty,rpm");
}

void updateSweep()
{
  if (!sweepActive) return;
  if (millis() - sweepStart < SWEEP_STEP_MS) return;

  // catat hasil level ini (RPM = jendela 1 detik terakhir)
  Serial.print("DATA,");
  Serial.print(dutyCycle);
  Serial.print(",");
  Serial.println(rpm, 1);

  sweepIndex++;
  if (sweepIndex >= SWEEP_COUNT)
  {
    sweepActive = false;
    setMotor(0);
    Serial.println("SWEEP selesai. Motor OFF.");
  }
  else
  {
    setMotor(SWEEP_LEVELS[sweepIndex]);
    sweepStart = millis();
  }
}

// ============================================================
void handleCommand(char c)
{
  c = toupper(c);

  if (c == '1')      { sweepActive = false; setMotor(DUTY_ON);   tampilkanStatus("Motor ON"); }
  else if (c == '0') { sweepActive = false; setMotor(0);         tampilkanStatus("Motor OFF"); }
  else if (c == 'L') { sweepActive = false; setMotor(DUTY_LOW);  tampilkanStatus("Level LOW"); }
  else if (c == 'M') { sweepActive = false; setMotor(DUTY_MID);  tampilkanStatus("Level MID"); }
  else if (c == 'H') { sweepActive = false; setMotor(DUTY_HIGH); tampilkanStatus("Level HIGH"); }
  else if (c == 'F') { sweepActive = false; setMotor(DUTY_FULL); tampilkanStatus("Level FULL"); }
  else if (c == 'S') { startSweep(); }
  else if (c == 'R') { tampilkanStatus("Status"); }
  else if (c == '?') { tampilkanBantuan(); }
  else
  {
    Serial.print("Perintah tidak dikenal: ");
    Serial.println(c);
  }
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
  setMotor(0);               // motor mati saat start
  oldTime = millis();

  tampilkanBantuan();
}

void loop()
{
  hitungRPM();
  updateSweep();

  while (Serial.available() > 0)
  {
    char c = Serial.read();
    if (c == '\n' || c == '\r' || c == ' ') continue;
    handleCommand(c);
  }
}
