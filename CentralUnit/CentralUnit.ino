/*
    Code for central unit, runs on OLIMEX Gateway
*/

#include <ETH.h>        // for ethernet to work
#include <esp_now.h>    // for enabling ESP NOW wich is used to communicate with units
#include <WiFi.h>       // for running ethernet interface
#include <WebServer.h>  // for running webserver on Olimex
#include <EEPROM.h>     // for writing data into EEPROM (probably wont be used here)
#include <Time.h>       // for timekeeping
#include <WiFiUdp.h>    // for WifiUdp - as argument in timeCliet
#include <NTPClient.h>  // for syncing time via NTP
#include <Nextion.h>    // for getting data from nextion display
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

WiFiUDP Udp;
NTPClient timeClient(Udp);
WebServer server(80);

String formattedDate;
String dayStamp;
String timeStamp;
int timeOffset = 7200;


const int slaveTypesNumber = 5;
// need last one whole array will be inicialized with emtpy sicne you cant go back to default value of enum
enum SlaveTypes{
  SECURITY,WATER,WHEELS,HEATING,POWER,EMPTY
};

// TODO: find out how callbacks are handled
// array of boolean prevents updating info in water.h by callbacks if there is new configuration being recived from olimex
bool updateLocks[slaveTypesNumber];

//NextionObject nextionObject;
NexButton b1 = NexButton(0, 9, "b1");  // Button added
NexTouch *nex_listen_list[] = {
  &b1,  // Button added
};

void b1PushCallback(void *ptr){
  digitalWrite(13, HIGH);  // Turn ON internal LED
}  // End of press event

void b1PopCallback(void *ptr){
  digitalWrite(13, LOW);  // Turn OFF internal LED
}

Power power;
Security security;
Water water;
Wheels wheels;
Heating heating;
Connection connection();
Temperatures temperatures;
Weather weather(0,0);



// millisResetsAfter50Day that would make it impossible (for a while) to remove unactiveChips
long millisOfLastDataRecv;

// two arrays, index, will be same, access only with methods, size is equal to number of enums in SlaveTypes
SlaveTypes slaveTypes[slaveTypesNumber];
esp_now_peer_info_t espInfo[slaveTypesNumber];

esp_now_peer_info_t untypedPeers[20]; // max nuber of untyped peers

esp_now_peer_info_t peerToBePairedWith; // wont be neccesseary here

esp_now_peer_info_t emptyInfo; // empty info, for when program need to fill something with 0, mostly for my confort, of course memcpy with 0 would work to

int getIndexOfUntyped(const uint8_t *mac_addr){
  for(int i; i< (sizeof(untypedPeers)/sizeof(untypedPeers[0])); i++){
    if(*mac_addr == *untypedPeers[i].peer_addr){
        return i;
      }
  }
}
esp_now_peer_info_t getEspInfoForType(SlaveTypes type){
  for(int i; i < (sizeof(slaveTypes)/sizeof(slaveTypes[0])); i++){
    if(type == slaveTypes[i]){
      return espInfo[i];
    }
  }
}

// returns index in SlaveTypes for given type, used in case we want to remove it
int getIndexInSlaveTypes(SlaveTypes type){
  for(int i; i < (sizeof(slaveTypes)/sizeof(slaveTypes[0])); i++){
    if(type == slaveTypes[i]){
      return i;
    }
  }
}

// returns SlaveTypes that corresponds with mac_addr in argument (comparing in espInfo)
SlaveTypes getSlaveTypeForMAC(const uint8_t *mac_addr){
  for(int i; i< sizeof(slaveTypes); i++){
    if(*mac_addr == *espInfo[i].peer_addr){
        return slaveTypes[i];
      }
  }
}

// checks if we had already registred MAC in espInfo
bool doesntContainMac(uint8_t addr[]){
  for(int i = 0; i < slaveTypesNumber; i++){
    if(checkIfTwoAddressesAreSame(addr, espInfo[i].peer_addr)){
        Serial.println("Mac address is already stored");
        return false;
    }
  }
  return true;
}

