/*
    Code for central unit, runs on OLIMEX EVB
*/

#include <ETH.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Olimex.h"
#include "Water.h"
#include "Heating.h"
#include "Power.h"
#include "Wheels.h"
#include "Security.h"
#include "Water.h"
#include "Connection.h"

#define CHANNEL 1
#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0
#define EEPROM_SIZE 1

#define stringify( name ) # name

WiFiUDP Udp;
NTPClient timeClient(Udp);
WebServer server(80);

String formattedDate;
String dayStamp;
String timeStamp;
int timeOffset = 7200;


const int slaveTypesNumber = 6;
enum SlaveTypes{
  SECURITY,WATER,WHEELS,HEATING,POWER,TEMP
};
// TODO: find out how callbacks are handled
// array of boolean prevents updating info in water.h by callbacks if there is new configuration being recived from olimex
bool updateLocks[slaveTypesNumber];

Olimex olimex;

Power power;
Security security;
Water water;
Wheels wheels;
Heating heating;
Connection connection;

// remove after testing
const char* slaveTypesNames[] =
  {
  stringify(SECURITY),
  stringify(WATER),
  stringify(WHEELS),
  stringify(HEATING),
  stringify(POWER),
};


// two arrays, index, will be same, access only with methods, size is equal to number of enums in SlaveTypes
SlaveTypes slaveTypes[slaveTypesNumber];
esp_now_peer_info_t espInfo[slaveTypesNumber];

esp_now_peer_info_t untypedPeers[20]; // max nuber of untyped peers

esp_now_peer_info_t peerToBePairedWith; // wont be neccesseary here

esp_now_peer_info_t emptyInfo;

int getIndexOfUntyped(const uint8_t *mac_addr){
  for(int i; i< sizeof(untypedPeers); i++){
    if(*mac_addr == *untypedPeers[i].peer_addr){
        return i;
      }
  }
}

esp_now_peer_info_t getEspInfoForType(SlaveTypes type){
  for(int i; i < sizeof(slaveTypes); i++){
    if(type == slaveTypes[i]){
      return espInfo[i];
    }
  }
}
SlaveTypes getSlaveTypeForMAC(const uint8_t *mac_addr){
  for(int i; i< sizeof(slaveTypes); i++){
    if(*mac_addr == *espInfo[i].peer_addr){
        return slaveTypes[i];
      }
  }
}

boolean doesntContainMac(uint8_t addr[]){
  for(int i = 0; i < slaveTypesNumber; i++){
    printAddress(espInfo[i].peer_addr );Serial.print("    "); printAddress(addr); Serial.println();
    if(checkIfTwoAddressesAreSame(addr, espInfo[i].peer_addr)){
        Serial.println("\t\tMac address is already stored");
        return false;
    }
  }
  return true;
}

boolean addNewSlaveToArray(int index, uint8_t type){
  Serial.print("Index is untyped: "); Serial.println(index);
  for(int i = 0; i < slaveTypesNumber; i++){
    if(slaveTypes[i] == 0){
      switch(type){
        case 1:
          slaveTypes[i] = SECURITY;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith)); // copies data to array
          Serial.println("Added SECURITY ESP32");
          i = slaveTypesNumber; // just exit for loop
          break;
        case 2:
          slaveTypes[i] = WATER;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("Added WATER ESP32");
          i = slaveTypesNumber;
          break;
        case 3:
          slaveTypes[i] = WHEELS;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("Added WHEELS ESP32");
          i = slaveTypesNumber;
          break;
        case 4:
          slaveTypes[i] = HEATING;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("Added HEATING ESP32");
          i = slaveTypesNumber;
          break;
        case 5:
          slaveTypes[i] = POWER;
          memcpy (&espInfo[i], &untypedPeers[index], sizeof(peerToBePairedWith));
          Serial.println("Added POWER ESP32");
          i = slaveTypesNumber;
          break;
      }
    }
  } // resets address so we wont in case for ifs in callbacks
  printRouteTable();
}

void printRouteTable(){
  Serial.println();
  for(int i=0; i < slaveTypesNumber; i++){
    Serial.print(slaveTypesNames[i]);
    Serial.print(" : ");
    printAddress(espInfo[i].peer_addr);
    Serial.print("   ");
  }
  Serial.println();
}

void printAddress(uint8_t addr[]){
  for (int i = 0; i < 6; ++i ) {
    Serial.print((uint8_t) addr[i], HEX);
    if (i != 5) Serial.print(":");
  }
}

// IMPLEMENT
byte noOfAttempts = 0; // how many times have we tried to establish and verify connection

