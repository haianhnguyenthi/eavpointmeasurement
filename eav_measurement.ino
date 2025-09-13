#include <Arduino.h>

#define diagnoseReadPin A3
#define buzzerPin 6
#define analogCurrentPin A7
#define electrodePin 10
#define polarizationPin 2
#define modeRelayPin 11
#define modeTherapyDiagnoseRealyPin 3
#define switchModePin 12

// ==== Vref = 5V DEFAULT ====
#define ONE_BIT_VOLTAGE 4.8876   // mV/bit (5000 / 1023)

// ===== Hệ số tuyến tính mV -> Acu (fit lại 5 điểm) =====
#define ACU_A 0.03469304f   // slope
#define ACU_B 27.92447f     // intercept

#define VERBOSE 0

long inputVoltage = 0;   // mV
int  score200 = 0;       // Acu (đã làm tròn)
bool started = false;

void beep(int millis){
  digitalWrite(buzzerPin, HIGH);
  delay(millis);
  digitalWrite(buzzerPin, LOW);
}

long measureVoltage(){
  long sum = 0;
  for (int i = 0; i < 8; i++){
    sum += analogRead(diagnoseReadPin);
  }
  int raw = sum >> 3;
  return (long)(raw * ONE_BIT_VOLTAGE + 0.5);
}

// Tuyến tính: Acu = A * mV + B, làm tròn int để in
static inline int mvToAcu(long mv){
  float acu = ACU_A * (float)mv + ACU_B;
  return (int)(acu + 0.5f);
}

void setup() {
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  pinMode(electrodePin, OUTPUT);
  pinMode(polarizationPin, OUTPUT);
  pinMode(modeRelayPin, OUTPUT);
  pinMode(modeTherapyDiagnoseRealyPin, OUTPUT);
  pinMode(switchModePin, INPUT_PULLUP);

  analogReference(DEFAULT); // 5V

  Serial.begin(115200);
  Serial.setTimeout(20); // tránh chờ lâu khi đọc lệnh
  beep(100);

  // ổn định ADC
  analogRead(diagnoseReadPin);
  analogRead(diagnoseReadPin);

  digitalWrite(modeRelayPin, LOW);
  digitalWrite(modeTherapyDiagnoseRealyPin, HIGH);

  Serial.println("miniVOLL EAV (filter by e-score: 0<e<200 and exclude 41±2)");
  Serial.println(">");
}

void loop() {
  inputVoltage = measureVoltage();
  score200     = mvToAcu(inputVoltage);

  // Điều kiện hợp lệ:
  //  - e nằm trong (0, 200)
  //  - loại trừ vùng e=41 ±2 => [39..43]
  bool valid = (score200 > 0 && score200 < 200) &&
               !(score200 >= 40 && score200 <= 42);

  if (valid) {
    if (!started) {
      started = true;
      Serial.println(":estart");
    }
    // Khung gọn: ':e<score> <mV>'
    Serial.print(":e");
    Serial.print(score200);
    Serial.print(' ');
    Serial.println(inputVoltage);

#if VERBOSE
    Serial.print("V="); Serial.print(inputVoltage);
    Serial.print("mV  S="); Serial.println(score200);
#endif
  } else {
    if (started) {
      started = false;
      Serial.println(":stop");
    }
  }

//  // --- NHẬN LỆNH TỪ PYTHON: kêu 1 tiếng bíp ---
//  while (Serial.available()) {
//    String cmd = Serial.readStringUntil('\n');
//    cmd.trim();
//    if (cmd.length() == 0) continue;
//    if (cmd.equalsIgnoreCase("BEEP")) {
//      beep(200);   // bíp 200 ms
//    }
//  }

  delay(200);
}
 