// checks if two addresses are same
boolean checkIfTwoAddressesAreSame(uint8_t addr1[], uint8_t addr2[]){
  if(sizeof(addr1) != sizeof(addr2)){
    Serial.println("diffrent size");
    return false;
  }
  for(int i = 0; i < sizeof(addr1); i++){
    if(addr1[i] != addr2[i]) return false;
  }
  return true;
}

// adds new entry into slaveTypes and espInfo, used after unit send its type
boolean addNewSlaveToArray(int index, uint8_t type){
  Serial.print("Index is untyped: "); Serial.println(index);
  for(int i = 0; i < slaveTypesNumber; i++){
    if(slaveTypes[i] == EMPTY){
      switch(type){
        case 1:
          slaveTypes[i] = SECURITY;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith)); // copies data to array
          Serial.println("Added SECURITY ESP32");
          security.setEstablishedConnection(true);
          i = slaveTypesNumber; // just exit for loop
          break;
        case 2:
          slaveTypes[i] = WATER;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("Added WATER ESP32");
          water.setEstablishedConnection(true);
          i = slaveTypesNumber;
          break;
        case 3:
          slaveTypes[i] = WHEELS;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("Added WHEELS ESP32");
          wheels.setEstablishedConnection(true);
          i = slaveTypesNumber;
          break;
        case 4:
          slaveTypes[i] = HEATING;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("Added HEATING ESP32");
          heating.setEstablishedConnection(true);
          i = slaveTypesNumber;
          break;
        case 5:
          slaveTypes[i] = POWER;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("Added POWER ESP32");
          power.setEstablishedConnection(true);
          i = slaveTypesNumber;
          break;
      }
    }
  } // resets address so we wont in case for ifs in callbacks
}

// prints given mac address 
void printAddress(uint8_t addr[]){
  for (int i = 0; i < 6; ++i ) {
    Serial.print((uint8_t) addr[i], HEX);
    if (i != 5) Serial.print(":");
  }
}

// IMPLEMENT
byte noOfAttempts = 0; // how many times have we tried to establish and verify connection

static bool ethConnected = false;
const short callsign = 999;

// root of server running on Olimex
void handleRoot() {
  server.send(200, "text/plain", "hello from esp32!");
}


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
void WiFiEvent(WiFiEvent_t event){
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
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

// tests if connection is running (for debug)
void testClient(const char * host, uint16_t port){
  Serial.print("\nconnecting to ");
  Serial.println(host);

  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
  client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
  while (client.connected() && !client.available());
  while (client.available()) {
    Serial.write(client.read());
  }

  Serial.println("closing connection\n");
  client.stop();
}

// Inits ESPNow
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

// scans network, finds all ESP32 unit
// after unit is calls AttempToPair() for that unit()
void ScanForSlave() {
  int8_t scanResults = WiFi.scanNetworks();
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");

    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
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
          Serial.println("Moving to next one");
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
bool attempToPair() {
  Serial.print("Processing: ");
  for (int ii = 0; ii < 6; ++ii ) {
    Serial.print((uint8_t) peerToBePairedWith.peer_addr[ii], HEX);
    if (ii != 5) Serial.print(":");
  }
  Serial.print("Status:");

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
          Serial.println("Found punctuationEmpty");
          memcpy(&untypedPeers[i], &peerToBePairedWith, sizeof(peerToBePairedWith));
          i = sizeof(untypedPeers);
          sendDataToGetDeviceInfo(i);
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
      Serial.println("Peer list full"); // wont be necessary
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("Out of memory"); // TODO: fix
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
      Serial.println("Peer Exists");  // Imposible case in this case due to higher if
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
  Serial.println("Sending data to get info");
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
  if(status != ESP_NOW_SEND_SUCCESS && *mac_addr == *peerToBePairedWith.peer_addr && noOfAttempts < 20){ // try until data is send successfully
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
  Serial.print("\t\tLast Packet Recv from: "); Serial.println(macStr);
  Serial.print("\t\tLast Packet Recv Data: "); Serial.println(*data);
  Serial.println();
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
      }else{ // we expect right input on first try
        deletePeer(untypedPeers[i]);
        untypedPeers[i] = emptyInfo;
      }
    }
  }
    // millisOfLastDataRecv
  if(!wasUnitAdded){
    Serial.println("Valid data");
    bool validMessage = true;
    switch(getSlaveTypeForMAC(mac_addr)){
      case SECURITY:
        validMessage = heating.updateYourData(data);
        validMessage ? heating.updateLastTimeRecived() : NULL;
        break;
      case WATER:
        Serial.println("adsadsad");
        validMessage = water.updateYourData(data);
        validMessage ? heating.updateLastTimeRecived() : NULL;
        break;
      case WHEELS:
        validMessage = wheels.updateYourData(data);
        validMessage ? heating.updateLastTimeRecived() : NULL;
        break;
      case HEATING:
        validMessage = heating.updateYourData(data);
        validMessage ? heating.updateLastTimeRecived() : NULL;
        break;
      case POWER:
        validMessage = power.updateYourData(data);
        validMessage ? heating.updateLastTimeRecived() : NULL;
        break;
      default:
        Serial.print("asa");
        validMessage = false;
        break;
    }
      if(validMessage){
        // doesnt matter if millis doesnt correspond it wont make a diffrence;
        millisOfLastDataRecv = millis();  
      }else{
        Serial.print("message was invalid");  
    }
  }
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

void setup(){
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1,16,17);
  //Serial2.send("baud=115200");
  //startEndNextionCommand();
  //Serial2.end();  // End the serial comunication of baud=9600
  //Serial2.begin(115200, SERIAL_8N1,16,17);  // Start serial comunication at baud=115200

  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  WiFi.mode(WIFI_STA);
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  initESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  for(int i = 0; i< 20; i++){
    for(int j = 0; i <6; i++){
        untypedPeers[i].peer_addr[j] = emptyInfo.peer_addr[j];
      }
  }
  for(int i; i < slaveTypesNumber; i++){
    slaveTypes[i] = EMPTY;
  }

  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();

  timeClient.begin();
  timeClient.setTimeOffset(7200);
  timeClient.forceUpdate();
}