static bool eth_connected = false;
const short callsign = 999;

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
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

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

void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}


// add boolean to stop scaning if slots are filled
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
          peerToBePairedWith.channel = CHANNEL; // pick a channel
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
      noOfAttempts = 0;
    }
  }
  // check is slaves were found + print which
  // Serial.println("No Slave Found, trying again.");

  // clean up ram
  WiFi.scanDelete();
}



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

// Check if the slave is already paired with the master.
// If not, pair the slave with master
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
        printRouteTable();
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
      for(int i = 0; i < sizeof(untypedPeers); i++){
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
      InitESPNow();
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
// called after pairing with new slave and in case of failiures
// if
void sendDataToGetDeviceInfo(int index){
  Serial.println("Sending data to get info");
  uint8_t data = 193;
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

// remove from arrays
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

/*
uint8_t pos = 0;
// send data
// will have enum in argument will affect what data is send and to whom
void sendData() {
  pos++;
  //const uint8_t *peer_addr = .peer_addr;
  uint8_t bs[sizeof(test)];
  memcpy(bs, &test, sizeof(test));
  for(int j=0; j < sizeof(bs); j++){
    Serial.print(bs[j]); Serial.print(" ");
  }
  Serial.println();
  Serial.print("Sending No.: "); Serial.println(pos);
  //esp_err_t result = esp_now_send(peer_addr, bs, sizeof(bs));
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
*/

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status != ESP_NOW_SEND_SUCCESS && *mac_addr == *peerToBePairedWith.peer_addr && noOfAttempts < 20){ // try until data is send successfully
    noOfAttempts++;
    sendDataToGetDeviceInfo(getIndexOfUntyped(mac_addr));
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  bool wasNewUnitAdded = false;
  // check if it wont make trouble bool messageWasDiscarded
  for(int i = 0; i < 20; i++){
    printAddress(untypedPeers[i].peer_addr); Serial.print(" and "); Serial.println(macStr);
    if(*untypedPeers[i].peer_addr == *mac_addr){ // does it really checks
      if((*data-100) > 0 && (*data-100) <= slaveTypesNumber){
        delay(10);
        addNewSlaveToArray(i, *data-100);
        untypedPeers[i] = emptyInfo;
        wasNewUnitAdded = true;
      }else{ // we expect right input on first try
        deletePeer(untypedPeers[i]);
        untypedPeers[1] = emptyInfo;
      }
    }
    if(!wasNewUnitAdded){
      switch(getSlaveTypeForMAC(mac_addr)){
        case SECURITY:
          heating.updateYourData(*data);
          break;
        case WATER:
          water.updateYourData(*data);
          break;
        case WHEELS:
          wheels.updateYourData(*data);
          break;
        case HEATING:
          heating.updateYourData(*data);
          break;
        case POWER:
          power.updateYourData(*data);
          break;
      }
    }

  }

  Serial.println();
  Serial.print("\t\tLast Packet Recv from: "); Serial.println(macStr);
  Serial.print("\t\tLast Packet Recv Data: "); Serial.println(*data);
  Serial.println();
}

void setup(){
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  WiFi.mode(WIFI_STA);
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  delay(2000);
  // summer / winter time or manual adjustment
  
  //

  for(int i = 0; i< 20; i++){
    for(int j = 0; i <6; i++){
        untypedPeers[i].peer_addr[j] = emptyInfo.peer_addr[j];
      }
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  //EEPROM.write(0, 64);

  //EEPROM.commit();

  //int temp = EEPROM.read(0);
  //Serial.print("On 0 is: "); Serial.println(temp);
  //Serial.print("On 0 is: "); Serial.println(EEPROM.read(0));
  timeClient.begin();
  timeClient.setTimeOffset(timeOffset);
  timeClient.forceUpdate();
  server.begin();

}


void loop(){

  while(!timeClient.update()) {
    timeClient.setTimeOffset(timeOffset);
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // will be send to olimex
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
  //delay(1000);

  //if (eth_connected) {
  //  testClient("duckduckgo.com", 80);
  //  server.handleClient();
  //}

  // In the loop we scan for slave
  ScanForSlave();
  // If Slave is found, it would be populate in `slave` variable
  // We will check if `slave` is defined and then we proceed further
  if (slaveTypes[0]==0) { // check if slave channel is defined
    uint8_t data = 6;
    //esp_err_t result = esp_now_send(espInfo[0].peer_addr, &data, sizeof(data));
    esp_err_t result = esp_now_send(getEspInfoForType(SECURITY).peer_addr, &data, sizeof(data));
  }
  else {
    // No slave found to process
  }
}
