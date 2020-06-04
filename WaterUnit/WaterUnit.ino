#include <OneWire.h>
#include <DallasTemperature.h>
#include <M5Stack.h>
#include "Free_Fonts.h"
#include <esp_now.h>
#include <WiFi.h>
#include <EEPROM.h>

#define EEPROM_SIZE 256
#define CHANNEL 1

#define RELEVALV 12
#define RELEHEAT 15
#define LEVELTOP 36
#define LEVELBOT 34
#define IMPULSEM 5
#define PREAWURE 35
#define DSPINTEMP 13

OneWire oneWireDS(DSPINTEMP);

DallasTemperature tempSensor(&oneWireDS);

// preasure - connected to analog pin 3
// outside leads to ground and +5V

const float maxVolumeOfTank = 40.7;
const float remainderWhenLowSensorHitted = 3;
const int pulsesPerLiter = 760;

esp_now_peer_info_t central;
esp_now_peer_info_t potencialCentral;
esp_now_peer_info_t emptyEspInfo;

bool sendedIMyTypeToCentral = false;
bool didCentralSendConfirmation = false;
bool checkingAgaintsEEPROMmaster = false;
bool isEEPROMinitialized = false;
long startTime = -1;
int counter = 0;
byte noOfAttempts = 0;

bool relayOpen = false;
bool heatingOn = false;
bool connectionToWaterSource;

float litersRemaining;
float waterTemperature;
float airTemperature;

//bool connectionToWater;
bool topTankSensor;
bool bottomTankSensor;
int pulseCounter;
byte validityOfData;

bool winter;

int indexOfOuterTemp;
int indexOfInnerTemp;

int lastTimeDataRecived = 0;

// checks if two addresses are same
boolean checkIfTwoAddressesAreSame(const uint8_t *addr1, const uint8_t *addr2) {
  if (sizeof(addr1) != sizeof(addr2)) {
    Serial.println("diffrent size");
    return false;
  }
  for (int i = 0; i < (sizeof(addr1) / sizeof(addr1[0])); i++) {
    if (addr1[i] != addr2[i]) return false;
  }
  return true;
}

// prints given mac address, for debug
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

// send and recive structure, for ease of access it it used only for reciving and sending data, ESP32 has plenty of room to store those data 2 times
struct SendRecvDataStruct {
  bool connectionToWaterSource;
  // 0 - data is valid, system hit top and counters were reseted, 1 - data could be valid, but was loaded from EEPROM, 2 - EEPROM was empty thus we can't guess state of tank, 3 - tank wasnt filled to its full capacity
  byte validityOfData;
  float litersRemaining;
  float temperature;
  float airTemperature;
  bool heating;
};

struct StructConf {
  bool winter;
  int minTemp;
};

StructConf conf;



// Init ESP Now with fallback
void initESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

