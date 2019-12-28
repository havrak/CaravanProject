/*
    Code for central unit, runs on OLIMEX Gateway
*/

//#include <ETH.h>        // for ethernet to work
#include <esp_now.h>    // for enabling ESP NOW wich is used to communicate with units
#include <WiFi.h>       // for ESP NOW
#include <SPI.h>        // for ethernet
#include <WebServer.h>  // for running webserver on Olimex
#include <EEPROM.h>     // for writing data into EEPROM (probably wont be used here)
#include <Time.h>       // for timekeeping
#include <WiFiUdp.h>    // for WifiUdp - as argument in timeCliet
#include <NTPClient.h>  // for syncing time via NTP
#include <Nextion.h>    // for getting data from nextion display
#include <Timezone.h>   // for keeping track of timezones and summer time
#include <M5Stack.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>

#include "Free_Fonts.h"
#include "Water.h"
#include "Heating.h"
#include "Power.h"
#include "Wheels.h"
#include "Security.h"
#include "Water.h"
#include "Connection.h"
#include "Temperatures.h"
#include "Weather.h"

#define EEPROM_SIZE 1   // define EEPROM size
#define SCK 18
#define MISO 19
#define MOSI 23
#define CS 26

//01 05 00 01 02 00 9d 6a
char uart_buffer[8] = {0x01, 0x05, 0x00, 0x01, 0x02, 0x00, 0x9d, 0x6a};
char uart_rx_buffer[8] = {0};
 
char Num = 0;
char stringnum = 0;
unsigned long W5500DataNum = 0;
unsigned long Send_Num_Ok = 0;
unsigned long Rec_num = 0;
unsigned long Rec_Num_Ok = 0;

WebServer server(80);      // worked

EthernetUDP Udp;               // worked
NTPClient timeClient(Udp, "europe.pool.ntp.org", 3600); // worked

TimeChangeRule CEST = { "CEST", Last, Sun, Mar, 2, 120 };     //Central European Summer Time
TimeChangeRule CET = { "CET ", Last, Sun, Oct, 3, 60 };       //Central European Standard Time
Timezone CE(CEST, CET);

String formattedTime;
String dayStamp;
String timeStamp;
const int slaveTypesNumber = 5;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress nm(255, 255, 255, 0); 
IPAddress gw(10, 18, 11, 254);
IPAddress dnsss( 8, 8, 8, 8); 
IPAddress ip(10, 18, 11, 197);

// need last one whole array will be inicialized with emtpy sicne you cant go back to default value of enum
enum SlaveTypes{
  SECURITY,WATER,WHEELS,HEATING,POWER,EMPTY
};

// TODO: find out how callbacks are handled
// array of boolean prevents updating info in water.h by callbacks if there is new configuration being recived from olimex
bool updateLocks[slaveTypesNumber];

//Nextion on screen interactive items
NexButton b1 = NexButton(0, 9, "b1");  // Button added
NexTouch *nex_listen_list[] = {
  &b1,  // Button added
};

byte noOfAttempts = 0; // how many times have we tried to establish and verify connection

static bool ethConnected = false;

// millisResetsAfter50Day that would make it impossible (for a while) to remove unactiveChips
long millisOfLastDataRecv;

// two arrays, index, will be same, access only with methods, size is equal to number of enums in SlaveTypes
SlaveTypes slaveTypes[slaveTypesNumber];
esp_now_peer_info_t espInfo[slaveTypesNumber];

esp_now_peer_info_t untypedPeers[20]; // max nuber of untyped peers

esp_now_peer_info_t peerToBePairedWith; // wont be neccesseary here

esp_now_peer_info_t emptyInfo; // empty info, for when program need to fill something with 0, mostly for my comfort, of course memcpy with 0 would work to

Power power;
Security security;
Water water;
Wheels wheels;
Heating heating;
Connection connection;     // unsafe, crashes whole unit
Temperatures temperatures;
Weather weather(49.233056,17.666944);

void b1PushCallback(void *ptr){
  digitalWrite(13, HIGH);  // Turn ON internal LED
}  // End of press event

void b1PopCallback(void *ptr){
  digitalWrite(13, LOW);  // Turn OFF internal LED
}