// check if mac exist -> if not no action
// check lastTimeRecived

void updateTime(){
  while(!timeClient.update()) {
   timeClient.setTimeOffset(timeOffset);
   timeClient.forceUpdate();
  }
  // get unix time and sets it into Time.h for timekeeping
  setTime(timeClient.getEpochTime());
}
void startEndNextionCommand(){
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

void displayTime(){
  String command;
  // in "" is mode with zeroes
  startEndNextionCommand();
  //command = hour() < 10 ? "textHours.txt=\"0"+String(hour())+"\"" : "textHours.txt=\""+String(hour())+"\"";
  command = "textHours.txt=\""+String(hour())+"\"";
  Serial2.print(command);
  startEndNextionCommand();
  //command = minute() < 10 ? "textAccuracy.txt=\"0"+String(minute())+"\"" : "textAccuracy.txt=\""+String(minute())+"\"";
  command = "textAccuracy.txt=\""+String(minute())+"\"";
  Serial2.print(command);
  startEndNextionCommand();
  //command = day() < 10 ? "textAccuracy.txt=\"0"+String(day())+"\"" : "textAccuracy.txt=\""+String(day())+"\"";
  command = "textAccuracy.txt=\""+String(day())+"\"";
  Serial2.print(command);
  startEndNextionCommand();
  //command = month() < 10 ? "textAccuracy.txt=\"0"+String(month())+"\"" : "textAccuracy.txt=\""+String(month())+"\"";
  command = "textAccuracy.txt=\""+String(month())+"\"";
  Serial2.print(command);
  startEndNextionCommand(); 
}

int interationCounter;
void loop(){
  //delay(1000);
  // TODO: update only some iterations
  //if(interationCounter % 1000 == 0 ){
  //  updateTime(); 
  //}
  
  //displayTime();
  ScanForSlave();
  
  //if (ethConnected) {
  //  testClient("duckduckgo.com", 80);
  //  server.handleClient();
  //}
  water.updateDataOnNextion();
  //sendData(WATER);
  removeUnactiveUnits();
  interationCounter++;
}
