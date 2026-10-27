/*
  Smart Breath-Sensing Mask - ESP32 firmware (research prototype, NOT a medical device)

  Libraries (Arduino Library Manager):
    NimBLE-Arduino (h2zero) 1.4.x   | Adafruit BME680 | Adafruit SHT31 | Adafruit ADS1X15
  Board: "ESP32 Dev Module"

  SIMULATE = 1  -> no sensors needed; synthetic exhale curves (for demo/testing the pipeline)
  SIMULATE = 0  -> real sensors (see wiring in README)

  Output lines (BLE notify + Serial 115200), newline terminated:
    L,ms,mox_mV,gas_uS,no_mV,rh,state     live sample (10 Hz)
    F,f0..f14                             feature vector after each exhale (order = FEATURES in ml/train.py)
    E,message                             info / error
*/
#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>

#define SIMULATE 1

#if !SIMULATE
#include <Adafruit_BME680.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_ADS1X15.h>
#endif

// ---------- pins ----------
#define PIN_MOX    34   // MEMS MOX sensor analog out (ADC1)
#define PIN_PRESS  35   // pressure sensor analog out via divider (ADC1)
#define PIN_PUMP   25   // optional purge pump (via MOSFET)

// ---------- timing / thresholds ----------
#define SAMPLE_MS      100      // 10 Hz
#define BASELINE_MS    6000
#define PURGE_MS       10000
#define MIN_CAP_MS     3000
#define MAX_CAP_MS     12000
#define PRESS_THR_MV   40.0f    // exhale detected when pressure rises this much above baseline
#define MAXN           (MAX_CAP_MS / SAMPLE_MS + 5)

struct Sample { float mox, gas, no, rh, temp, press; };
enum State { BASELINE = 0, ARMED = 1, CAPTURE = 2, PURGE = 3 };

// ---------- BLE (Nordic UART UUIDs) ----------
#define SVC_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define RX_UUID  "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   // write from phone
#define TX_UUID  "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   // notify to phone

NimBLECharacteristic *txChar;
volatile bool bleConnected = false;
String pendingCmd = "";

class SrvCb : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *s, NimBLEConnInfo &info) override { bleConnected = true; }
  void onDisconnect(NimBLEServer *s, NimBLEConnInfo &info, int reason) override {
    bleConnected = false;
    NimBLEDevice::getAdvertising()->start();
  }
};
class RxCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &info) override {
    pendingCmd = String(c->getValue().c_str()); pendingCmd.trim();
  }
};

void sendLine(const String &s) {
  Serial.println(s);
  if (!bleConnected) return;
  String line = s + "\n";
  for (size_t i = 0; i < line.length(); i += 20) {        // 20-byte chunks work with default MTU
    String part = line.substring(i, min((size_t)line.length(), i + 20));
    txChar->setValue((uint8_t *)part.c_str(), part.length());
    txChar->notify();
    delay(6);
  }
}

// ---------- sensors ----------


#if SIMULATE
bool simActive = false; uint32_t simStart = 0; int simClass = 0;
float noise(float a) { return a * (random(-1000, 1000) / 1000.0f); }
Sample readSensors() {
  Sample s;
  float t = simActive ? (millis() - simStart) / 1000.0f : 0;
  if (simActive && t > 16) simActive = false;
  float k = simActive ? (1 - expf(-t / 2.0f)) * (t < 8 ? 1.0f : expf(-(t - 8) / 2.5f)) : 0;
  float amp = simClass ? 1.4f : 1.0f;                     // class 1 = "outside reference" proxy
  s.mox = 900 + 110 * amp * k + noise(3);
  s.gas = 40 + 6 * amp * k + noise(0.2f);
  s.no = -20 - 8 * amp * k + noise(0.3f);
  s.rh = 45 + 35 * k + noise(0.5f);
  s.temp = 28 + 5 * k + noise(0.1f);
  s.press = 1000 + ((simActive && t < 8) ? 80 : 0) + noise(2);
  return s;
}
void sensorsBegin() { randomSeed(esp_random()); }
#else
Adafruit_BME680 bme; Adafruit_SHT31 sht; Adafruit_ADS1115 ads;
float lastGasOhm = 100000; bool bmeBusy = false; uint32_t bmeEnd = 0;
void sensorsBegin() {
  Wire.begin();
  if (!bme.begin()) sendLine("E,BME688 not found");
  bme.setTemperatureOversampling(BME680_OS_2X); bme.setHumidityOversampling(BME680_OS_NONE);
  bme.setPressureOversampling(BME680_OS_NONE); bme.setGasHeater(320, 150);
  if (!sht.begin(0x44)) sendLine("E,SHT31 not found");
  if (!ads.begin()) sendLine("E,ADS1115 not found");
  ads.setGain(GAIN_FOUR);                                  // +-1.024 V
  analogSetAttenuation(ADC_11db);
}
Sample readSensors() {
  Sample s;
  if (!bmeBusy) { bmeEnd = bme.beginReading(); bmeBusy = true; }
  else if (millis() >= bmeEnd) { if (bme.endReading()) lastGasOhm = bme.gas_resistance; bmeBusy = false; }
  s.gas = 1e6f / max(lastGasOhm, 1.0f);                    // conductance (uS): rises with VOCs
  s.mox = analogReadMilliVolts(PIN_MOX);
  s.press = analogReadMilliVolts(PIN_PRESS);
  s.no = ads.computeVolts(ads.readADC_Differential_0_1()) * 1000.0f;   // WE-AE (mV), electrochemical NO
  float t, h; if (!sht.readBoth(&t, &h)) { t = NAN; h = NAN; }
  s.temp = t; s.rh = h;
  return s;
}
#endif

