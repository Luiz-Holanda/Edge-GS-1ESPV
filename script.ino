#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22

#define LED_VERDE 13
#define LED_AMARELO 12
#define LED_VERMELHO 11

#define BUZZER 8

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte fogo[8] = {
  B00100,
  B00100,
  B01110,
  B01110,
  B11111,
  B01110,
  B00100,
  B00000
};

void setup() {

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  noTone(BUZZER);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, fogo);
  dht.begin();

  lcd.setCursor(0, 0);
  lcd.print("Monitoramento");
  lcd.setCursor(0, 1);
  lcd.print("Queimadas");
  delay(2000);

  lcd.clear();
}

void loop() {

  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();

  if (isnan(umidade) || isnan(temperatura)) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro Sensor");

    delay(2000);
    return;
  }
String risco;

int pontosTemp = 0;
int pontosUmidade = 0;
if (temperatura >= 20) pontosTemp = 1;
if (temperatura >= 25) pontosTemp = 2;
if (temperatura >= 30) pontosTemp = 3;
if (temperatura >= 35) pontosTemp = 4;
if (temperatura >= 40) pontosTemp = 5;

if (umidade <= 60) pontosUmidade = 1;
if (umidade <= 50) pontosUmidade = 2;
if (umidade <= 40) pontosUmidade = 3;
if (umidade <= 30) pontosUmidade = 4;
if (umidade <= 25) pontosUmidade = 5;

int riscoTotal = pontosTemp + pontosUmidade;

// BAIXO
if (riscoTotal <= 2) {

  risco = "BAIXO";

  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  noTone(BUZZER);
}

// MÉDIO
else if (riscoTotal <= 4) {

  risco = "MEDIO";

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, HIGH);
  digitalWrite(LED_VERMELHO, LOW);

  noTone(BUZZER);
}

// ALTO
else if (riscoTotal <= 6) {

  risco = "ALTO";

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, HIGH);

  noTone(BUZZER);
}

// CRÍTICO
else {

  risco = "CRITICO";

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, HIGH);

  tone(BUZZER, 1000);
}

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperatura, 1);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(9, 0);
  lcd.print("U:");
  lcd.print(umidade, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Risco:");
  lcd.print(risco);
  lcd.setCursor(15, 1);

if (risco == "CRITICO") {

  if ((millis() / 500) % 2 == 0) {
    lcd.write(byte(0)); // fogo
  } else {
    lcd.print(" ");
  }

} else {
  lcd.print(" ");
}

  delay(2000);
}