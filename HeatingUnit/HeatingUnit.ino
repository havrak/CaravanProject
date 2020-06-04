#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <M5Stack.h>
#include "Free_Fonts.h"
#include <WiFi.h>
#include <EEPROM.h>
#include <esp_now.h>
#include "esp_system.h"


#define EEPROM_SIZE 7

const int button = 0;         //gpio to use to trigger delay
const int wdtTimeout = 15000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

esp_now_peer_info_t central;
esp_now_peer_info_t potencialCentral;
esp_now_peer_info_t emptyEspInfo;

bool sendedIMyTypeToCentral = false;
bool didCentralSendConfirmation = false;
bool checkingAgaintsEEPROMmaster = false;
bool isEEPROMinitialized = false;
int counter = 0;
byte noOfAttempts = 0;
long startTime = -1; // we give 10 seconds to central to respod, then we will try diffrent cenral with same name, values wont be so big that we would have to worry about unsigned long max size is bigger




int lastTimeDataRecived = 0;

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

struct StructConf {
  bool heating; // turn on / off heating
  bool winter; // winter
  bool cycle1; // user desires cycle 1 to be turned on / off
  bool cycle2; // user desires cycle 2 to be turned on / off
  bool cycle3; // user desires cycle 3 to be turned on / off
  bool cycle4; // user desires cycle 4 to be turned on / off
  int airTemp; //  MaxTemperatureInside
  int airTempTol; // tolerance při udržování teploty
  int floorTempMax; // MaxSupportTemperatureInside
  int limitFloorTemp; // MaxTemperature
};
StructConf conf;

struct SendDataStruct {
  double batteryState;
  double currentDraw;
  bool charging;
  
  int iHeatingCircuits;
  float CurrentTempBUS1[4];
  int CurrentStatusBUS1[4];
  int iExternSensors = 3;
  float CurrentTempBUS2[3];
  boolean bLowTemperature;
  boolean bCircuitLowTemperature[3];
  boolean bCircuitLowSupportTemperature[3];
  double dConsumption;
  bool bMasterRelayOn;
};



boolean checkIfTwoAddressesAreSame(const uint8_t *addr1, const uint8_t *addr2) {
  if (sizeof(addr1) != sizeof(addr2)) {
    return false;
  }
  for (int i = 0; i < (sizeof(addr1) / sizeof(addr1[0])); i++) {
    if (addr1[i] != addr2[i]) return false;
  }
  return true;
}

// prints given mac address
void printAddress(const uint8_t addr[]) {
  for (int i = 0; i < 6; ++i ) {
    Serial.print((uint8_t) addr[i], HEX);
    if (i != 5) Serial.print(":");
  }
  Serial.println();
}

// millis() counter resets every 50 days, gives time diffrence between millis() and sTime in argument
unsigned long getTimeDiffrence(const unsigned long sTime) {
  if (millis() < sTime) {
    return (ULONG_MAX - sTime) + millis();
  }
  return millis() - sTime;
}

// Init ESP Now with fallback
void initESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("HU | initESPNow | ESPNow Init Success");
  }
  else {
    Serial.println("HU | initESPNow | ESPNow Init Failed");
    ESP.restart();
  }
}

void storeDataInEEPROM() {
  if (EEPROM.read(0) == 0) { // limited number of writes
    EEPROM.write(0, 1);
  }
  uint8_t test[6];
  for (int i = 1 ; i < 7; i++ ) {
    test[i - 1] = EEPROM.read(i);
  }
  if (!checkIfTwoAddressesAreSame(test, central.peer_addr)) {
    Serial.println("HU | storeDataInEEPROM | storing new central mac into EEPROM");
    for (int i = 1 ; i < 7; i++ ) {
      EEPROM.write(i, central.peer_addr[i - 1]);
    }
  }
  // check if data is same
  EEPROM.commit();
}