bool attempToPair() {
  Serial.print("WU | attempToPair | Processing: ");

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
      if (noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) {
        attempToPair();
      }
    } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Add Peer - Invalid Argument");
      if (noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair();
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
      Serial.println("Peer list full");
      if (noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair();
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("Out of memory");
      if (noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair();
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
      Serial.println("Peer Exists");
      if (noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair();
      noOfAttempts++;
    } else {
      Serial.println("Not sure what happened");
      if (noOfAttempts < (checkingAgaintsEEPROMmaster ? 16 : 8)) attempToPair();
      noOfAttempts++;
    }
    delay(100);
  }
  if (noOfAttempts >= (checkingAgaintsEEPROMmaster ? 16 : 8)) return false;
  delay(50);
}

// counting speed is diffrent than what it should be
// what to set when water is refiling
void addPulse() {
  // evry liter / half a leter
  if (relayOpen) {
    // here when water is refilling
  } else {
    pulseCounter++;
    litersRemaining -= (pulseCounter / pulsesPerLiter);
  }
  // evry +- liter we want to
  if (pulseCounter % pulsesPerLiter == 0) {
    storeDataInEEPROM();
  }
}

// EEPROM data with theirs respective adresses in parentheses: byte(0) -- will be one if something was written, literRemaiding(1,4), pulseCounter(5,8), relayOpen(9), heatingOn(10)
void storeDataInEEPROM() {
  if (EEPROM.read(0) == 0) {
    EEPROM.write(0, 1);
  }
  uint8_t test[6];
  for (int i = 1 ; i < 7; i++ ) {
    test[i - 1] = EEPROM.read(i);
  }
  if (!checkIfTwoAddressesAreSame(test, central.peer_addr)) {
    Serial.println("WU | storeDataInEEPROM | storing new central mac into EEPROM");
    for (int i = 1 ; i < 7; i++ ) {
      EEPROM.write(i, central.peer_addr[i - 1]);
    }
  }

  // split data into bytes
  char temp[sizeof(litersRemaining)];
  memcpy(temp, &litersRemaining, sizeof(litersRemaining));
  for (int i = 0; i < (sizeof(temp) / sizeof(temp[0])); i++) {
    EEPROM.write(i + 8, temp[i]);
  }
  temp[sizeof(pulseCounter)];
  memcpy(temp, &pulseCounter, sizeof(pulseCounter));
  for (int i = 0; i < (sizeof(temp) / sizeof(temp[0])); i++) {
    EEPROM.write(i + 12, temp[i]);
  }
  EEPROM.write(16, relayOpen);
  EEPROM.write(17, heatingOn);
  EEPROM.commit();
}
DeviceAddress tempDeviceAddressA; // type: byte[8]
DeviceAddress tempDeviceAddressB;
DeviceAddress tempDeviceAddressC;

// loads date from EEPROM
void loadDataFromEEPROM() {
  if (EEPROM.read(0) == 0) { // water data
    validityOfData = 2;
    Serial.println("WU | SETUP | EEPROM is empty");
  } else {
    Serial.println("CU | SETUP | Loading data from EEPROM");
    for (int i = 0; i < 6; ++i ) {
      potencialCentral.peer_addr[i] = (uint8_t) EEPROM.read(i + 1); // first byte is to check
    }
    potencialCentral.channel = 1; // pick a channel
    potencialCentral.encrypt = 0;
    Serial.print("WU | SETUP | Central address that i got from EEPROM is: "); printAddress(potencialCentral.peer_addr); Serial.println("");
    if (attempToPair()) {
      checkingAgaintsEEPROMmaster = true;
      startTime = millis();
    }
    validityOfData = 1;
    char temp[sizeof(litersRemaining)];
    for (int i = 0; i < (sizeof(temp) / sizeof(temp[0])); i++) {
      temp[i] = EEPROM.read(i + 8);
    }
    memcpy(&litersRemaining , temp, sizeof(temp));
    temp[sizeof(pulseCounter)];
    for (int i = 0; i < (sizeof(temp) / sizeof(temp[0])); i++) {
      temp[i] = EEPROM.read(i + 12);
    }
    memcpy(&pulseCounter, temp, sizeof(temp));
    relayOpen = EEPROM.read(16);
    if (relayOpen) digitalWrite(RELEVALV, HIGH);
    heatingOn = EEPROM.read(17);
    if (heatingOn) digitalWrite(RELEHEAT, HIGH);

  }
  if (EEPROM.read(18) == 0)identifyTemperatureSensors(); // remp sensors
  else {
    //int indexOfOuterTemp;
    //int indexOfInnerTemp;
    for (int i = 19; i < 27; i++) { // address for inner sensor
      tempDeviceAddressA[i - 19] = EEPROM.read(i);
    }
    for (int i = 27; i < 35; i++) { // address for outer sensor
      tempDeviceAddressB[i - 19] = EEPROM.read(i);
    }
    tempSensor.getAddress(tempDeviceAddressC, 0);
    if (checkIfTwoAddressesAreSame(tempDeviceAddressC, tempDeviceAddressA)) { // on 0 is water
      tempSensor.getAddress(tempDeviceAddressC, 1);
      if (checkIfTwoAddressesAreSame(tempDeviceAddressC, tempDeviceAddressB)) {
        indexOfOuterTemp = 1;
        indexOfInnerTemp = 0;
      } else {
        identifyTemperatureSensors();
      }

    } else if (checkIfTwoAddressesAreSame(tempDeviceAddressC, tempDeviceAddressB)) {
      tempSensor.getAddress(tempDeviceAddressC, 1);
      if (checkIfTwoAddressesAreSame(tempDeviceAddressC, tempDeviceAddressA)) {
        indexOfOuterTemp = 0;
        indexOfInnerTemp = 1;
      } else {
        identifyTemperatureSensors();
      }
    } else {
      identifyTemperatureSensors();
    }
  }
}

float tempTemp1;
float tempTemp2;

// stores addresses of outer and inners sensor in EEPROM
void storeAddressesInEEPROM(DeviceAddress inner, DeviceAddress outer) {
  EEPROM.write(18, 1);
  for (int i = 19; i < 27; i++) { // address for inner sensor
    EEPROM.write(i, tempDeviceAddressA[i - 19]);
  }
  for (int i = 27; i < 35; i++) { // address for outer sensor
    EEPROM.write(i, tempDeviceAddressA[i - 27]);
  }
}

// identifies heat sensor,
void identifyTemperatureSensors() {
  if (tempSensor.getDeviceCount() != 2) {
    tempTemp1 = tempSensor.getTempCByIndex(0);
    tempTemp2 = tempSensor.getTempCByIndex(1);
    Serial.println("WU | identifyTemperatureSensors | warm sensor in tank by 3 degrees");
    M5.Lcd.setCursor(0, 150);
    M5.Lcd.print("Warm sensor in tank by 3 degrees");
    while (tempSensor.getTempCByIndex(0) - tempTemp1 < 3 || tempSensor.getTempCByIndex(1) - tempTemp2 < 3) delay(1); // wait

    tempSensor.getAddress(tempDeviceAddressA, 0);
    tempSensor.getAddress(tempDeviceAddressB, 1);
    if (tempSensor.getTempCByIndex(0) - tempTemp1 >= 3) { // sensor on index 0 is in water
      indexOfOuterTemp = 1;
      indexOfInnerTemp = 0;
      storeAddressesInEEPROM(tempDeviceAddressA, tempDeviceAddressB);
    } else {
      indexOfOuterTemp = 0;
      indexOfInnerTemp = 1;
      storeAddressesInEEPROM(tempDeviceAddressB, tempDeviceAddressA);
    }
  } else {
    Serial.print("WU | identifyTemperatureSensors | two sensors weren't detected");
  }
}

// sends type to central
void sendMyTypeToCentral() {
  Serial.print("SU | sendMyTypeToCentral | sending my type to central, its mac is: "); printAddress(potencialCentral.peer_addr); Serial.println("");
  uint8_t data = 102;
  esp_err_t result = esp_now_send(potencialCentral.peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
  Serial.print("SU | sendMyTypeToCentral | Send Status: ");
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
    Serial.println("WU | onDataSent | Sending info failed");
    delay(100);
    sendMyTypeToCentral();
    counter++;
  } else if (status == ESP_NOW_SEND_SUCCESS && !sendedIMyTypeToCentral) {
    sendedIMyTypeToCentral = true;
    counter = 0;
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("WU | onDataSent | Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("WU | onDataSent | Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

}

void sendData() {
  Serial.println();
  Serial.println("Sending data");
  SendRecvDataStruct data;

  data.connectionToWaterSource = connectionToWaterSource;
  data.litersRemaining = litersRemaining;
  data.temperature = waterTemperature;
  data.validityOfData = validityOfData;
  data.heating = heatingOn;
  data.airTemperature = airTemperature;
  
  uint8_t dataToBeSend[sizeof(data)];
  memcpy(dataToBeSend, &data, sizeof(data));

  esp_err_t result = esp_now_send(central.peer_addr, dataToBeSend, sizeof(dataToBeSend));
  Serial.print("Send Status: ");
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
  Serial.print("WU | onDataRecv | Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("WU | onDataRecv | Last Packet Recv Data: "); Serial.println(*data);


  if (*data == (uint8_t) 92) {
    if (checkIfTwoAddressesAreSame(potencialCentral.peer_addr, mac_addr) || (!isEEPROMinitialized && checkIfTwoAddressesAreSame(potencialCentral.peer_addr, emptyEspInfo.peer_addr))) { // after OR -- we recived info EEPROM was down yet we didn't foud any centaral so potencialCentral wouldn't be empty
      Serial.println("WU | onDataRecv | Set up central");
      counter = 0;
      memcpy(central.peer_addr, mac_addr, sizeof(central.peer_addr)); // size if diffrent,  sa d sa dsa sad
      didCentralSendConfirmation = true;
      esp_err_t addStatus = esp_now_add_peer(&central);
      lastTimeDataRecived = millis();
      Serial.print("WU | onDataRecv | Centrals mac address is: "); printAddress(central.peer_addr); Serial.println("");
      storeDataInEEPROM(); // save new central into EEPROM
    } else  {
      Serial.println("WU | onDataRecv | got 92 from unit I wasn't expecting");
    }
  }
  if (checkIfTwoAddressesAreSame(mac_addr, central.peer_addr)) {
    Serial.println("WU | onDataRecv | got some data");
    lastTimeDataRecived = millis();
    if (*data != (uint8_t) 88) { // check if message is not just a ping
      // NEW CONFIGURATION IS PROCESSED HERE
      
      if (sizeof(data) != sizeof(conf)) {
        memcpy(&conf, data, sizeof(data));
        
        Serial.println("WU | onDataRecv | updated configuration");
      }
    }
  }
}

void ScanForCentral() {
  int8_t scanResults = WiFi.scanNetworks();
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("WU | ScanForCentral | No WiFi devices in AP Mode found");
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
          if (startTime == -1 || !checkIfTwoAddressesAreSame(potencialCentral.peer_addr, temp.peer_addr)) {
            memcpy(potencialCentral.peer_addr, temp.peer_addr, sizeof(temp.peer_addr));
            potencialCentral.channel = 1;
            potencialCentral.encrypt = 0;
            // attempts to pair to found slave
            startTime = millis();
            checkingAgaintsEEPROMmaster = false;
            sendMyTypeToCentral();
            noOfAttempts = 0;
            if (!attempToPair()) startTime == -1;
          }
        }
      }
    }
  }
  sendMyTypeToCentral();
  WiFi.scanDelete();
}

