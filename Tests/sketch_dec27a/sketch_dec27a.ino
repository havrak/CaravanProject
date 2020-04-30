  

#include <OneWire.h>

#include <DallasTemperature.h>
t

 

const int pinCidlaDS = 13;               

 

OneWire oneWireDS(pinCidlaDS);

DallasTemperature senzoryDS(&oneWireDS);

               

int analogPin = 35; // potentiometer wiper (middle terminal) connected to analog pin 3

                    // outside leads to ground and +5V

int val = 0;  // variable to store the value read

int LevelHigh = 0;

long PulseCount = 0;

 

 

void setup() {

  M5.begin();

  M5.Lcd.setTextColor(TFT_YELLOW);

  M5.Lcd.setFreeFont(FSB12);   

  Serial.begin(115200);           //  setup serial

  pinMode(12, OUTPUT);             // rele ventil(HIGH vypnuto, LOW zapnuto

  pinMode(15, OUTPUT);             // rele vyhrev(HIGH vypnuto, LOW zapnuto

  digitalWrite(12, LOW);

  digitalWrite(15, LOW);

  pinMode(36, INPUT);             // snímač hladiny Horni, LOW sepnuto

  pinMode(34, INPUT);             // snímač hladiny spodní, LOW sepnuto

  pinMode(5, INPUT);              // Prutokomer impulsy

 

  attachInterrupt(5, AddPulse, FALLING);

  //pinMode(5, INPUT);

  senzoryDS.begin();

 

}

 

void loop() {

  val = analogRead(analogPin);  // read the input pin

  Serial.print("Tlak     :");

  Serial.println(val);          // debug value

  M5.Lcd.setTextColor(TFT_YELLOW,TFT_BLACK);

  M5.Lcd.setTextSize(1);

  M5.Lcd.setTextFont(4);

  M5.Lcd.setTextDatum(BL_DATUM);

  M5.Lcd.setCursor(0, 0);

  M5.Lcd.print("Preasure:");

  M5.Lcd.print(val);

  M5.Lcd.print("          ");

 

  

  val = digitalRead(36);

  Serial.print("Hladina H  :");

  Serial.println(val);          // debug value

  M5.Lcd.setCursor(0, 30);

  M5.Lcd.print("Hladina H:");

  M5.Lcd.print(val);

  LevelHigh = val;

 

 

  val = digitalRead(34);

  Serial.print("Hladina L  :");

  Serial.println(val);          // debug value

  M5.Lcd.setCursor(0, 60);

  M5.Lcd.print("Hladina L:");

  M5.Lcd.print(val);

 

  Serial.print("Pulzů  :");

  Serial.println(PulseCount);          // debug value

 

  M5.Lcd.setCursor(0, 90);

  M5.Lcd.print("Pulzů  :");

  M5.Lcd.print(PulseCount);

 

 

  senzoryDS.requestTemperatures();

  // výpis teploty na sériovou linku, při připojení více čidel

  // na jeden pin můžeme postupně načíst všechny teploty

  // pomocí změny čísla v závorce (0) - pořadí dle unikátní adresy čidel

  Serial.print("Teplota cidla DS18B20: ");

  Serial.print(senzoryDS.getTempCByIndex(0));

  Serial.println(" stupnu Celsia");

  M5.Lcd.setCursor(0, 120);

  M5.Lcd.print("Teplota :");

  M5.Lcd.print(senzoryDS.getTempCByIndex(0));

 

 

  if (LevelHigh == 0) {

    digitalWrite(12, HIGH);

    M5.Lcd.setCursor(0, 150);

    M5.Lcd.print("Relay : ON    ");

  } else {

    digitalWrite(12, LOW);

    M5.Lcd.setCursor(0, 150);

    M5.Lcd.print("Relay : OFF  ");

  }

 

  delay(2000);

 

}

 

 

void AddPulse() {

  // inkrementace čítače pulzů

  PulseCount++;

} 
