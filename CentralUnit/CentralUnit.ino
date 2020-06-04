#include <esp_now.h>        // for enabling ESP NOW wich is used to communicate with units
#include <WiFi.h>           // for ESP NOW
#include <SPI.h>            // for ethernet
#include <WebServer.h>      // for running webserver on Nextion
#include <EEPROM.h>         // for writing data into EEPROM (probably wont be used here)
#include <Time.h>           // for timekeeping
//#include <WiFiUdp.h>        // for WifiUdp
#include <NTPClient.h>      // for syncing time via NTP
#include <Nextion.h>        // for getting data from nextion display
#include <Timezone.h>       // for keeping track of timezones and summer time
#include <M5Stack.h>        // for controling M5Stack functions
#include <Ethernet2.h>      // ethernet library for M5Stack lan module
#include <EthernetUdp2.h>   // for Udp which is needed for NTPClient

#include "Free_Fonts.h"
#include "Water.h"
#include "PowerAndHeating.h"
#include "Wheels.h"
#include "Security.h"
#include "Water.h"
#include "Connection.h"
//#include "Temperatures.h"
#include "Weather.h"

#define EEPROM_SIZE 36   // define EEPROM size -- laveTypesNumber * 7 -- 1 for type (1,2,3,4....), 6 for mac, first byte for declering if something is stored
#define SCK 18
#define MISO 19
#define MOSI 23
#define CS 26
#define CHANNEL 1

WebServer server(80);

EthernetUDP Udp;
//NTPClient timeClient(Udp, "europe.pool.ntp.org", 7200);
NTPClient timeClient(Udp);
bool pairingMode = true; // will effectt whether device starts in pairing mode

TimeChangeRule CEST = { "CEST", Last, Sun, Mar, 2, 120 };     //Central European Summer Time
TimeChangeRule CET = { "CET ", Last, Sun, Oct, 3, 60 };       //Central European Standard Time
Timezone CE(CEST, CET);

String formattedTime;
String dayStamp;
String timeStamp;
const int slaveTypesNumber = 4;

// mac address of LAN unit
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// Zlin network
//IPAddress nm(255, 255, 255, 0);
//IPAddress gw(10, 18, 11, 254);
//IPAddress dnsss( 8, 8, 8, 8);
//IPAddress ip(10, 18, 11, 197);

// Home network
IPAddress nm(255, 255, 255, 0);
IPAddress gw(192, 168, 1, 1);
IPAddress dnsss(8, 8, 8, 8);
IPAddress ip(192, 168, 1, 198);
// need last one whole array will be inicialized with emtpy sicne you cant go back to default value of enum
enum SlaveTypes {
  SECURITY, WATER, WHEELS, HEATINGANDPOWER, EMPTY
};

//Nextion on screen interactive items
// connection
NexButton changeConBtn = NexButton(0, 57, "changeConBtn");  // Button added

// time
NexDSButton btnTimeOffset = NexDSButton(2, 39, "btnTimeOffset");
NexDSButton btnNTP = NexDSButton(2, 65, "btnNTPTime");
NexDSButton btnGPS = NexDSButton(2, 64, "btnGPSTime");
NexNumber numOffset = NexNumber(2, 40, "numOffset");
NexNumber editTime = NexNumber(2, 43, "editTime");
NexNumber numHour = NexNumber(2, 45, "numHour");
NexNumber numMin = NexNumber(2, 44, "numMin");
NexNumber numMonth = NexNumber(2, 47, "numMonth");
NexNumber numDay = NexNumber(2, 46, "numDay");
NexNumber numYear = NexNumber(2, 48, "numYear");
// heating
NexDSButton cycleOneBtn = NexDSButton(1, 49, "cycle1");
NexDSButton cycleTwoBtn = NexDSButton(1, 53, "cycle2");
NexDSButton cycleThreeBtn = NexDSButton(1, 52, "cycle3");
NexDSButton cycleFourBtn = NexDSButton(1, 51, "cycle4");
NexDSButton heatingOnOff = NexDSButton(1, 50, "heatingOnOff");
NexDSButton winterBtn = NexDSButton(1, 48, "winterBtn");
NexNumber airTemp = NexNumber(2, 42, "airTemp");
NexNumber floorTemp = NexNumber(2, 41, "floorTemp");
NexNumber maxFloorTemp = NexNumber(2, 52, "maxTemp");
NexNumber limitFloorTemp = NexNumber(2, 63, "limitFloorTemp");
NexNumber waterTemp = NexNumber(2, 56, "waterTemp");
NexNumber airTempTol = NexNumber(2, 60, "airTempTol");