// end nextion command, also starts in case something was in serial line
void startEndNextionCommand(){
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

// Cloud cause problems
// returns index of mac_addr in untypedPeers, return -1 if mac was not found
int getIndexOfUntyped(const uint8_t *mac_addr){
  for(int i; i< (sizeof(untypedPeers)/sizeof(untypedPeers[0])); i++){
    if(*mac_addr == *untypedPeers[i].peer_addr){
        return i;
      }
  }
  return -1; // TODO: check for colision futher on
}

// returns esp info that corresponds with given type (must be in arrays)
esp_now_peer_info_t getEspInfoForType(SlaveTypes type){
  for(int i = 0; i < (sizeof(slaveTypes)/sizeof(slaveTypes[0])); i++){
    if(type == slaveTypes[i]){
      return espInfo[i];
    }
  }
  return emptyInfo;
}

// returns index in SlaveTypes for given type, used in case we want to remove it
int getIndexInSlaveTypes(SlaveTypes type){
  for(int i = 0; i < (sizeof(slaveTypes)/sizeof(slaveTypes[0])); i++){
    if(type == slaveTypes[i]){
      return i;
    }
  }
  return -1; // TODO: check for colision futher on
}

// returns SlaveTypes that corresponds with mac_addr in argument (comparing in espInfo)
SlaveTypes getSlaveTypeForMAC(const uint8_t *mac_addr){ //// propably does not work
  for(int i = 0; i< slaveTypesNumber; i++){
    //if(*mac_addr == *espInfo[i].peer_addr){
    //    return slaveTypes[i];
    //}
    if(checkIfTwoAddressesAreSame(mac_addr, espInfo[i].peer_addr)){
      return slaveTypes[i];  
    }
  }
  return EMPTY;
}

// checks if we had already registred MAC in espInfo
bool doesntContainMac(uint8_t addr[]){
  for(int i = 0; i < slaveTypesNumber; i++){
    if(checkIfTwoAddressesAreSame(addr, espInfo[i].peer_addr)){
        Serial.println("CU | doesntContainMac | Mac address is already stored");
        return false;
    }
  }
  return true;
}

// checks if two addresses are same
boolean checkIfTwoAddressesAreSame(const uint8_t *addr1,const uint8_t *addr2){
  Serial.print("CU | checkIfTwoAddressesAreSame | Address 1: "); printAddress(addr1);
  Serial.print("CU | checkIfTwoAddressesAreSame | Address 2: "); printAddress(addr2);
  if(sizeof(addr1) != sizeof(addr2)){
    Serial.println("checkIfTwoAddressesAreSame in CU | diffrent size");
    return false;
  }
  for(int i = 0; i < (sizeof(addr1)/sizeof(addr1[0])); i++){
    if(addr1[i] != addr2[i]) return false;
  }
  return true;
}

// adds new entry into slaveTypes and espInfo, used after unit send its type
void addNewSlaveToArray(int index, uint8_t type){
  for(int i = 0; i < slaveTypesNumber; i++){
    if(slaveTypes[i] == EMPTY){
      switch(type){
        case 1:
          slaveTypes[i] = SECURITY;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith)); // copies data to array
          Serial.println("CU | addNewSlaveToArray | Added SECURITY ESP32");
          security.setEstablishedConnection(true);
          Serial.print("CU | addNewSlaveToArray | index is: "); Serial.print(i);
          Serial.print(", MAC address of security is "); printAddress(espInfo[i].peer_addr);
          i = slaveTypesNumber;
          break;
        case 2:
          slaveTypes[i] = WATER;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("CU | addNewSlaveToArray | Added WATER ESP32");
          water.setEstablishedConnection(true);
          Serial.print("CU | addNewSlaveToArray | index is: "); Serial.print(i);
          Serial.print(", MAC address of water is "); printAddress(espInfo[i].peer_addr);
          i = slaveTypesNumber;
          break;
        case 3:
          slaveTypes[i] = WHEELS;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("CU | addNewSlaveToArray | Added WHEELS ESP32");
          wheels.setEstablishedConnection(true);         
          Serial.print("CU | addNewSlaveToArray | index is: "); Serial.print(i);
          Serial.print(", MAC address of wheels is "); printAddress(espInfo[i].peer_addr);
          i = slaveTypesNumber;
          break;
        case 4:
          slaveTypes[i] = HEATING;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("CU | addNewSlaveToArray | Added HEATING ESP32");
          heating.setEstablishedConnection(true);         
          Serial.print("CU | addNewSlaveToArray | index is: "); Serial.print(i);
          Serial.print(", MAC address of heating is "); printAddress(espInfo[i].peer_addr);
          i = slaveTypesNumber;
          break;
        case 5:
          slaveTypes[i] = POWER;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("CU | addNewSlaveToArray | Added POWER ESP32");
          power.setEstablishedConnection(true);
          Serial.print("CU | addNewSlaveToArray | index is: "); Serial.print(i);
          Serial.print(", MAC address of power is "); printAddress(espInfo[i].peer_addr);
          i = slaveTypesNumber;
          break;
      }
    }
  } // resets address so we wont in case for ifs in callbacks
}