// ---------- state machine ----------

State state = BASELINE; uint32_t stateStart = 0;
float base[4], bsum[4]; int bn = 0; float pBase = 0, pSum = 0;
float buf[MAXN][5]; int n = 0;                              // mox, gas, no, rh, temp

void enter(State s) {
  state = s; stateStart = millis();
  if (s == BASELINE) { bn = 0; for (int i = 0; i < 4; i++) bsum[i] = 0; pSum = 0; digitalWrite(PIN_PUMP, LOW); }
  if (s == ARMED) sendLine("E,ready - exhale into the mask");
  if (s == CAPTURE) { n = 0; }
  if (s == PURGE) { digitalWrite(PIN_PUMP, HIGH); }
}

// features for channel c (0 mox, 1 gas, 2 no): peak, auc, slope, tpeak
void channelFeatures(int c, float *out) {
  float peak = 0, auc = 0, slope = 0, tpeak = 0, prev = 0;
  for (int i = 0; i < n; i++) {
    float d = buf[i][c] - base[c];
    if (fabsf(d) > fabsf(peak)) { peak = d; tpeak = i * SAMPLE_MS / 1000.0f; }
    auc += d * SAMPLE_MS / 1000.0f;
    if (i > 0) { float sl = (d - prev) / (SAMPLE_MS / 1000.0f); if (fabsf(sl) > fabsf(slope)) slope = sl; }
    prev = d;
  }
  out[0] = peak; out[1] = auc; out[2] = slope; out[3] = tpeak;
}

void finishCapture() {
  if (n * SAMPLE_MS < MIN_CAP_MS) { sendLine("E,exhale too short - discard"); enter(PURGE); return; }
  float f[15]; for (int c = 0; c < 3; c++) channelFeatures(c, &f[c * 4]);
  float rh = 0, tp = 0; for (int i = 0; i < n; i++) { rh += buf[i][3]; tp += buf[i][4]; }
  f[12] = rh / n; f[13] = tp / n; f[14] = n * SAMPLE_MS / 1000.0f;
  String s = "F"; for (int i = 0; i < 15; i++) { s += ","; s += String(f[i], 3); }
  sendLine(s);
  enter(PURGE);
}

void handleCmd() {
  if (pendingCmd.length() == 0) return;
  String c = pendingCmd; pendingCmd = "";
#if SIMULATE
  if (c == "SIM0") { simClass = 0; sendLine("E,sim class 0 (reference-like)"); }
  if (c == "SIM1") { simClass = 1; sendLine("E,sim class 1 (altered-like)"); }
  if (c == "TRIG" && state == ARMED) { simActive = true; simStart = millis(); }
#endif
  if (c == "RESET") enter(BASELINE);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PUMP, OUTPUT); digitalWrite(PIN_PUMP, LOW);
  sensorsBegin();
  NimBLEDevice::init("BreathMask");
  NimBLEServer *srv = NimBLEDevice::createServer(); srv->setCallbacks(new SrvCb());
  NimBLEService *svc = srv->createService(SVC_UUID);
  txChar = svc->createCharacteristic(TX_UUID, NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic *rx = svc->createCharacteristic(RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rx->setCallbacks(new RxCb());
  svc->start();
  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SVC_UUID); adv->start();
  enter(BASELINE);
}

void loop() {
  static uint32_t next = 0;
  if (Serial.available()) { pendingCmd = Serial.readStringUntil('\n'); pendingCmd.trim(); }
  handleCmd();
  if (millis() < next) return;
  next = millis() + SAMPLE_MS;

  Sample s = readSensors();
  sendLine("L," + String(millis()) + "," + String(s.mox, 1) + "," + String(s.gas, 2) + "," +
           String(s.no, 2) + "," + String(s.rh, 1) + "," + String((int)state));

  uint32_t el = millis() - stateStart;
  switch (state) {
    case BASELINE:
      bsum[0] += s.mox; bsum[1] += s.gas; bsum[2] += s.no; pSum += s.press; bn++;
      if (el >= BASELINE_MS && bn > 0) {
        for (int i = 0; i < 3; i++) base[i] = bsum[i] / bn;
        pBase = pSum / bn; enter(ARMED);
      }
      break;
    case ARMED:
      if (s.press - pBase > PRESS_THR_MV) enter(CAPTURE);
      break;
    case CAPTURE:
      if (n < MAXN) { buf[n][0] = s.mox; buf[n][1] = s.gas; buf[n][2] = s.no; buf[n][3] = s.rh; buf[n][4] = s.temp; n++; }
      if ((s.press - pBase < PRESS_THR_MV * 0.5f && el > 1000) || el >= MAX_CAP_MS) finishCapture();
      break;
    case PURGE:
      if (el >= PURGE_MS) enter(BASELINE);
      break;
  }
}
