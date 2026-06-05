#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <Keypad.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define BUZZER 8
#define PINO_FITA 13
#define NUM_LEDS 7

const byte LINHAS = 4;
const byte COLUNAS = 4;

Adafruit_NeoPixel fita(NUM_LEDS, PINO_FITA, NEO_GRB + NEO_KHZ800);
bool alarmeSilenciado = false;
bool mostrarQueimada = true;
unsigned long ultimaTrocaTela = 0;
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);
unsigned long ultimaAtualizacaoLCD = 0;
float tempMin = 0;
float tempMax = 0;
float umiMin = 0;
float umiMax = 0;

bool configurado = false;

String entrada = "";

int etapaConfiguracao = 0;

char teclas[LINHAS][COLUNAS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte pinosLinhas[LINHAS] = {3,4,5,6};
byte pinosColunas[COLUNAS] = {7,9,10,11};

Keypad keypad = Keypad(
  makeKeymap(teclas),
  pinosLinhas,
  pinosColunas,
  LINHAS,
  COLUNAS
);

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
int calcularRiscoQueimada(float temperatura, float umidade) {

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

    return pontosTemp + pontosUmidade;
  }
String obterStatusTemp(float temperatura) {

    if (temperatura < tempMin)
      return "Temp Baixa";

    if (temperatura > tempMax)
      return "Temp Alta";

    return "Temp Ideal";
}
String obterStatusUmi(float umidade) {

    if (umidade < umiMin)
      return "Umidade Baixa";

    if (umidade > umiMax)
      return "Umidade Alta";

    return "Umidade Ideal";
}
String obterRisco(int riscoTotal) {

    if (riscoTotal <= 2)
      return "BAIXO";

    if (riscoTotal <= 4)
      return "MEDIO";

    if (riscoTotal <= 6)
      return "ALTO";

    return "CRITICO";
}
void atualizarBarra(int riscoTotal) {

  fita.clear();

  int ledsAcesos = min(riscoTotal, 7);

  bool piscar = false;

  if (riscoTotal >= 7) {
    piscar = ((millis() / 500) % 2 == 0);
  }

  if (riscoTotal >= 7 && !piscar) {
    fita.show();
    return;
  }

  for (int i = 0; i < ledsAcesos; i++) {

    // LEDs 0 e 1 → Verde
    if (i <= 1) {
      fita.setPixelColor(i, fita.Color(0, 255, 0));
    }

    // LEDs 2 e 3 → Amarelo
    else if (i <= 3) {
      fita.setPixelColor(i, fita.Color(255, 255, 0));
    }

    // LEDs 4,5,6 → Vermelho
    else {
      fita.setPixelColor(i, fita.Color(255, 0, 0));
    }
  }

  fita.show();
}
void reiniciarConfiguracao() {

  tempMin = 0;
  tempMax = 0;
  umiMin = 0;
  umiMax = 0;

  entrada = "";
  etapaConfiguracao = 0;
  configurado = false;

  alarmeSilenciado = false;

  noTone(BUZZER);

  lcd.clear();
  lcd.print("Temp MIN planta");
}
void configurarPlanta(char tecla) {

  if (!tecla) return;

  if (tecla == 'B') {
    reiniciarConfiguracao();
    return;
  }

  if (tecla >= '0' && tecla <= '9') {

    int posPonto = entrada.indexOf('.');

    // Se já existe ponto, limita a 1 casa decimal
    if (posPonto != -1) {

      if (entrada.length() - posPonto >= 2)
        return;
    }

    entrada += tecla;

    lcd.setCursor(0,2);
    lcd.print("                    ");
    lcd.setCursor(0,2);
    lcd.print("Valor: ");
    lcd.print(entrada);
  }

  if (tecla == '#') {
    float valor = entrada.toFloat();
    // Temperatura
    if ((etapaConfiguracao == 0 || etapaConfiguracao == 1) &&
      (valor < -10 || valor > 60)) {

      lcd.clear();
    lcd.print("Temp invalida");
    delay(1500);

    lcd.clear();

    if (etapaConfiguracao == 0)
     lcd.print("Temp MIN planta");
    else
      lcd.print("Temp MAX planta");

    entrada = "";
    return;
    }

    if ((etapaConfiguracao == 2 || etapaConfiguracao == 3) &&
    (valor < 0 || valor > 100)) {

      lcd.clear();
      lcd.print("Umidade invalida");
      delay(1500);

      lcd.clear();

      if (etapaConfiguracao == 2)
        lcd.print("Umidade MIN");
      else
        lcd.print("Umidade MAX");

      entrada = "";
      return;
    }

    switch(etapaConfiguracao) {

      case 0:
        tempMin = valor;
        break;

      case 1:
        if (valor <= tempMin) {

          lcd.clear();
          lcd.print("Max > Min");
          delay(1500);
          lcd.clear();
          lcd.print("Temp MAX planta");

          entrada = "";
          return;
        }
        tempMax = valor;
        break;

      case 2:
        umiMin = valor;
        break;

      case 3:
        if (valor <= umiMin) {

          lcd.clear();
          lcd.print("Max > Min");
          delay(1500);

          lcd.clear();
          lcd.print("Umidade MAX");

          entrada = "";
          return;
        }
        umiMax = valor;
        configurado = true;
        lcd.clear();
        return;
  
    }
    entrada = "";
    etapaConfiguracao++;

    lcd.clear();

    if (etapaConfiguracao == 1)
      lcd.print("Temp MAX planta");

    else if (etapaConfiguracao == 2)
      lcd.print("Umidade MIN");

    else if (etapaConfiguracao == 3)
      lcd.print("Umidade MAX");
  }

  if (tecla == '*') {

    if (entrada.indexOf('.') == -1) {

      entrada += '.';

      lcd.setCursor(0,2);
      lcd.print("                    ");
      lcd.setCursor(0,2);
      lcd.print("Valor: ");
      lcd.print(entrada);
    }
  }
  if (tecla == 'A') {

    if (entrada.length() > 0) {

      entrada.remove(entrada.length() - 1);

      lcd.setCursor(0,2);
      lcd.print("                    ");
      lcd.setCursor(0,2);
      lcd.print("Valor: ");
      lcd.print(entrada);
    }
  }




}

void atualizarLCD(
  float temperatura,
  float umidade,
  String risco,
  String statusTemp,
  String statusUmi
) {

  if (millis() - ultimaAtualizacaoLCD < 1000)
    return;

  ultimaAtualizacaoLCD = millis();

  // Linha 0
  lcd.setCursor(0,0);
  lcd.print("                    ");
  lcd.setCursor(0,0);
  lcd.print("Temperatura:");
  lcd.print(temperatura,1);
  lcd.print((char)223);
  lcd.print("C");

  // Linha 1
  lcd.setCursor(0,1);
  lcd.print("                    ");
  lcd.setCursor(0,1);
  lcd.print("Umidade:");
  lcd.print(umidade,0);
  lcd.print("%");

  // Limpa linhas inferiores
  lcd.setCursor(0,2);
  lcd.print("                    ");

  lcd.setCursor(0,3);
  lcd.print("                    ");

  if (mostrarQueimada) {

    lcd.setCursor(0,2);
    lcd.print("Risco Queimada:");

    lcd.setCursor(0,3);
    lcd.print(risco);

  } else {

    lcd.setCursor(0,2);
    lcd.print("Planta: ");
    lcd.print(statusTemp);

    lcd.setCursor(0,3);
    lcd.print(statusUmi);
  }

  lcd.setCursor(19,3);

  if (risco == "CRITICO")
    lcd.write(byte(0));
  else
    lcd.print(" ");
}
void setup() {
  fita.begin();
  fita.clear();
  fita.show();

  lcd.init();
  lcd.backlight();

  lcd.createChar(0, fogo);

  dht.begin();

  noTone(BUZZER);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Monitoramento");
  lcd.setCursor(0,1);
  lcd.print("Temperatura e");
  lcd.setCursor(0,2);
  lcd.print("Umidade");
  delay(5000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("* = ponto decimal");
  lcd.setCursor(0,1);
  lcd.print("# = enviar valor");
  delay(5000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("A = apagar");
  lcd.setCursor(0,1);
  lcd.print("digito");
  lcd.setCursor(0,2);
  lcd.print("B = resetar");
  lcd.setCursor(0,3);
  lcd.print("configuracao");
  delay(5000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("D = mutar");
  lcd.setCursor(0,1);
  lcd.print("buzzer");
  delay(5000);

  lcd.clear();
  lcd.print("Temp MIN planta");
}

void loop() {
  char tecla = keypad.getKey();

  if (tecla == 'B') {
    reiniciarConfiguracao();
    return;
  }


  if (!configurado) {

    configurarPlanta(tecla);
    return;
  }
if (tecla) {
  Serial.print("Tecla: ");
  Serial.println(tecla);
}
  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();
  if (millis() - ultimaTrocaTela >= 3000) {

    mostrarQueimada = !mostrarQueimada;
    ultimaTrocaTela = millis();
  }
  if (isnan(umidade) || isnan(temperatura)) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro Sensor");

    delay(100);
    return;
  }
  int riscoTotal = calcularRiscoQueimada(temperatura, umidade);
  String statusTemp = obterStatusTemp(temperatura);
  String statusUmi = obterStatusUmi(umidade);
  String risco = obterRisco(riscoTotal);
  atualizarBarra(riscoTotal);

  if (risco == "CRITICO" && !alarmeSilenciado)
    tone(BUZZER, 1000);
  else
    noTone(BUZZER);
  if (risco != "CRITICO")
    alarmeSilenciado = false;
  
    atualizarLCD(
    temperatura,
    umidade,
    risco,
    statusTemp,
    statusUmi
  );
  if (tecla == 'D' && risco == "CRITICO") {

    alarmeSilenciado = true;
    noTone(BUZZER);
  }
}