byte noOfAttempts = 0; // how many times have we tried to establish and verify connection

static bool ethConnected = false;

// millisResetsAfter50Day that would make it impossible (for a while) to remove unactiveChips
long millisOfLastDataRecv;

// two arrays, index, will be same, access only with methods, size is equal to number of enums in SlaveTypes
SlaveTypes slaveTypes[slaveTypesNumber];
esp_now_peer_info_t espInfo[slaveTypesNumber];
unsigned long registryTime[slaveTypesNumber];

esp_now_peer_info_t emptyInfo; // empty info, for when program need to fill something with 0, mostly for my comfort, of course memcpy with 0 would work to

PowerAndHeating powerAndHeating;
Security security;
Water water;
Wheels wheels;
Connection connection;     // unsafe, crashes whole unit
Weather weather(49.233056, 17.666944); // lat and lon of Zlin

NexTouch *nex_listen_list[] = {
  &changeConBtn,  // Button added
  &editTime,
  NULL
};


void NextionCheckHeatingConfiguration() {
  // voda - zima, temp
  if (water.getEstablishedConnection()) {
    powerAndHeating.setUpOuterTemp(water.getOuterTemp());
  } else {
    powerAndHeating.setUpOuterTemp(weather.getOuterTemp());
  }

  uint32_t winterVal; winterBtn.getValue(&winterVal);
  uint32_t waterTempVal; waterTemp.getValue(&waterTempVal);
  bool temp1 = water.setUpSendConf(winterVal, waterTempVal);

  uint32_t heatingVal; heatingOnOff.getValue(&heatingVal);
  uint32_t cycle1Val; cycleOneBtn.getValue(&cycle1Val);
  uint32_t cycle2Val; cycleTwoBtn.getValue(&cycle2Val);
  uint32_t cycle3Val; cycleThreeBtn.getValue(&cycle3Val);
  uint32_t cycle4Val; cycleFourBtn.getValue(&cycle4Val);
  uint32_t airTempVal; airTemp.getValue(&airTempVal);
  uint32_t airTempTolVal; airTempTol.getValue(&airTempTolVal);
  uint32_t floorTempMaxVal;  maxFloorTemp.getValue(&floorTempMaxVal);
  uint32_t limitFloorTempVal; limitFloorTemp.getValue(&limitFloorTempVal);
  bool temp2 = powerAndHeating.setUpSendConf(heatingVal, winterVal, cycle1Val, cycle2Val, cycle3Val, cycle4Val, airTempVal, airTempTolVal, floorTempMaxVal, limitFloorTempVal);
  if (temp2 || temp1) sendConfigurationt();
}

