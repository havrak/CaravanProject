#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <M5Stack.h>
#include "Free_Fonts.h"
#include <WiFi.h>
#include <EEPROM.h>
#include <esp_now.h>
#include "esp_system.h"

const int button = 0;         //gpio to use to trigger delay
const int wdtTimeout = 15000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

const int pinTemp_BUS1 = 13;
const int pinTemp_BUS2 = 5;


OneWire oneWireDS_BUS1(pinTemp_BUS1);
OneWire oneWireDS_BUS2(pinTemp_BUS2);

DallasTemperature TempSensors_BUS1(&oneWireDS_BUS1);
DallasTemperature TempSensors_BUS2(&oneWireDS_BUS2);


float MaxTemperature = 34;
float MaxSupportTemperature = 27;



DeviceAddress tempSensorsBUS1_Address[3] = {
  {0x28, 0x76, 0xD1, 0xD6, 0x37, 0x19, 0x01, 0x0B}, //Back part   - Port1
  {0x28, 0x0B, 0x99, 0xF1, 0x37, 0x19, 0x01, 0xF8}, //Middle part - Port 2
  {0x28, 0xE5, 0X9F, 0xE8, 0x37, 0x19, 0x01, 0xF5} //Front part  - Port 3
};


int tempSensorsBUS1_Mapping[3] = {
  1,
  2,
  2
};

float CurrentTempBUS1[4];
int CurrentStatusBUS1[4];

String tempSensorsBUS1_Name[4] = {
  "Back_290W",
  "Middle_400W",
  "Front_350W",
  "Reserve    "
};

int tempSensorsBUS1_PIN[4] = {
  17,     //Relay 3
  21,     //Relay 2
  19,     //Relay 1
  22      //Relay 4
};

int     iMasterRelayPin = 16;
int     iHeatingCircuits = 3;
int     iExternSensors = 3;
int     iCircuitRelaysMax = 4;
int     iConnected230VPin = 34;
int     iLcdRow = 1;

DeviceAddress tempSensorsBUS2_Address[3] = {
  {0x28, 0xE7, 0xFA, 0xE2, 0x37, 0x19, 0x01, 0x43},
  {0x28, 0xFF, 0x6E, 0xC7, 0x57, 0x16, 0x04, 0x62},
  {0x28, 0x82, 0x1B, 0x7B, 0x27, 0x19, 0x01, 0x44}
};

boolean bCircuitLowTemperature [3] = {
  false,
  false,
  false
};

boolean bCircuitLowSupportTemperature [3] = {
  false,
  false,
  false
};


String tempSensorsBUS2_Name[3] = {
  "OutSide",
  "Top Back",
  "Middle"
};

float MaxTemperatureInside [3] = {
  -99,
  22,
  22
};

float MaxSupportTemperatureInside [3] = {
  -99,
  23,
  23
};

double dConsumption;
float CurrentTempBUS2[3];
int iRefreshRate = 30000;
long lCircuitStartTime [4];
long lCircuitRunTime[4];
float fCircuitWatts[4] = {
  290,
  400,
  350,
  0
};


// outside leads to ground and +5V
int val = 0;  // variable to store the value read
boolean bMasterRelayOn = false;
boolean bLowTemperature = false;
float MaxTemperatureOutside = 22.0;
float MaxSupportTemperatureOutside = 24.0;
byte TemperatureExternSensorInside = 2;
byte TemperatureExternSensorOutside = 0;
long LastRefresh = 0;

int LevelHigh = 0;
long PulseCount = 0;
float TemperatureFloor;
char cName;


