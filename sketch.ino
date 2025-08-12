#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Pin tanımlamaları
#define DHTPIN 15         
#define DHTTYPE DHT22

#define FAN_PIN 2         
#define PUMP_PIN 4        
#define LDR_PIN 34        
#define LED_PIN 25        
#define BUZZER_PIN 12     

#define BUTTON_FAN_PIN 19     
#define BUTTON_PUMP_PIN 18   

#define CO2_SENSOR_PIN 35     // MQ-135 AO pini (analog)

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 4); // 4 satırlı LCD

int currentHour = 19;
int co2Threshold = 3500;  // CO2 eşik seviyesi

// Manuel kontrol durumları
bool manualFanState = false;
bool manualPumpState = false;

// Debounce değişkenleri
bool lastFanButtonState = HIGH;
bool lastPumpButtonState = HIGH;
unsigned long lastFanDebounceTime = 0;
unsigned long lastPumpDebounceTime = 0;
const unsigned long debounceDelay = 200;

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(BUTTON_FAN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PUMP_PIN, INPUT_PULLUP);
  pinMode(CO2_SENSOR_PIN, INPUT);

  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Akilli Sera");
  delay(3000);
  lcd.clear();
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int co2Value = analogRead(CO2_SENSOR_PIN);  // CO2 sensor degeri

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT verisi okunamiyor!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor hatasi");
    delay(3000);
    return;
  }

  int ldrValue = analogRead(LDR_PIN);

  Serial.println("-----------");
  Serial.print("Saat: "); Serial.println(currentHour);
  Serial.print("Sicaklik: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Nem: ");Serial.print(" %"); Serial.println(humidity); 
  Serial.print("Isik (LDR): "); Serial.println(ldrValue);
  Serial.print("CO2: "); Serial.println(co2Value);

  // LCD Göstergesi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sicaklik:");
  lcd.print(temperature);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Nem: ");
  lcd.print(" %");
   lcd.print(humidity);

  lcd.setCursor(0, 2);
  lcd.print("Fan:");
  lcd.print((digitalRead(FAN_PIN) ? "ON " : "OFF "));
  lcd.print("Pompa:");
  lcd.print((digitalRead(PUMP_PIN) ? "ON " : "OFF "));

  lcd.setCursor(0, 3);
  lcd.print("CO2: ");
  lcd.print(co2Value);

  // FAN Buton Kontrolü
  bool fanReading = digitalRead(BUTTON_FAN_PIN);
  if (fanReading == LOW && lastFanButtonState == HIGH && (millis() - lastFanDebounceTime) > debounceDelay) {
    manualFanState = !manualFanState;
    Serial.println(manualFanState ? "Manuel Fan Acik" : "Manuel Fan Kapali");
    lastFanDebounceTime = millis();
  }
  lastFanButtonState = fanReading;

  // POMPA Buton Kontrolü
  bool pumpReading = digitalRead(BUTTON_PUMP_PIN);
  if (pumpReading == LOW && lastPumpButtonState == HIGH && (millis() - lastPumpDebounceTime) > debounceDelay) {
    manualPumpState = !manualPumpState;
    Serial.println(manualPumpState ? "Manuel Pompa Acik" : "Manuel Pompa Kapali");
    lastPumpDebounceTime = millis();
  }
  lastPumpButtonState = pumpReading;

  // FAN KONTROLÜ
  if ((temperature > 30 || co2Value > co2Threshold) && !manualFanState) {
    digitalWrite(FAN_PIN, HIGH);
    Serial.println("Fan Acildi (Sicaklik/CO2)");
  } else if (manualFanState) {
    digitalWrite(FAN_PIN, HIGH);
    Serial.println("Fan Acik (Manuel)");
  } else {
    digitalWrite(FAN_PIN, LOW);
    Serial.println("Fan Kapali");
  }

  // POMPA KONTROLÜ
  if (humidity < 40 && !manualPumpState) {
    digitalWrite(PUMP_PIN, HIGH);
    Serial.println("Sulama Acildi (Oto)");
  } else if (manualPumpState) {
    digitalWrite(PUMP_PIN, HIGH);
    Serial.println("Sulama Acik (Manuel)");
  } else {
    digitalWrite(PUMP_PIN, LOW);
    Serial.println("Sulama Kapali");
  }

  // Aydınlatma
  if (ldrValue < 1000 || (currentHour >= 18 || currentHour < 6)) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Aydinlatma Acildi");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("Aydinlatma Kapali");
  }

 // Buzzer Uyarısı: Sadece DÜŞÜK CO2 durumu
if (co2Value < 3500) {
  tone(BUZZER_PIN, 1000);
  delay(1000);
  noTone(BUZZER_PIN);
  Serial.println("CO2 DUSUK! Lutfen CO2 tupu ekleyin.");
}


  currentHour++;
  if (currentHour >= 24) currentHour = 0;

  delay(3000);
}