void sendConfigurationt() {
  uint8_t data = powerAndHeating.getDataToBeSend();
  esp_err_t result = esp_now_send(getEspInfoForType(HEATINGANDPOWER).peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
  delay(50);
  data = water.getDataToBeSend();
  result = esp_now_send(getEspInfoForType(WATER).peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
}

void changeConBtnPopCallback(void *ptr) {
  Serial.println("CU | changeConBtnPopCallback");
  connection.changeConnection();
}

void timeManualCallback(void *ptr) {
  Serial.println("CU | timeManualCallback");
  uint32_t stateEditTime;
  editTime.getValue(&stateEditTime);
  uint32_t stateNTP;
  btnNTP.getValue(&stateNTP);
  uint32_t stateGPS;
  btnGPS.getValue(&stateGPS);
  if (!stateEditTime && !stateNTP) { // dispaly time on manual setup, if editing is off
    uint32_t minN; numMin.getValue(&minN);
    uint32_t hourN; numHour.getValue(&hourN);
    uint32_t dayN; numDay.getValue(&dayN);
    uint32_t monthN; numMonth.getValue(&monthN);
    uint32_t yearN; numYear.getValue(&yearN);
    setTime(hourN, minN, 0, dayN, monthN, yearN);
  }
}

// end nextion command, also starts in case something was in serial line
void startEndNextionCommand() {
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

// millis() counter resets every 50 days, gives time diffrence between millis() and sTime in argument
unsigned long getTimeDiffrence(unsigned long sTime) {
  if (millis() < sTime) {
    return (ULONG_MAX - sTime) + millis();
  }
  return millis() - sTime;
}

// returns esp info that corresponds with given type (must be in arrays)
esp_now_peer_info_t getEspInfoForType(SlaveTypes type) {
  for (int i = 0; i < (sizeof(slaveTypes) / sizeof(slaveTypes[0])); i++) {
    if (type == slaveTypes[i]) {
      return espInfo[i];
    }
  }
  return emptyInfo;
}

// returns index in SlaveTypes for given type, used in case we want to remove it
int getIndexInSlaveTypes(SlaveTypes type) {
  for (int i = 0; i < (sizeof(slaveTypes) / sizeof(slaveTypes[0])); i++) {
    if (type == slaveTypes[i]) {
      return i;
    }
  }
  return -1; // TODO: check for colision futher on
}

// returns SlaveTypes that corresponds with mac_addr in argument (comparing in espInfo)
SlaveTypes getSlaveTypeForMAC(const uint8_t *mac_addr) { //// propably does not work
  for (int i = 0; i < slaveTypesNumber; i++) {
    //if(*mac_addr == *espInfo[i].peer_addr){
    //    return slaveTypes[i];
    //}
    if (checkIfTwoAddressesAreSame(mac_addr, espInfo[i].peer_addr)) {
      return slaveTypes[i];
    }
  }
  return EMPTY;
}

// calls all updateDataOnNexion function
void refreshScreen() {
  displayTime();
  weather.updateDataOnNextion();
  connection.updateDataOnNextion();
  water.updateDataOnNextion();
  security.updateDataOnNextion();
  powerAndHeating.updateDataOnNextion();
  wheels.updateDataOnNextion();
}

// checks if we had already registred MAC in espInfo
bool doesntContainMac(uint8_t addr[]) {
  for (int i = 0; i < slaveTypesNumber; i++) {
    if (checkIfTwoAddressesAreSame(addr, espInfo[i].peer_addr)) {
      Serial.println("CU | doesntContainMac | Mac address is already stored");
      return false;
    }
  }
  return true;
}

// checks if two addresses (arrays) are same
boolean checkIfTwoAddressesAreSame(const uint8_t *addr1, const uint8_t *addr2) {
  if (sizeof(addr1) != sizeof(addr2)) {
    return false;
  }
  for (int i = 0; i < (sizeof(addr1) / sizeof(addr1[0])); i++) {
    if (addr1[i] != addr2[i]) return false;
  }
  return true;
}

// checks if given type already has its esp info
bool doesArrayAlreadyContainsType(SlaveTypes type) {
  for (int i = 0; i < slaveTypesNumber; i++) if (slaveTypes[i] == type) return true;
  return false;
}

// adds new entry into slaveTypes and espInfo, used after unit send its type
void addNewUnitToArray(esp_now_peer_info_t newUnitInfo, uint8_t type) {
  for (int i = 0; i < slaveTypesNumber; i++) {
    if (slaveTypes[i] == EMPTY) {
      switch (type) {
        case 1:
          if (doesArrayAlreadyContainsType(SECURITY)) {
            Serial.println("CU | addNewUnitToArray | You are trying to add type that is already stored");
            break;
          }
          slaveTypes[i] = SECURITY;
          memcpy (&espInfo[i], &newUnitInfo, sizeof(newUnitInfo)); // copies data to array
          Serial.println("CU | addNewSlaveToArray | Added SECURITY");
          security.setEstablishedConnection(true); security.updateLastTimeRecived();
          Serial.print("CU | addNewSlaveToArray | index is: "); Serial.print(i);
          Serial.print(", MAC address of security is "); printAddress(espInfo[i].peer_addr); Serial.println("");
          storeUnitInEEPROM(espInfo[i], 1); sendConformationToUnit(i);
          startEndNextionCommand(); Serial2.print("t21.pco=2016"); startEndNextionCommand();
          i = slaveTypesNumber;
          break;
        case 2:
          if (doesArrayAlreadyContainsType(WATER)) {
            Serial.println("CU | addNewUnitToArray | You are trying to add type that is already stored");
            break;
          }
          slaveTypes[i] = WATER;
          memcpy (&espInfo[i], &newUnitInfo, sizeof(newUnitInfo));
          Serial.println("CU | addNewSlaveToArray | Added WATER");
          water.setEstablishedConnection(true); water.updateLastTimeRecived();
          Serial.print("CU | addNewSlaveToArray | index is: "); Serial.print(i);
          Serial.print(", MAC address of water is "); printAddress(espInfo[i].peer_addr); Serial.println("");
          storeUnitInEEPROM(espInfo[i], 2); sendConformationToUnit(i);
          startEndNextionCommand(); Serial2.print("t16pco=2016"); startEndNextionCommand();
          i = slaveTypesNumber;
          break;
        case 3:
          if (doesArrayAlreadyContainsType(WHEELS)) {
            Serial.println("CU | addNewUnitToArray | You are trying to add type that is already stored");
            break;
          }
          slaveTypes[i] = WHEELS;
          memcpy (&espInfo[i], &newUnitInfo, sizeof(newUnitInfo));
          Serial.println("CU | addNewSlaveToArray | Added WHEELS");
          wheels.setEstablishedConnection(true); wheels.updateLastTimeRecived();
          Serial.print("CU | addNewSlaveToArray | index is: "); Serial.print(i);
          Serial.print(", MAC address of wheels is "); printAddress(espInfo[i].peer_addr); Serial.println("");
          storeUnitInEEPROM(espInfo[i], 3); sendConformationToUnit(i);
          i = slaveTypesNumber;
          break;
        case 4:
          if (doesArrayAlreadyContainsType(HEATINGANDPOWER)) {
            Serial.println("CU | addNewUnitToArray | You are trying to add type that is already stored");
            break;
          }
          slaveTypes[i] = HEATINGANDPOWER;
          memcpy (&espInfo[i], &newUnitInfo, sizeof(newUnitInfo));
          Serial.println("CU | addNewSlaveToArray | Added HEATINGANDPOWER");
          powerAndHeating.setEstablishedConnection(true); powerAndHeating.updateLastTimeRecived();
          Serial.print(", index is: "); Serial.print(i);
          Serial.print(", MAC address of powerAndHeating is "); printAddress(espInfo[i].peer_addr); Serial.println("");
          storeUnitInEEPROM(espInfo[i], 4); sendConformationToUnit(i);
          startEndNextionCommand(); Serial2.print("t23.pco=2016"); startEndNextionCommand();
          startEndNextionCommand(); Serial2.print("t24.pco=2016"); startEndNextionCommand();
          i = slaveTypesNumber;
          break;
      }
    }
  } // resets address so we wont in case for ifs in callbacks
}

// prints given mac address
void printAddress(const uint8_t addr[]) {
  for (int i = 0; i < 6; ++i ) {
    Serial.print((uint8_t) addr[i], HEX);
    if (i != 5) Serial.print(":");
  }
  Serial.println();
}

// root of server running on M5Stack
void handleRoot() {
  server.send(200, "text/plain", "This feature is yet to be implemeted");
}

// callback for WiFi on handle not found
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// Inits ESPNow
void initESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("CU | initESPNow | ESPNow Init Success");
  }
  else {
    Serial.println("CU | initESPNow | ESPNow Init Failed");
    ESP.restart();
  }
}

// set ups AP and tries to pair with units loaded from EEPROM
void configDeviceAP() {
  WiFi.softAPdisconnect(1);
  const char *SSID = "CARAVAN_CENTRAL_UNIT";
  bool result = WiFi.softAP(SSID, "supersecretpassword", CHANNEL, !pairingMode);
  if (!result) {
    Serial.println("CU | configDeviceAP | AP Config failed.");
  } else {
    Serial.print("CU | configDeviceAP | AP Config Success. Broadcasting with AP: " + String(SSID) + ", pairing Mode is: "); Serial.println(pairingMode);
  }
  initESPNow();
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  for (int i = 0; i < slaveTypesNumber; i++) { // after ESPNow init we need to repair with each unit
    if (slaveTypes[i] != EMPTY) {
      esp_err_t addStatus = esp_now_add_peer(&espInfo[i]);
      if (addStatus != ESP_OK) {
        Serial.print("CU | configDeviceAP | pairing failed, tried to pair with: "); Serial.println(int(slaveTypes[i]));
      }
    }
  }
}

// remove peer (unpair)
void deletePeer(esp_now_peer_info_t toDelete) {
  esp_err_t delStatus = esp_now_del_peer(toDelete.peer_addr);
  Serial.print("CU | deletePeer | Slave Delete Status: ");
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

// send data to unit specified in argument
void sendData(SlaveTypes type) {
  uint8_t dataToBeSent;
  switch (type) {
    case SECURITY: {
        dataToBeSent = security.getDataToBeSend();
        break;
      }
    case WATER: {
        dataToBeSent = water.getDataToBeSend();
        break;
      }
    case WHEELS: {
        dataToBeSent = wheels.getDataToBeSend();
        break;
      }
    case HEATINGANDPOWER: {
        dataToBeSent = powerAndHeating.getDataToBeSend();
        break;
      }
    default: {
        dataToBeSent = 0;
        break;
      }
  }
  esp_err_t result = esp_now_send(getEspInfoForType(type).peer_addr, &dataToBeSent, sizeof(dataToBeSent));
  Serial.print("CU | sendData |Send Status: ");
  if (result == ESP_OK) {
    Serial.println("Success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    // How did we get so far!!
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

// callback for when data is sent from Master to Slave
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("CU | onDataSent | Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("CU | onDataSent | Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// sendsConformationTo given unit (given by index in array EspInfo), used after recieving message from it
void sendConformationToUnit(byte indexInEspInfo) {
  Serial.println("CU | sendConformationToUnit | sending to unit: " + String(indexInEspInfo));
  uint8_t data = 92;
  esp_err_t result = esp_now_send(espInfo[indexInEspInfo].peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
  Serial.print("CU | sendConformationToUnit | Send Status: ");
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
  } else {
    Serial.println("Not sure what happened");
  }
}


// prints whole EEPROM
void pritnEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    Serial.print((uint8_t) EEPROM.read(i), HEX);
  }
  Serial.println();
}

// first byte in/off EEPROM, 1 - 6 is masters mac
// stores unit on first empty space, each unit takes 7 bytes, first byte is used as indicator whether something was written.
void storeUnitInEEPROM(esp_now_peer_info toStore, uint8_t type) { // will be called after new unit was addded
  if (EEPROM.read(0) == 0) { // limited number of writes
    EEPROM.write(0, 1);
  }
  for (int i = 1 ; i < (slaveTypesNumber * 7 + 1); i++) {
    if (EEPROM.read(i) == 0) {
      Serial.println("CU | storeUnitInEEPROM | Storing new unit to EEPROM");
      EEPROM.write(i, type);
      for (int j = i + 1; j <= i + 6; j++ ) EEPROM.write(j, toStore.peer_addr[j - i - 1]); // store new data into EEPROM
    } else if (EEPROM.read(i) == type) {
      uint8_t test[6];
      bool change = false;
      for (int j = i + 1 ; j <= i + 6; j++ ) if (toStore.peer_addr[j - i - 1] != (uint8_t) EEPROM.read(j)) {
          change = true;  // check if address stored is different
          Serial.println("CU | storeUnitInEEPROM | rewriting old unit in EEPROM");
          break;
        }
      if (change) for (int j = i + 1; j <= i + 6; j++ ) EEPROM.write(j, toStore.peer_addr[j - i - 1]); // store new data into EEPROM
    }
  }
  // check if data is same
  EEPROM.commit();
}

bool loadDataFromEEPROM() {
  if (EEPROM.read(0) == 0) {
    Serial.println("CU | loadDataFromEEPROM | EEPROM is empty");
    return false;
  } else {
    Serial.println("CU | loadDataFromEEPROM | loading data");
    for (int i = 1; i < (slaveTypesNumber * 7 + 1); i += 7) {
      uint8_t type = EEPROM.read(i);
      esp_now_peer_info_t temp;
      temp.channel = 1;
      temp.encrypt = 0;
      for (int j = i + 1; j <= i + 6; j++) {
        temp.peer_addr[j - i - 1] = (uint8_t) EEPROM.read(j);
      }
      if (getSlaveTypeForMAC(temp.peer_addr) == EMPTY) {
        addNewUnitToArray(temp, type);
      } else {
        Serial.println("CU | loadDataFromEEPROM | we already know that unit");
      }
    }
    return true;
  }
}

esp_now_peer_info_t toAdd;
// callback for when data is recived from sensor unit
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);


  Serial.println();
  Serial.print("CU | onDataRecv  | Last Packet Recv from:   "); Serial.println(macStr);
  Serial.print("CU | onDataRecv  | Last Packet Recv lenght: "); Serial.println(data_len);

  byte temp = (*data - 100);
  if ( temp > 0 && temp <= slaveTypesNumber) {
    if (getSlaveTypeForMAC(mac_addr) == EMPTY) {
      Serial.print("CU | onDataRecv | Got new unit ist mac is: "); printAddress(mac_addr); Serial.println("");

      toAdd.channel = 1;
      toAdd.encrypt = 0;
      memcpy(toAdd.peer_addr, mac_addr, sizeof(toAdd.peer_addr));

      Serial.print("CU | onDataRecv | mac of toAdd is: "); printAddress(toAdd.peer_addr); Serial.println("");
      if (!esp_now_is_peer_exist(toAdd.peer_addr)) {
        esp_err_t addStatus = esp_now_add_peer(&toAdd);
        if (addStatus == ESP_OK) {
          Serial.print("CU | onDataRecv | Pairing was cuccesfull, paired with: "); Serial.println(temp);
        } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
          Serial.println("CU | onDataRecv | Pairing  failed, ESPNOW Not Init");
          initESPNow();
        } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
          Serial.println("CU | onDataRecv | Pairing  failed, Invalid Argument");
        } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
          Serial.println("CU | onDataRecv | Pairing  failed, Peer list full");
        } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
          Serial.println("CU | onDataRecv | Pairing  failed, Out of memory");
        } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
          Serial.println("CU | onDataRecv | Pairing  failed, Peer Exists");
        } else {
          Serial.println("CU | onDataRecv | Pairing  failed, Not sure what happened");
        }
      }
      if (esp_now_is_peer_exist(toAdd.peer_addr)) addNewUnitToArray(toAdd, temp);
    } else {
      Serial.println("CU | onDataRecv | got data from unit that i know, sending confirmation");
      sendConformationToUnit(getIndexInSlaveTypes(getSlaveTypeForMAC(mac_addr)));
    }
  } else {
    bool validMessage = true;
    switch (getSlaveTypeForMAC(mac_addr)) {
      case SECURITY:
        Serial.println("CU | onDataRecv | got new data for security");
        if (security.updateYourData(data)) security.updateLastTimeRecived();
        else validMessage = false;
        break;
      case WATER:
        Serial.println("CU | onDataRecv | got new data for water");
        if (water.updateYourData(data)) water.updateLastTimeRecived();
        else validMessage = false;
        break;
      case WHEELS:
        Serial.println("CU | onDataRecv | got new data for wheels");
        if (wheels.updateYourData(data)) wheels.updateLastTimeRecived();
        else validMessage = false;
        break;
      case HEATINGANDPOWER:
        Serial.println("CU | onDataRecv | got new data for powerAndHeating");
        if (powerAndHeating.updateYourData(data)) powerAndHeating.updateLastTimeRecived();
        else validMessage = false;
        break;
      default:
        Serial.println("CU | onDataRecv | not right mac, getSlaveForMac retruned EMPTY");
        validMessage = false;
        break;
    }
    if (validMessage) {
      // doesnt matter if millis doesnt correspond it wont make a diffrence;
      millisOfLastDataRecv = millis();
    } else {
      Serial.println("CU | onDataRecv | Data was invalid");
    }
  }

  Serial.println();
}

