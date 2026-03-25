#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// --- CZUJNIKI I PINS ---
#define DHTPIN 2
#define DHTTYPE DHT11
#define LM35_PIN A3
#define LIGHT_SENSOR_PIN A2
#define WATER_SENSOR_PIN A1
#define SOIL_SENSOR_PIN_ANALOG A0  
#define PUMP_PIN 12
#define BUZZER_PIN 9

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long previousMillis = 0;
const unsigned long interval = 1000;
const float TEMP_OFFSET = 0.0;

float readLM35Trimmed(int pin) {
  const int samples = 10;
  int readings[samples];
  int sum = 0;
  int minVal = 1023;
  int maxVal = 0;

  for (int i = 0; i < samples; i++) {
    readings[i] = analogRead(pin);
    sum += readings[i];
    if (readings[i] < minVal) minVal = readings[i];
    if (readings[i] > maxVal) maxVal = readings[i];
    delay(5);
  }

  sum = sum - minVal - maxVal;
  float avg = sum / float(samples - 2);
  float voltage = avg * (5.0 / 1023.0);
  return voltage * 100.0 + TEMP_OFFSET;
}

float readLux(int pin) {
  const float Res0 = 1.0;
  int reading = analogRead(pin);
  float Vout = reading * 0.0048828125;
  if (Vout <= 0.0) return 0.0;
  return 500.0 / (Res0 * ((5.0 - Vout) / Vout));
}

void playMarioMelody() {
  int melody[] = {
    660, 660, 0, 660, 0, 523, 660, 0, 784, 0, 392, 0,
    523, 0, 392, 0, 330, 0, 440, 0, 494, 0, 466, 440,
    0, 392, 660, 784, 880, 0, 698, 784, 0, 660, 523,
    587, 494, 0, 523, 0, 392, 0, 330, 0, 440, 0
  };

  int durations[] = {
    100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 300, 100,
    300, 100, 300, 100, 300, 100, 300, 100, 100, 100, 100, 200,
    100, 100, 100, 100, 300, 100, 100, 100, 300, 100, 100,
    100, 100, 300, 100, 300, 100, 300, 100, 300, 100
  };

  int notesCount = sizeof(melody) / sizeof(int);
  for (int i = 0; i < notesCount; i++) {
    if (melody[i] != 0) {
      tone(BUZZER_PIN, melody[i], durations[i]);
    }
    delay(durations[i] + 30);
    noTone(BUZZER_PIN);
  }
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(PUMP_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH); 
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float temperatureC = readLM35Trimmed(LM35_PIN);
    float humidity = dht.readHumidity();
    float lux = readLux(LIGHT_SENSOR_PIN);
    int waterLevel = analogRead(WATER_SENSOR_PIN);

    int soilValue = analogRead(SOIL_SENSOR_PIN_ANALOG);
    float soilPercent = 100.0 - ((soilValue - 210) / 458.0) * 100.0;
    if (soilPercent > 100.0) soilPercent = 100.0;
    if (soilPercent < 0.0) soilPercent = 0.0;

    // Sterowanie pompą na podstawie wilgotności i poziomu wody
    if (soilPercent < 60.0 && waterLevel >= 630) {
      digitalWrite(PUMP_PIN, LOW);  // Pompa włączona (LOW = ON)
    } else {
      digitalWrite(PUMP_PIN, HIGH); // Pompa wyłączona (HIGH = OFF)
    }

    // Alarm niskiego poziomu wody
    if (waterLevel < 630) {
      playMarioMelody();
    }

    // LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperatureC, 1);
    lcd.print((char)223);
    lcd.print("C R.H:");
    lcd.print(humidity, 0);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Lux:");
    lcd.print(lux, 0);
    lcd.print("  S.M:");
    lcd.print(soilPercent, 0);
    lcd.print("%");

    // Serial debug
    Serial.print("Temp: "); Serial.print(temperatureC); Serial.print(" C, ");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.print(" %, ");
    Serial.print("Lux: "); Serial.print(lux); Serial.print(" lux, ");
    Serial.print("Woda: "); Serial.print(waterLevel);
    Serial.print(", Gleba analog: ");
    Serial.print(soilValue);
    Serial.print(" ("); Serial.print(soilPercent, 1); Serial.print("%), ");
    Serial.print("Pompa: ");
    Serial.println((soilPercent < 60.0 && waterLevel >= 630) ? "ON" : "OFF");
  }

  delay(500);
}