// prints given mac address 
void printAddress(const uint8_t addr[]){
  for (int i = 0; i < 6; ++i ) {
    Serial.print((uint8_t) addr[i], HEX);
    if (i != 5) Serial.print(":");
  }
  Serial.println();
}

// root of server running on Olimex
void handleRoot() {
  server.send(200, "text/plain", "hello from esp32!");
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


// callback for WiFiEvent
/*
void WiFiEvent(WiFiEvent_t event){
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
//      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH__IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      ethConnected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      ethConnected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      ethConnected = false;
      break;
    default:
      break;
  }
}
*/

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

// adjust number in case millis overflow happend 
long adujistNumberIfTimeOverFlowed(long toBeAdjusted){
  return ULONG_MAX - toBeAdjusted + millis();
}

// scans network, finds all ESP32 unit
// after unit is calls AttempToPair() for that unit()
void ScanForSlave() {
  int8_t scanResults = WiFi.scanNetworks();
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    //Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");

    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      //Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
      delay(60);

      // Check if the current device starts with `Slave`
      if (SSID.indexOf("ESPNOW") == 0) {
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          // we have mac, now we create temp slave and attemp to pair
          for (int ii = 0; ii < 6; ++ii ) {
            peerToBePairedWith.peer_addr[ii] = (uint8_t) mac[ii];
          }
          peerToBePairedWith.channel = 1; // pick a channel
          peerToBePairedWith.encrypt = 0;
          // attempts to pair to found slave

          // TODO: stop function here;
          attempToPair();
          // can be moved to callbacks after sending message here needs to be boolean to stop
          // moving to next found STA
          delay(10);
        }
      }
    }
  }
  // clean up ram
  WiFi.scanDelete();
}

// Checks if the slave is already paired with the master.
// If not, than it pairs the unit with master and adds unit to untypedPeers 
// than will send request for conformation same goes for when unit is paired but we didn't recived any info
// units aren't (for now) removed from untypedPeers
void attempToPair() {
  Serial.print("Processing: ");
  for (int ii = 0; ii < 6; ++ii ) {
    Serial.print((uint8_t) peerToBePairedWith.peer_addr[ii], HEX);
    if (ii != 5) Serial.print(":");
  }
  Serial.print(", Status:");

  // check if the peer exists
  bool exists = esp_now_is_peer_exist(peerToBePairedWith.peer_addr);
  if (exists) {
    // Slave already paired.
    Serial.println("It exists");
    delay(10);
    // maybe check is mac is in table
    if(doesntContainMac(peerToBePairedWith.peer_addr)){
        //Serial.println("Doesn't contain mac");
        sendDataToGetDeviceInfo(getIndexOfUntyped(peerToBePairedWith.peer_addr));
    }
    Serial.println("Already Paired");
  } else {
    // Slave not paired, attempt pair
    Serial.println("Pairing");
    esp_err_t addStatus = esp_now_add_peer(&peerToBePairedWith);
    if (addStatus == ESP_OK) {
      Serial.println("Paired");
      // Pair success here message will be send and changed boolean/ metod for recv data
      // also need to change sendCallback - that will check mac if failed than will data be sent again
      // recv data also will check mac address - than special behaveior
      // NEED TO CLEAN peerToBePairedWith !!!!!
      // check form of mac (What type)

      Serial.println("Pair success"); // attempt to pair ends

      // peer will be added to array of peers we are expecting answer from


      // can add only once, address is paired
      for(int i = 0; i < (sizeof(untypedPeers)/ sizeof(untypedPeers[0])); i++){
        printAddress(untypedPeers[i].peer_addr); Serial.print(" and "); printAddress(emptyInfo.peer_addr);
        if(checkIfTwoAddressesAreSame(untypedPeers[i].peer_addr, emptyInfo.peer_addr)){ // will this work???
          memcpy(&untypedPeers[i], &peerToBePairedWith, sizeof(peerToBePairedWith));
          i = sizeof(untypedPeers);
          sendDataToGetDeviceInfo(i);
          break; // exit for loop
        }
        Serial.println();
      }

      // TODO: after couple of retries attemToPair should exit maybe 3, than delete peer
      // Should failiurese be counted in one int for whole pairing ????
    } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      Serial.println("ESPNOW Not Init");
      initESPNow();
      if(noOfAttempts < 8){
        attempToPair();
      }
    } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Add Peer - Invalid Argument");
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
      Serial.println("Peer list full");
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("Out of memory"); 
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
      Serial.println("Peer Exists");
      noOfAttempts++;
    } else {
      Serial.println("Not sure what happened");
      noOfAttempts++;
      if(noOfAttempts < 8){
        attempToPair();
      }
    }
    delay(100);
  }
}