// remove unit that is fully set up
void removeUnit(SlaveTypes type) {
  deletePeer(getEspInfoForType(type));
  int index = getIndexInSlaveTypes(type);
  slaveTypes[index] = EMPTY;
  espInfo[index] = emptyInfo;
}

// removes unactive units that didnt send any data (right data) in last 4 minutes
void removeUnactiveUnits() {
  if (getTimeDiffrence(security.getLastTimeRecived()) > 240000 && security.getEstablishedConnection()) {
    removeUnit(SECURITY);
    security.setEstablishedConnection(false); // write something on nextion
  }
  if (getTimeDiffrence(water.getLastTimeRecived()) > 240000 && water.getEstablishedConnection()) {
    removeUnit(WATER);
    water.setEstablishedConnection(false);
  }
  if (getTimeDiffrence(wheels.getLastTimeRecived()) > 240000 && wheels.getEstablishedConnection()) {
    removeUnit(WHEELS);
    wheels.setEstablishedConnection(false);
  }
  if (getTimeDiffrence(powerAndHeating.getLastTimeRecived()) > 240000 && powerAndHeating.getEstablishedConnection()) {
    removeUnit(HEATINGANDPOWER);
    powerAndHeating.setEstablishedConnection(false);
  }
}

// pings each unit with number 88, units that way known that central is pressent
// they do not operate on callback when data is send succesfully
void pingEachSesnorUnit() {
  uint8_t data = 88;
  for (int i; i < slaveTypesNumber; i++) {
    esp_now_send(espInfo[i].peer_addr, &data, sizeof(data));
  }
}