void sendMyTypeToCentral() {
  Serial.print("HU | sendMyTypeToCentral | sending my type to central, its mac is: "); printAddress(potencialCentral.peer_addr); Serial.println("");
  uint8_t data = 104;
  esp_err_t result = esp_now_send(potencialCentral.peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
  Serial.print("HU | sendMyTypeToCentral | Send Status: ");
  if (result == ESP_OK) {
    Serial.println("success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    initESPNow();
    Serial.println("ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else {
    Serial.println("Not sure what happened");
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS && !sendedIMyTypeToCentral && counter < 10) { // try until data is send successfully
    Serial.println("HU | onDataSent | Sending info failed");
    delay(100);
    sendMyTypeToCentral();
    counter++;
  } else if (status == ESP_NOW_SEND_SUCCESS && !sendedIMyTypeToCentral) {
    sendedIMyTypeToCentral = true;
    counter = 0;
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("HU | onDataSent | Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("HU | onDataSent | Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

}

void sendData() {
  Serial.println();
  Serial.println("HU | sendData | Sending data");
  SendDataStruct data;

  uint8_t dataToBeSend[sizeof(data)];
  memcpy(dataToBeSend, &data, sizeof(data));
  Serial.print("HU | sendData | Size of dataToBeSend is: "); Serial.println(sizeof(dataToBeSend));

  esp_err_t result = esp_now_send(central.peer_addr, dataToBeSend, sizeof(dataToBeSend));

  Serial.print("HU | sendData | Send Status: ");
  if (result == ESP_OK) {
    Serial.println("Success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    initESPNow();
    Serial.println("ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
    esp_now_add_peer(&central);
  } else {
    Serial.println("Not sure what happened");
  }
  Serial.println();
}

// callback when data is recv from central
// check if mac_addr matches central, others inpouts ignore
// also sets up central, in case central asks again than address will be set up again for new central
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(*mac_addr);
  Serial.print("HU | onDataRecv | Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("HU | onDataRecv | Last Packet Recv Data: "); Serial.println(*data);


  if (*data == (uint8_t) 92) {
    if (checkIfTwoAddressesAreSame(potencialCentral.peer_addr, mac_addr) || (!isEEPROMinitialized && checkIfTwoAddressesAreSame(potencialCentral.peer_addr, emptyEspInfo.peer_addr))) { // after OR -- we recived info EEPROM was down yet we didn't foud any centaral so potencialCentral wouldn't be empty
      Serial.println("HU | onDataRecv | Set up central");
      counter = 0;
      memcpy(central.peer_addr, mac_addr, sizeof(central.peer_addr)); // size if diffrent,  sa d sa dsa sad
      didCentralSendConfirmation = true;
      esp_err_t addStatus = esp_now_add_peer(&central);
      lastTimeDataRecived = millis();
      Serial.print("HU | onDataRecv | Centrals mac address is: "); printAddress(central.peer_addr); Serial.println("");
      storeDataInEEPROM(); // save new central into EEPROM
    } else  {
      Serial.println("HU | onDataRecv | got 92 from unit I wasn't expecting");
    }
  }
  if (checkIfTwoAddressesAreSame(mac_addr, central.peer_addr)) {
    Serial.println("HU | onDataRecv | got some data");
    lastTimeDataRecived = millis();
    if (*data != (uint8_t) 88) { // check if message is not just a ping
      if (sizeof(data) != sizeof(temp)) {
        memcpy(&temp, data, sizeof(data));
        MaxTemperature = conf.limitFloorTemp;
        MaxTemperatureInside[1] = conf.airTemp;
        MaxTemperatureInside[2] = conf.airTemp;
        MaxSupportTemperatureInside[1] = conf.floorTempMax;
        MaxSupportTemperatureInside[2] = conf.floorTempMax;
        Serial.println("HU | onDataRecv | updated configuration");
      }
    }
  }
}

void ScanForCentral() {
  int8_t scanResults = WiFi.scanNetworks();
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("SU | ScanForCentral | No WiFi devices in AP Mode found");
  } else {
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
      delay(60);

      // Check if the current device starts with `Slave`
      if (SSID.indexOf("CARAVAN_CENTRAL_UNIT") == 0) {
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          esp_now_peer_info_t temp;
          for (int ii = 0; ii < 6; ++ii ) {
            temp.peer_addr[ii] = (uint8_t) mac[ii];
          }
          if(startTime == -1 || !checkIfTwoAddressesAreSame(potencialCentral.peer_addr, temp.peer_addr)){
            memcpy(potencialCentral.peer_addr, temp.peer_addr, sizeof(temp.peer_addr));
            potencialCentral.channel = 1;
            potencialCentral.encrypt = 0;
            // attempts to pair to found slave
            startTime = millis();
            checkingAgaintsEEPROMmaster = false;
            sendMyTypeToCentral();
            noOfAttempts = 0;
            if(!attempToPair()) startTime == -1;
          }
        }
      }
    }
  }
  sendMyTypeToCentral();
  WiFi.scanDelete();
}

// tryes to pair with potencialCentral
bool attempToPair() {
  Serial.print("SU | attempToPair | Processing: ");
  
  for (int ii = 0; ii < 6; ++ii ) {
    Serial.print((uint8_t) potencialCentral.peer_addr[ii], HEX);
    if (ii != 5) Serial.print(":");
  }
  Serial.print(", Status:");

  // check if the peer exists
  if (!esp_now_is_peer_exist(potencialCentral.peer_addr)) {
    Serial.println("Pairing");
    esp_err_t addStatus = esp_now_add_peer(&potencialCentral);
    if (addStatus == ESP_OK) {
      Serial.println("Paired");
      sendMyTypeToCentral();
      return true;
    } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
      Serial.println("ESPNOW Not Init");
      initESPNow();
      if(noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)){
        attempToPair();
      }
    } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Add Peer - Invalid Argument");
      if(noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair(); 
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
      Serial.println("Peer list full");
      if(noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair(); 
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("Out of memory"); 
      if(noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair(); 
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
      Serial.println("Peer Exists");
      if(noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair(); 
      noOfAttempts++;
    } else {
      Serial.println("Not sure what happened");
      if(noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair(); 
      noOfAttempts++;
    }
    delay(100);
  }
  if(noOfAttempts >= (checkingAgaintsEEPROMmaster ? 16 : 8)) return false;
  delay(50);
  
}

// if central didin't send anything for 30 second we will delete her
void deleteUnactiveCentral(){
  if(getTimeDiffrence(lastTimeDataRecived) > 30000){
    Serial.println("SU | deleteUnactiveCentral | deleting");
    sendedIMyTypeToCentral = false;
    didCentralSendConfirmation = false;  
    potencialCentral = emptyEspInfo;
    central = emptyEspInfo;
    
    esp_err_t delStatus = esp_now_del_peer(central.peer_addr);
    Serial.print("SU | deleteUnactiveCentral | Slave Delete Status: ");
    if (delStatus == ESP_OK) {
      // Delete success
      Serial.println("Success");
    } else if (delStatus == ESP_ERR_ESPNOW_NOT_INIT) {
      Serial.println("ESPNOW Not Init");
    } else if (delStatus == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Invalid Argument");
    } else if (delStatus == ESP_ERR_ESPNOW_NOT_FOUND) {
      Serial.println("Peer not found.");
    } else {
      Serial.println("Not sure what happened");
    } 
  }
}

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
      if (digitalRead(iConnected230VPin) == 1) { // neni přípojka do sítě
        M5.Lcd.setTextColor(TFT_RED);
        M5.Lcd.println("230V disconnected");
        M5.Lcd.println("Master Relay Off");
        bMasterRelayOn = false; // masterRelayOn na false
        digitalWrite(iMasterRelayPin, HIGH); // vypnot masterRaley

        for (byte i = 0; i < iCircuitRelaysMax; i++) // projít všechny topné okruhy a vypnout je
        { 
          pinMode(tempSensorsBUS1_PIN[i], OUTPUT);
          digitalWrite(tempSensorsBUS1_PIN[i], LOW);  //OFF
          CurrentStatusBUS1[i] = 0; // okruh je vypnutý
        }

      } else {
        M5.Lcd.setTextColor(TFT_GREEN);
        M5.Lcd.println("230V connected");
        M5.Lcd.println("Master Relay On");
        bMasterRelayOn = true;
        digitalWrite(iMasterRelayPin, LOW);// zapteme rele, okruhy na okruzís se nic nemění
      }
      iLcdRow = iLcdRow + 2;

      //delay (1000);
      for (byte i = 0; i < iHeatingCircuits; i++) // procházíme všechny 4 okruhy
      {

        TempSensors_BUS1.requestTemperaturesByAddress(tempSensorsBUS1_Address[i]); // zbytečný řádek??????
        CurrentTempBUS1[i] = TempSensors_BUS1.getTempC(tempSensorsBUS1_Address[i]); // aktualizujeme teplotu v daném okruhu

        if ((bMasterRelayOn && bCircuitLowTemperature[i]) || (bMasterRelayOn && bCircuitLowSupportTemperature[i])) { // je zapnuté masterRele a je zaplý okruh || je zaptnuté masterRele a okruh topí na podporu?????
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

  M5.begin(true, false, true);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.setFreeFont(FSB12);
  Serial.begin(115200);           //  setup serial
  Wire.begin();
  WiFi.mode(WIFI_STA);
  initESPNow();
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);
  potencialCentral = emptyEspInfo;
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
  delay(500);
  if (!EEPROM.begin(EEPROM_SIZE)){
    // we can't read from EEPROM
    Serial.println("CU | SETUP | failed to initialize EEPROM");
  }else{
    isEEPROMinitialized = true;
    if(EEPROM.read(0) == 0){
      Serial.println("CU | SETUP | EEPROM is empty");
    }else{
      Serial.println("CU | SETUP | Loading data from EEPROM");
      for (int i = 0; i < 6; ++i ) {
        potencialCentral.peer_addr[i] = (uint8_t) EEPROM.read(i+1);// first byte is to check
      }
      potencialCentral.channel = 1; // pick a channel
      potencialCentral.encrypt = 0;
      Serial.print("SU | SETUP | Central address that i got from EEPROM is: "); printAddress(potencialCentral.peer_addr); Serial.println("");
      if(attempToPair()){ 
        checkingAgaintsEEPROMmaster = true;
        startTime = millis();
      }
    }
  }

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

byte count = 0;
float temperature;

void loop() {

  M5.update();
  if(!didCentralSendConfirmation){
    if((startTime == -1&& !checkingAgaintsEEPROMmaster) || (getTimeDiffrence(startTime) > (checkingAgaintsEEPROMmaster ? 15000 : 10000))){ // we waited too long to get a
      Serial.println("SU | LOOP | Scanning");
      ScanForCentral();
    }else{
      Serial.println("SU | LOOP | Sending my type to central");
      sendMyTypeToCentral();
      delay(75);
    }
  }else{
    deleteUnactiveCentral();  
  }
  
  if(count % 3 == 0 && didCentralSendConfirmation){
    count = 0;
    sendData();
  }
  count++; 
  delay(1000);
}
