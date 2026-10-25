/*********************************************************
 * MODUL 2 - iMCLab dikendalikan dari Python (Serial)
 * ESP32 + Motor DC + sensor RPM
 *
 * Perintah (diakhiri newline '\n', 115200 baud):
 *   1        -> Motor ON (pakai duty terakhir, default 200)
 *   0        -> Motor OFF
 *   +        -> Naikkan duty +10
 *   -        -> Turunkan duty -10
 *   PWM:xxx  -> Set duty langsung (0-255)
 *   R        -> Kirim status sekarang
 *
 * Output otomatis tiap 1 detik (untuk dibaca Python):
 *   DATA,<millis>,<duty>,<rpm>
 *********************************************************/

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

int dutyCycle = 0;
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
void applyMotor()
{
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  pwmWrite(dutyCycle);
  digitalWrite(ledPin, dutyCycle > 0 ? HIGH : LOW);
}

void MotorOn()
{
  if (dutyCycle == 0) dutyCycle = DUTY_DEFAULT;
  applyMotor();
  Serial.print("Motor ON, duty = ");
  Serial.println(dutyCycle);
}

void MotorOff()
{
  dutyCycle = 0;
  applyMotor();
  Serial.println("Motor OFF, duty = 0");
}

void naikkanKecepatan()
{
  dutyCycle = constrain(dutyCycle + DUTY_STEP, DUTY_MIN, DUTY_MAX);
  applyMotor();
  Serial.print("Kecepatan NAIK, duty = ");
  Serial.println(dutyCycle);
}

void turunkanKecepatan()
{
  dutyCycle = constrain(dutyCycle - DUTY_STEP, DUTY_MIN, DUTY_MAX);
  applyMotor();
  Serial.print("Kecepatan TURUN, duty = ");
  Serial.println(dutyCycle);
}

void setDuty(int val)
{
  dutyCycle = constrain(val, DUTY_MIN, DUTY_MAX);
  applyMotor();
  Serial.print("Set duty = ");
  Serial.println(dutyCycle);
}

void kirimData()
{
  Serial.print("DATA,");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(dutyCycle);
  Serial.print(",");
  Serial.println(rpm, 1);
}

// Hitung RPM + kirim DATA tiap 1 detik
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
    kirimData();
  }
}

// ============================================================
void prosesPerintah(String cmd)
{
  cmd.trim();
  if (cmd.length() == 0) return;

  String up = cmd;
  up.toUpperCase();

  if (up.startsWith("PWM:"))
  {
    setDuty(cmd.substring(4).toInt());
  }
  else if (cmd == "1")      MotorOn();
  else if (cmd == "0")      MotorOff();
  else if (cmd == "+")      naikkanKecepatan();
  else if (cmd == "-")      turunkanKecepatan();
  else if (up == "R")       kirimData();
  else
  {
    Serial.print("Perintah tidak dikenal: ");
    Serial.println(cmd);
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
  dutyCycle = 0;
  applyMotor();              // motor mati saat start
  oldTime = millis();

  Serial.println("ESP32 Motor Controller siap.");
  Serial.println("Perintah: 1=ON, 0=OFF, +=naik, -=turun, PWM:xxx=set, R=status");
}

void loop()
{
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