// tryes to pair with potencialCentral


// deletes unactive cenral (if no data/ping was recieved for 30 seconds)
void deleteUnactiveCentral() {
  if (getTimeDiffrence(lastTimeDataRecived) > 30000) {
    Serial.println("WU | deleteUnactiveCentral | deleting");
    sendedIMyTypeToCentral = false;
    didCentralSendConfirmation = false;
    potencialCentral = emptyEspInfo;
    central = emptyEspInfo;

    esp_err_t delStatus = esp_now_del_peer(central.peer_addr);
    Serial.print("WU | deleteUnactiveCentral | Slave Delete Status: ");
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

void setup() {
  Serial.begin(115200);

  //Set device in AP mode to begin with
  WiFi.mode(WIFI_STA);
  // This is the mac address of the Slave in AP Mode
  Serial.print("WU | SETUP | STA MAC: "); Serial.println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  initESPNow();
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  //pinMode(4, OUTPUT);                     // relay (LOW on, HIGH off) - when on water can start flowing to water tank
  //pinMode(15, INPUT);                     // sensor of upper water level, HIGH on - sensor sends one if top sensor was hitted
  //pinMode(13, INPUT);                     // sensor of lower water level, HIGH on
  //pinMode(5, INPUT);                      // flowmeter sends impule
  //pinMode(14, INPUT);
  pinMode(RELEVALV, OUTPUT);                // relay (LOW on, HIGH off) - when on water can start flowing to water tank
  pinMode(LEVELTOP, INPUT);                 // sensor of upper water level, HIGH on - sensor sends one if top sensor was hitted
  pinMode(LEVELBOT, INPUT);                 // sensor of lower water level, HIGH on
  pinMode(IMPULSEM, INPUT);                 // flowmeter sends impule
  pinMode(RELEHEAT, OUTPUT);                // rele for heating water
  pinMode(25, OUTPUT);                      // speaker pin

  digitalWrite(RELEVALV, LOW);
  digitalWrite(RELEHEAT, LOW);
  digitalWrite(25, LOW);
  //  pinMode(12, OUTPUT);             // rele ventil(HIGH vypnuto, LOW zapnuto

  //pinMode(15, OUTPUT);             // rele vyhrev(HIGH vypnuto, LOW zapnuto
  //pinMode(36, INPUT);             // snímač hladiny Horni, LOW sepnuto
  //pinMode(34, INPUT);             // snímač hladiny spodní, LOW sepnuto
  //pinMode(5, INPUT);              // Prutokomer impulsy

  conf.minTemp = 2;
  
  if (!EEPROM.begin(EEPROM_SIZE)) {
    validityOfData = 2; // we can't read from EEPROM
    Serial.println("failed to initialize EEPROM");
  } else { // if EERPROM is empty validity is set to 2, if data is loaded validity is set to 1
    loadDataFromEEPROM();
    isEEPROMinitialized = true;
  }

  M5.begin();
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.setFreeFont(FSB12);

  attachInterrupt(5, addPulse, FALLING);  // added interrupt for flow meter impulses
  tempSensor.begin();

}

int val;
byte count = 0;

void loop() {
  if (M5.BtnA.wasReleased()) {
    identifyTemperatureSensors();
  }

  M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextFont(4);
  M5.Lcd.setTextDatum(BL_DATUM);


  tempSensor.requestTemperatures();
  // check for unactive
  connectionToWaterSource = (analogRead(PREAWURE) > 250) ? true : false;
  topTankSensor = (digitalRead(LEVELTOP) == HIGH) ? true : false;
  bottomTankSensor = (digitalRead(LEVELBOT) == LOW) ? true : false; // is high until water is low enough
  waterTemperature = tempSensor.getTempCByIndex(indexOfInnerTemp);
  airTemperature = tempSensor.getTempCByIndex(indexOfOuterTemp);
  // take care of temperature

  // výpis teploty na sériovou linku, při připojení více čidel

  // na jeden pin můžeme postupně načíst všechny teploty

  // pomocí změny čísla v závorce (0) - pořadí dle unikátní adresy čidel
  val = analogRead(PREAWURE);

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Preasure:");
  M5.Lcd.print(val);
  M5.Lcd.print("          ");

  M5.Lcd.setCursor(0, 30);
  M5.Lcd.print("Hladina H:");
  M5.Lcd.print(topTankSensor);

  M5.Lcd.setCursor(0, 60);
  M5.Lcd.print("Hladina L:");
  M5.Lcd.print(bottomTankSensor);


  M5.Lcd.setCursor(0, 90);
  M5.Lcd.print("Pulzů  :");
  M5.Lcd.print(pulseCounter);

  M5.Lcd.setCursor(0, 120);
  M5.Lcd.print("Teplota :");
  M5.Lcd.print(waterTemperature);

  M5.Lcd.setCursor(0, 150);
  M5.Lcd.print("1. tlačítko spustí");
  M5.Lcd.print("identifikaci senzorů");

  Serial.print("Connection to water : "); Serial.println(connectionToWaterSource);
  Serial.print("Preasure            : "); Serial.println(val);
  Serial.print("Top                 : "); Serial.println(topTankSensor);
  Serial.print("Bottom              : "); Serial.println(bottomTankSensor);
  Serial.print("Heating             : "); Serial.println(heatingOn);
  //Serial.print("Temperature         : "); Serial.println(watertemperature);

  // heatingOn = false;
  // bool connectionToWaterSource;

  // float litersRemaining;
  // float waterTemperature;
  // float airTemperature;
  if (waterTemperature < 3 && !heatingOn) {
    Serial.println("WU | LOOP | turning on heating");
    heatingOn = true;
    digitalWrite(RELEHEAT, HIGH);
  } else if (waterTemperature > 8 && !heatingOn) {
    Serial.println("WU | LOOP | turning off heating");
    heatingOn = false;
    digitalWrite(RELEHEAT, LOW);
  }

  if (connectionToWaterSource && airTemperature > 1 && !topTankSensor && !relayOpen) {
    // we can refill tank
    Serial.println("Refilling");
    M5.Lcd.setCursor(0, 150);
    M5.Lcd.print("Reffiling");
    //digitalWrite(RELEVALV, HIGH);
    relayOpen = true;
    // add value for refilling
  } else if (topTankSensor) { // relayOpen &&
    // close reffiling of tank
    Serial.println("Refilling finished");
    //digitalWrite(RELEVALV, LOW);
    M5.Lcd.setCursor(0, 150);
    M5.Lcd.print("Reffiling finished");
    relayOpen = false;
    litersRemaining = maxVolumeOfTank;
    validityOfData = 0;
    pulseCounter = 0;
  } else if (!connectionToWaterSource && relayOpen) {
    Serial.println("Refilling stopped");
    M5.Lcd.setCursor(0, 150);
    M5.Lcd.print("Reffiling stopped");
    //digitalWrite(RELEVALV, HIGH);
    // tank had been refilling but water source was disconnected
    // we have no method guessing how much is in tank (there is no input pulse counter)
    relayOpen = false;
    //digitalWrite(RELEVALV, LOW);
    validityOfData = 4;
  } else if (bottomTankSensor == true) {
    Serial.println("Bottom sensor");
    // we can readjust volume of water left in tank
    litersRemaining = remainderWhenLowSensorHitted;
    validityOfData = 0;
  }
  if (waterTemperature <= conf.minTemp) {
    heatingOn = true;
    digitalWrite(RELEHEAT, HIGH);
  } else if (heatingOn && waterTemperature >= conf.minTemp + 1) {
    heatingOn = true;
    digitalWrite(RELEHEAT, LOW);
  }

  delay(500);

  if (!didCentralSendConfirmation) {
    if ((startTime == -1 && !checkingAgaintsEEPROMmaster) || (getTimeDiffrence(startTime) > (checkingAgaintsEEPROMmaster ? 15000 : 10000))) { // we waited too long to get a
      Serial.println("WU | LOOP | Scanning");
      ScanForCentral();
    } else {
      Serial.println("WU | LOOP | Sending my type to central");
      sendMyTypeToCentral();
      delay(75);
    }
  } else {
    deleteUnactiveCentral();
  }

  // sendDataToUnit every half a minute
  // return to 6
  if (count % 3 == 0 && didCentralSendConfirmation) {
    sendData();
    count = 0;
  }
  count++;
  delay(1000);
}