void HeatingMain(void * pvParameters) {

  for (;;) {
    if (LastRefresh == 0 || millis() - LastRefresh > iRefreshRate) {
      M5.Lcd.clear();
      iLcdRow = 1;
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.setCursor(0, iLcdRow * 24);
      M5.Lcd.print(WiFi.localIP());
      iLcdRow = iLcdRow + 1;
      M5.Lcd.setCursor(0, iLcdRow * 24);
      if (digitalRead(iConnected230VPin) == 1) {
        M5.Lcd.setTextColor(TFT_RED);
        M5.Lcd.println("230V disconnected");
        M5.Lcd.println("Master Relay Off");
        bMasterRelayOn = false;
        digitalWrite(iMasterRelayPin, HIGH);

        for (byte i = 0; i < iCircuitRelaysMax; i++)
        {
          pinMode(tempSensorsBUS1_PIN[i], OUTPUT);
          digitalWrite(tempSensorsBUS1_PIN[i], LOW);  //OFF
          CurrentStatusBUS1[i] = 0;
        }

      } else {
        M5.Lcd.setTextColor(TFT_GREEN);
        M5.Lcd.println("230V connected");
        M5.Lcd.println("Master Relay On");
        bMasterRelayOn = true;
        digitalWrite(iMasterRelayPin, LOW);
      }
      iLcdRow = iLcdRow + 2;

      //delay (1000);


      for (byte i = 0; i < iHeatingCircuits; i++)
      {

        TempSensors_BUS1.requestTemperaturesByAddress(tempSensorsBUS1_Address[i]);
        CurrentTempBUS1[i] = TempSensors_BUS1.getTempC(tempSensorsBUS1_Address[i]);

        if ((bMasterRelayOn && bCircuitLowTemperature[i]) || (bMasterRelayOn && bCircuitLowSupportTemperature[i])) {
          if ( (bCircuitLowTemperature[i] && CurrentTempBUS1[i]  < MaxTemperature) || (bCircuitLowSupportTemperature[i] && CurrentTempBUS1[i]  < MaxSupportTemperature)) {
            digitalWrite(tempSensorsBUS1_PIN[i], HIGH);
            if (CurrentStatusBUS1[i] == 0) {
              lCircuitStartTime[i] = millis();
            } else {
              lCircuitRunTime[i] = lCircuitRunTime[i] + (millis() - lCircuitStartTime[i]);
              lCircuitStartTime[i] = millis();
            }
            CurrentStatusBUS1[i] = 1;
            M5.Lcd.setTextColor(TFT_GREEN);
          } else {
            digitalWrite(tempSensorsBUS1_PIN[i], LOW);
            if (CurrentStatusBUS1[i] == 1) {
              lCircuitRunTime[i] = lCircuitRunTime[i] + (millis() - lCircuitStartTime[i]);
            }
            CurrentStatusBUS1[i] = 0;
            M5.Lcd.setTextColor(TFT_RED);
          }

        } else {
          digitalWrite(tempSensorsBUS1_PIN[i], LOW);
          if (CurrentStatusBUS1[i] == 1) {
            lCircuitRunTime[i] = lCircuitRunTime[i] + (millis() - lCircuitStartTime[i]);
          }
          CurrentStatusBUS1[i] = 0;
          M5.Lcd.setTextColor(TFT_RED);
        }
        M5.Lcd.setCursor(0, iLcdRow * 24);
        M5.Lcd.print(tempSensorsBUS1_Name[i]);
        M5.Lcd.setCursor(200, iLcdRow * 24);
        M5.Lcd.print(CurrentTempBUS1[i]);
        iLcdRow = iLcdRow + 1;

      }


      for (byte i = 0; i < iExternSensors; i++)
      {

        TempSensors_BUS2.requestTemperaturesByAddress(tempSensorsBUS2_Address[i]);
        CurrentTempBUS2[i] = TempSensors_BUS2.getTempC(tempSensorsBUS2_Address[i]);



        M5.Lcd.setTextColor(TFT_YELLOW);
        M5.Lcd.setCursor(0, iLcdRow * 24);
        M5.Lcd.print(tempSensorsBUS2_Name[i]);
        M5.Lcd.setCursor(200, iLcdRow * 24);
        M5.Lcd.print(CurrentTempBUS2[i]);
        iLcdRow = iLcdRow + 1;

      }

      bLowTemperature = false;

      for (byte i = 0; i < iHeatingCircuits; i++) {
        if (CurrentTempBUS2[tempSensorsBUS1_Mapping[i]] < MaxTemperatureInside[tempSensorsBUS1_Mapping[i]]  && CurrentTempBUS2[TemperatureExternSensorOutside] < MaxTemperatureOutside) {
          bLowTemperature = true;
          bCircuitLowTemperature[i] = true;
        } else {
          bCircuitLowTemperature[i] = false;
        }

        if (CurrentTempBUS2[tempSensorsBUS1_Mapping[i]] >= MaxTemperatureInside[tempSensorsBUS1_Mapping[i]] && CurrentTempBUS2[tempSensorsBUS1_Mapping[i]] < MaxSupportTemperatureInside[tempSensorsBUS1_Mapping[i]] && CurrentTempBUS2[TemperatureExternSensorOutside] < MaxSupportTemperatureOutside) {
          bLowTemperature = true;
          bCircuitLowSupportTemperature[i] = true;
        } else {
          bCircuitLowSupportTemperature[i] = false;
        }
      }

      LastRefresh = millis();
    }

    delay(1000);

  }
}

void setup() {

  M5.begin();
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.setFreeFont(FSB12);
  Serial.begin(115200);           //  setup serial

  iLcdRow = 1;
  M5.Lcd.setTextColor(TFT_WHITE);

  pinMode(iMasterRelayPin, OUTPUT); // OK Master
  digitalWrite(iMasterRelayPin, HIGH);

  for (byte i = 0; i < iCircuitRelaysMax; i++)
  {
    pinMode(tempSensorsBUS1_PIN[i], OUTPUT);
    digitalWrite(tempSensorsBUS1_PIN[i], LOW);  //OFF
  }

  //  pinMode(19, OUTPUT); // OK kanal 1
  //  pinMode(21, OUTPUT); // OK kanal 2
  //  pinMode(17, OUTPUT); // OK kanal 3
  //  pinMode(22, OUTPUT); // OK kanal 4

  pinMode(iConnected230VPin, INPUT);
  TempSensors_BUS1.begin();
  TempSensors_BUS2.begin();

  pinMode(button, INPUT_PULLUP);                    //init control pin
  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt


  // Task 2
  xTaskCreatePinnedToCore(
    HeatingMain,     /* Function to implement the task */
    "Heating",   /* Name of the task */
    4096,      /* Stack size in words */
    NULL,      /* Task input parameter */
    2,         /* Priority of the task */
    NULL,      /* Task handle. */
    1);        /* Core where the task should run */
}


void loop() {

  M5.update();
}
