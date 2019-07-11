/*
    This sketch shows the Ethernet event usage

*/

#include <ETH.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>
#include <list> 
// Global copy of slave


#define CHANNEL_MASTER 3
#define CHANNEL_SLAVE 1
#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0

WebServer server(80);

enum SlaveTypes{
  SECURITY,WATER,WHEELS,HEATING,POWER
};
const int slaveTypesNumber = 5;

// two arrays, index, will be same, access only with methods, size is equal to number of enums in SlaveTypes
SlaveTypes slaveTypes[slaveTypesNumber];
esp_now_peer_info_t espInfo[slaveTypesNumber];

esp_now_peer_info_t getEspInfoForType(SlaveTypes type){
  for(int i; i < sizeof(slaveTypes); i++){
    if(type = slaveTypes[i]){
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

boolean addNewSlaveToArray(uint8_t type){
  for(int i = 0; i < slaveTypesNumber; i++){
    if(slaveTypes[i] == 0){ // is this correct????
      
      
    }  
  }
}

esp_now_peer_info_t peerToBePairedWith;

// IMPLEMENT
byte noOfAttempts = 0; // how many times have we tried to establish and verify connection


struct TEST_STRUCT{
  float f1;  
  int i1;
  int i2;
  short signature;
};
TEST_STRUCT test;

static bool eth_connected = false;
const short callsign = 999;

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
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


void WiFiEvent(WiFiEvent_t event)
{
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

void testClient(const char * host, uint16_t port)
{
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


// Scan for slaves in AP mode, when found will pair and get special 
// message (will be from mac that we are expecting to answer) with sensor type
// after reciving new slave will be added, others slaves can comunicate with central meanwhile
// others can send data

// if we wont get answer in couple of second program will skip it.
// that will be done with while boolean cycle, boolean will change if central is contacted
// or some time (difference of milis()) passes. Boolean is global.

// will have info about slave to which is currently connecting, 
// after we know slave type, then it will be added to arrays

// message will be number which will corespond with index in emums
// format 10X where X is index of SlaveType in enum

// make sure channels are same
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

      //if (PRINTSCANRESULTS) {
      //  Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
      //}
      //delay(10);
      
      // Check if the current device starts with `Slave`
      if (SSID.indexOf("ESPNOW") == 0) {
        // SSID of interest
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          // we have mac, now we create temp slave and attemp to pair
          for (int ii = 0; ii < 6; ++ii ) {
            peerToBePairedWith.peer_addr[ii] = (uint8_t) mac[ii];
          }
          peerToBePairedWith.channel = CHANNEL_MASTER; // pick a channel
          peerToBePairedWith.encrypt = 0; 
          // attempts to pair to found slave

          // TODO: stop function here;
          attempToPair(); 
          // can be moved to callbacks after sending message here needs to be boolean to stop
          // moving to next found STA
        }
      }
    }
  }
  // check is slaves were found + print which
  // Serial.println("No Slave Found, trying again.");

  // clean up ram
  WiFi.scanDelete();
}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
bool attempToPair() {
  Serial.print("Processing: ");
  for (int ii = 0; ii < 6; ++ii ) {
    Serial.print((uint8_t) peerToBePairedWith.peer_addr[ii], HEX);
    if (ii != 5) Serial.print(":");
  }
  Serial.print(" Status: ");
  
  // check if the peer exists
  bool exists = esp_now_is_peer_exist(peerToBePairedWith.peer_addr);
  if (exists) {
    // Slave already paired.
    Serial.println("Already Paired");
  } else {
    // Slave not paired, attempt pair 
    esp_err_t addStatus = esp_now_add_peer(&peerToBePairedWith);
    if (addStatus == ESP_OK) {           
      // Pair success here message will be send and changed boolean/ metod for recv data
      // also need to change sendCallback - that will check mac if failed than will data be sent again
      // recv data also will check mac address - than special behaveior
      // NEED TO CLEAN peerToBePairedWith !!!!!
      // check form of mac (What type)
            
      Serial.println("Pair success"); // attempt to pair ends
      sendDataToGetDeviceInfo();
      // TODO: after couple of retries attemToPair should exit maybe 3, than delete peer
      // Should failiurese be counted in one int for whole pairing ????
    } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      Serial.println("ESPNOW Not Init");
      InitESPNow();
      attempToPair();
    } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Add Peer - Invalid Argument"); 
    } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
      Serial.println("Peer list full"); // wont be necessary
    } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("Out of memory"); // TODO: fix
    } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
      Serial.println("Peer Exists");  // Imposible case in this case due to higher if
    } else {
      Serial.println("Not sure what happened");
      attempToPair();
    }
    delay(100);
  }
}
// called after pairing with new slave and in case of failiures
// if 
void sendDataToGetDeviceInfo(){
  Serial.println();
  uint8_t data = 193;
  esp_err_t result = esp_now_send(peerToBePairedWith.peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
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

uint8_t pos = 0;
// send data 
// will have enum in argument will affect what data is send and to whom
/*
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

//noOfAttempts


// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status != ESP_NOW_SEND_SUCCESS && *mac_addr == *peerToBePairedWith.peer_addr){ // try until data is send successfully
      sendDataToGetDeviceInfo();
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if(*mac_addr == *peerToBePairedWith.peer_addr){
    if((*data-100) >= 0 && (*data-100) < slaveTypesNumber){
      // calls addNewSlaveToArray
      addNewSlaveToArray(*data);
    }else{
      // we get something that we didn't asked for
      // delete Peer
      }
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println();
  Serial.print("\t\tLast Packet Recv from: "); Serial.println(macStr);
  Serial.print("\t\tLast Packet Recv Data: "); Serial.println(*data);
  Serial.println("");
}

void setup()
{
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  WiFi.mode(WIFI_MODE_APSTA);
  Serial.println("ESPNow/Basic/Master Example");
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  delay(2000);

  // 128 166 240 71 207 4 0 0 137 2 0 0 119 5 0 0
  // 128 166 240 71 207 4 0 0 137 2 0 0 105 4 0 0
  
  test.f1 =         123213;
  test.i1 =         1231;
  test.i2 =         649;
  test.signature =  1129;

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  
}


void loop(){
  //if (eth_connected) {
  //  testClient("google.com", 80);
  //  server.handleClient();
  //}



  // In the loop we scan for slave
  ScanForSlave();
  // If Slave is found, it would be populate in `slave` variable
  // We will check if `slave` is defined and then we proceed further
  if (slaveTypes[0]==0) { // check if slave channel is defined
    // ready to send data
  }
  else {
    // No slave found to process
  }  
}