// Maybe could be moved to its own class
// updates time via NTP cilent
void updateTime() {
  uint32_t stateNTP;
  btnNTP.getValue(&stateNTP);

  uint32_t stateGPS;
  btnGPS.getValue(&stateGPS);

  if (stateGPS || stateNTP) {
    if (security.getEstablishedConnection() && stateGPS) {
      setTime(security.getTime());
    } else { // if security wasn't connected we will use NTP, even in case when GPS is set on
      byte  tries = 0; // while with timeClient.update() can result in infinite loop (some internal problem of library), so just kill it after few tries
      int   triesTime = millis();
      //timeClient.setTimeOffset(setOffSetForSummerTime());
      while (!timeClient.update() && tries < 5 && getTimeDiffrence(triesTime) < 5000) {
        timeClient.forceUpdate();
        Serial.println("CU | updateTime | updated");
        tries++;
      }
      // get unix time and sets it into Time.h for timekeeping
      setTime(timeClient.getEpochTime());
    }

    uint32_t state;
    btnTimeOffset.getValue(&state);
    if (state) {
      setTime(CE.toLocal(now()));
    } else {
      uint32_t number;
      numOffset.getValue(&number);
      adjustTime(number * 3600);
    }
  }
  Serial.print("CU | updateTime | time is: ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.println(second());
}

// displys time on nextion
void displayTime() {

  String command;
  startEndNextionCommand();
  command = hour() < 10 ? "textHours.txt=\"0" + String(hour()) + "\"" : "textHours.txt=\"" + String(hour()) + "\"";
  //command = "textHours.txt=\""+String(hour())+"\"";
  Serial2.print(command);
  startEndNextionCommand();
  //Serial.println(minute());
  command = minute() < 10 ? "textMinutes.txt=\"0" + String(minute()) + "\"" : "textMinutes.txt=\"" + String(minute()) + "\"";
  //command = "textMinutes.txt=\""+String(minute())+"\"";
  Serial2.print(command);
  startEndNextionCommand();
  //command = day() < 10 ? "textAccuracy.txt=\"0"+String(day())+"\"" : "textAccuracy.txt=\""+String(day())+"\"";
  command = "textDay.txt=\"" + String(day()) + "\"";
  Serial2.print(command);
  startEndNextionCommand();
  //command = month() < 10 ? "textAccuracy.txt=\"0"+String(month())+"\"" : "textAccuracy.txt=\""+String(month())+"\"";
  command = "textMonth.txt=\"" + String(month()) + "\"";
  Serial2.print(command);
  startEndNextionCommand();
  uint32_t stateEditTime;
  editTime.getValue(&stateEditTime);
  if (!stateEditTime) { // dispaly time on manual setup, if editing is off
    String command;
    command = "numYear.val=" + String(year());
    Serial2.print(command);
    startEndNextionCommand();
    command = "numMin.val=" + String(minute());
    Serial2.print(command);
    startEndNextionCommand();
    command = "numHours.val=" + String(hour());
    Serial2.print(command);
    startEndNextionCommand();
    command = "numDay.val=" + String(day());
    Serial2.print(command);
    startEndNextionCommand();
    command = "numMonth.val=" + String(month());
    Serial2.print(command);
    startEndNextionCommand();
  }
}


void setup() {
  M5.begin(true, false, true);
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  while (!Serial);
  WiFi.mode(WIFI_AP_STA);
  // This is the mac address of the Master in Station Mode

  for (int i = 0; i < slaveTypesNumber; i++) {
    slaveTypes[i] = EMPTY;
  }

  configDeviceAP();
  Serial.print("CU | SETUP | AP MAC: "); Serial.println(WiFi.softAPmacAddress());

  SPI.begin(SCK, MISO, MOSI, -1);
  delay(1000);
  Ethernet.init(CS);
  delay(1000);
  Ethernet.begin(mac, ip, dnsss, gw , nm);
  Udp.begin(8888);

  /*
    server.on("/", handleRoot);
    server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
    });
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("Server started");
  */

  M5.Lcd.clear(BLACK);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(3, 35);
  M5.Lcd.println("Press button A for 300ms");
  M5.Lcd.println("to enter/exit pairing mode");
  M5.Lcd.println("!! Pairing mode si on");
  M5.Lcd.println("on start");
  M5.Lcd.println("Press button B for 300ms");
  M5.Lcd.println("to clear EEPROM");

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("CU | SETUP | failed to initialize EEPROM");
    M5.Lcd.println("EEPROM is down");
  } else {
    loadDataFromEEPROM();
  }
  nexInit();
  changeConBtn.attachPop(changeConBtnPopCallback, &changeConBtn);
  editTime.attachPop(timeManualCallback, &editTime);

  connection.displayUnknownState();
  startEndNextionCommand();
  Serial2.print("maxTemp.val=30");
  startEndNextionCommand();


  timeClient.begin();
  updateTime();
  weather.update();
  //weather.usePictureOnNextion(true);
  Serial.println("CU | SETUP | FINISHED");
}

int interationCounter = 0;

void loop() {
  M5.update();

  nexLoop(nex_listen_list);

  if (M5.BtnA.wasReleased()) {
    pairingMode = !pairingMode;
    configDeviceAP();
  }
  if (M5.BtnB.wasReleased() && EEPROM.read(0) == 1) {
    Serial.println("CU | LOOP | EEPROM was cleared");
    EEPROM.write(0, 0);
  }
  if (interationCounter % 10 = 0) {
    sendConfigurationt(); // sending can fail so its better to send conf reguraly
  }
  if (interationCounter == 0) {
    updateTime();
    if (security.getIsPositionKnown()) weather.setNewPosition(security.getLatitude(), security.getLongitude());
    NextionCheckHeatingConfiguration();
    weather.update();
    refreshScreen();
    interationCounter = 19;
    pingEachSesnorUnit();
  }

  displayTime();

  //connection.getStateOfConnection();
  //sendData(WATER);
  removeUnactiveUnits();
  interationCounter--;
  delay(500);
}