// send 190 towards unit, index refers to index of unit in untypedPeers
void sendDataToGetDeviceInfo(int index){
  Serial.println("CU | sendDataToGetDeviceInfo");
  uint8_t data = 190;
  esp_err_t result = esp_now_send(untypedPeers[index].peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
  Serial.print("Send Status: ");
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

// remove peer (unpair)
void deletePeer(esp_now_peer_info_t toDelete) {
  esp_err_t delStatus = esp_now_del_peer(toDelete.peer_addr);
  Serial.print("Slave Delete Status: ");
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
  switch(type){
    case SECURITY:{
      dataToBeSent = security.getDataToBeSend();
      break;
    }
    case WATER:{
      dataToBeSent = water.getDataToBeSend();
      break;
    }
    case WHEELS:{
      dataToBeSent = wheels.getDataToBeSend();
      break;
    }
    case HEATING:{
      dataToBeSent = heating.getDataToBeSend();
      break;
    }
    case POWER:{
      dataToBeSent = power.getDataToBeSend();
      break;
    }
    default:{
      dataToBeSent = 0;
      break;  
    }
  }
  esp_err_t result = esp_now_send(getEspInfoForType(type).peer_addr, &dataToBeSent, sizeof(dataToBeSent));
  Serial.print("Send Status: ");
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
  if(status != ESP_NOW_SEND_SUCCESS && *mac_addr == *peerToBePairedWith.peer_addr && noOfAttempts < 10){ // try until data is send successfully
    noOfAttempts++;
    sendDataToGetDeviceInfo(getIndexOfUntyped(mac_addr));
  }else if(status == ESP_NOW_SEND_SUCCESS && *mac_addr == *peerToBePairedWith.peer_addr ){
    noOfAttempts = 0;
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// callback for when data is recived from sensor unit
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  
  Serial.println();
  Serial.print("CU | onDataRecv  | Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("CU | onDataRecv  | Last Packet Recv Data: "); Serial.println(*data);
  Serial.print("CU | onDataRecv  | Last Packet Recv Data: "); Serial.println(data_len);
  
  // check if it wont make trouble bool messageWasDiscarded
  bool wasUnitAdded = false;
  for(int i = 0; i < 20; i++){
    //printAddress(untypedPeers[i].peer_addr); Serial.print(" and "); Serial.println(macStr);
    if(*untypedPeers[i].peer_addr == *mac_addr){ // does it really checks
      if((*data-100) > 0 && (*data-100) <= slaveTypesNumber){
        delay(10);
        addNewSlaveToArray(i, *data-100);
        untypedPeers[i] = emptyInfo;
        wasUnitAdded = true;
        break;
      }else{ // we expect right input on first try
        deletePeer(untypedPeers[i]);
        untypedPeers[i] = emptyInfo;
        break;
      }
    }
  }
    // millisOfLastDataRecv
  if(!wasUnitAdded){
    Serial.println("New Data for unit");
    bool validMessage = true;
    switch(getSlaveTypeForMAC(mac_addr)){
      case SECURITY:
        Serial.println("Sendind data to security");
        if(security.updateYourData(data)) security.updateLastTimeRecived();
        else validMessage = false;
        break;
      case WATER:
        Serial.println("Sendind data to water");
        if(water.updateYourData(data)) water.updateLastTimeRecived();
        else validMessage = false;
        break;
      case WHEELS:
        Serial.println("Sendind data to wheels");
        if(wheels.updateYourData(data)) wheels.updateLastTimeRecived();
        else validMessage = false;
        break;
      case HEATING:
        Serial.println("Sendind data to heating");
        if(heating.updateYourData(data)) heating.updateLastTimeRecived();
        else validMessage = false;
        break;
      case POWER:
        Serial.println("Sendind data to power");
        if(power.updateYourData(data)) power.updateLastTimeRecived();
        else validMessage = false;
        break;
      default:
        Serial.println("Not right mac, getSlaveForMac retruned EMPTY");
        validMessage = false;
        break;
    }
      if(validMessage){
        // doesnt matter if millis doesnt correspond it wont make a diffrence;
        millisOfLastDataRecv = millis();  
      }else{
        Serial.println("CU | onDataRecv | Data was invalid");  
    }
  }
  
  Serial.println();
}

// remove unit that is fully set up
void removeUnit(SlaveTypes type){
 deletePeer(getEspInfoForType(type));
 int index = getIndexInSlaveTypes(type);
 slaveTypes[index] = EMPTY;
 espInfo[index] = emptyInfo;
}

// removes unactive units that didnt send any data (right data) in last 4 minutes
void removeUnactiveUnits(){
  // checks if millis was reseted
  // if thats case all lastTimeRecived will be set to new value
  if(millis() < millisOfLastDataRecv){
      millisOfLastDataRecv = millis();
      security.updateLastTimeRecived();
      water.updateLastTimeRecived();
      wheels.updateLastTimeRecived();
      heating.updateLastTimeRecived();
      power.updateLastTimeRecived(); 
  }else{
     // delets only if connectionWasEstablished (since lastTimeRecived can be set, thanks to if statem just above, without recivining any data)
    if(millis() - security.getLastTimeRecived() > 240000 && security.getEstablishedConnection()){
      removeUnit(SECURITY);
      security.setEstablishedConnection(false);
    }
    if(millis() - water.getLastTimeRecived() > 240000 && water.getEstablishedConnection()){
      removeUnit(WATER);
      water.setEstablishedConnection(false);
    }
    if(millis() - wheels.getLastTimeRecived() > 240000 && wheels.getEstablishedConnection()){
      removeUnit(WHEELS);
      wheels.setEstablishedConnection(false);
    }
    if(millis() - heating.getLastTimeRecived() > 240000 && heating.getEstablishedConnection()){
      removeUnit(HEATING);
      heating.setEstablishedConnection(false);
    }
    if(millis() - power.getLastTimeRecived() > 240000 && power.getEstablishedConnection()){
      removeUnit(POWER);
      power.setEstablishedConnection(false);
    }
  }
}

// pings each unit with number 88, units that way known that central is pressent
// they do not operate on callback when data is send succesfully
// for now ignore errors
void pingEachSesnorUnit(){
  uint8_t data = 88;
  if(security.getEstablishedConnection()){
    esp_now_send(getEspInfoForType(SECURITY).peer_addr, &data, sizeof(data));
  }
  if(water.getEstablishedConnection()){
    esp_now_send(getEspInfoForType(WATER).peer_addr, &data, sizeof(data));
  }
  if(wheels.getEstablishedConnection()){
    esp_now_send(getEspInfoForType(WHEELS).peer_addr, &data, sizeof(data));
  }
  if(heating.getEstablishedConnection()){
    esp_now_send(getEspInfoForType(HEATING).peer_addr, &data, sizeof(data));
  }
  if(power.getEstablishedConnection()){
    esp_now_send(getEspInfoForType(POWER).peer_addr, &data, sizeof(data));
  }
}

// Maybe could be moved to its own class

// updates time via NTP cilent
void updateTime(){
  byte  tries = 0; // while with timeClient.update() can result in infinite loop (some internal problem of library), so just kill it after few tries
  int   triesTime = millis();
  Serial.println("TIME | UPDATING");
  //timeClient.setTimeOffset(setOffSetForSummerTime());
  while(!timeClient.update() && tries < 5 && millis() - triesTime < 5000) {
    if(millis() - triesTime < 0){
      triesTime = millis();
    }
    timeClient.forceUpdate();
    Serial.println("TIME | UPDATED");
    tries++;
  } 
  // get unix time and sets it into Time.h for timekeeping
  setTime(timeClient.getEpochTime());
  //formattedTime = timeClient.getFormattedTime();
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.println(second());
}

// displys time on nextion
void displayTime(){
  Serial.println("Dispaling time");
  String command;
  // in / is mode with zeroes
  startEndNextionCommand();
  //command = hour() < 10 ? "textHours.txt=\"0"+String(hour())+"\"" : "textHours.txt=\""+String(hour())+"\"";
  command = "textHours.txt=\""+String(hour())+"\"";
  Serial2.print(command);
  startEndNextionCommand();
  //command = minute() < 10 ? "textAccuracy.txt=\"0"+String(minute())+"\"" : "textAccuracy.txt=\""+String(minute())+"\"";
  command = "textMinutes.txt=\""+String(minute())+"\"";
  Serial2.print(command);
  startEndNextionCommand();
  //command = day() < 10 ? "textAccuracy.txt=\"0"+String(day())+"\"" : "textAccuracy.txt=\""+String(day())+"\"";
  command = "textDay.txt=\""+String(day())+"\"";
  Serial2.print(command);
  startEndNextionCommand();
  //command = month() < 10 ? "textAccuracy.txt=\"0"+String(month())+"\"" : "textAccuracy.txt=\""+String(month())+"\"";
  command = "textMonth.txt=\""+String(month())+"\"";
  Serial2.print(command);
  startEndNextionCommand(); 
}


void setup(){
  M5.begin(true, false, true);
  Serial.begin(115200);
  //ss.begin(9600);
  Serial2.begin(9600, SERIAL_8N1,16,17);
  //Serial2.print("baud=115200");
  //startEndNextionCommand();
  //Serial2.end();  // End the serial comunication of baud=9600
  //Serial2.begin(115200, SERIAL_8N1,16,17);  // Start serial comunication at baud=115200
  while (!Serial);
//  WiFi.onEvent(WiFiEvent);
//  ETH.begin();
  WiFi.mode(WIFI_STA); // must have for telnet to work
  // This is the mac address of the Master in Station Mode
  Serial.print("CU | STA MAC: "); Serial.println(WiFi.macAddress());

  Serial.println("CU | SETUP | NETWORKING");
  // Init ESPNow with a fallback logic
  initESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  SPI.begin(SCK, MISO, MOSI, -1);
  delay(1000);
  Ethernet.init(CS);
  delay(1000); 
  Ethernet.begin(mac, ip, dnsss, gw , nm);
  Udp.begin(8888);
  
  Serial.println("CU | SETUP | ESP32");
  for(int i = 0; i< 20; i++){
    for(int j = 0; i <6; i++){
        untypedPeers[i].peer_addr[j] = emptyInfo.peer_addr[j];
      }
  }

  for(int i = 0; i < slaveTypesNumber; i++){
    slaveTypes[i] = EMPTY;
  }
  
  Serial.println("CU | SETUP | PREPARED ARRAYS");
  /*
  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started");
  */
  
  timeClient.begin();
  //updateTime();
  
  //weather.update();
  Serial.println("CU | SETUP | FINISHED");
}

int interationCounter = 0;
void loop(){
  
  delay(1000);
  // TODO: update only some iterations
  
  if(interationCounter == 0){
    Serial.println("CU | LOOP | COUNTER HIT");
    updateTime(); 
    //delay(400);
    if(security.getIsPositionKnown()) weather.setNewPosition(security.getLatitude(), security.getLongitude());
    //weather.update();
    //weather.updateDataOnNextion(hour());
    interationCounter = 10;    
    pingEachSesnorUnit();
  }

  displayTime();
  
  security.updateDataOnNextion();
  water.updateDataOnNextion();
  
  ScanForSlave();
  
  //sendData(WATER);
  //removeUnactiveUnits();
  interationCounter--;
}
