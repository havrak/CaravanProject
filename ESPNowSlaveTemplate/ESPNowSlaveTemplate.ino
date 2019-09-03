// NOTE: 
// Send all data after reciving new configuration
// this call contain some constants you can find those for your specific hardware via WaterTest

#include <esp_now.h>
#include <WiFi.h>
#include <EEPROM.h>

// TODO: check size, check if EEPROM size is in bytes/bits
#define EEPROM_SIZE 80
#define CHANNEL 1

esp_now_peer_info_t master;
bool sendedIMyTypeToCentral = false;
uint8_t master_addr;

bool eepromOn = false;

int lastTimeDataRecived = 0;



byte validityOfData;
// send and recive structure, for ease of access it it used only for reciving and sending data, ESP32 has plenty of room to store those data 2 times 
struct SendRecvDataStruct{
  // 0 - data is valid, system hit top and counters were reseted, 1 - data could be valid, but was loaded from EEPROM, 2 - EEPROM was empty thus we can't guess state of tank, 3 - tank wasnt filled to its full capacity
  byte validityOfData;
};

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

// config AP SSID
void configDeviceAP() {
  const char *SSID = "ESPNOW_1";
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}

boolean checkIfTwoAddressesAreSame(const uint8_t addr1[],const uint8_t addr2[]){
  if(sizeof(addr1) != sizeof(addr2)){
    Serial.println("diffrent size");
    return false;
  }
  for(int i = 0; i < sizeof(addr1); i++){
    if(addr1[i] != addr2[i]) return false;
  }
  return true;
}

// EEPROM data with theirs respective adresses in parentheses: byte(0) -- will be one if something was written, literRemaiding(1,4), pulseCounter(5,8), relayOpen(9)
void storeDataInEEPROM(){
  if(EEPROM.read(0) == 0){
      EEPROM.write(0,1);
  }
  // store data here
  EEPROM.commit();
}

void loadDataFromEEPROM(){
  if(EEPROM.read(0) == 0){
    validityOfData = 2;
  }else{
    // load data here
  }
}
int counter = 0;

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status != ESP_NOW_SEND_SUCCESS && !sendedIMyTypeToCentral && counter <10){ // try until data is send successfully
    Serial.println("Sending info failed");
    delay(100);
    sendConfirmation();
    counter++;
  }else{
    sendedIMyTypeToCentral = true;
    counter=0;
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  
}

void sendData() {
  Serial.println("Sending data");
  SendRecvDataStruct data;
  uint8_t dataToBeSend[sizeof(data)];
  memcpy(dataToBeSend, &data, sizeof(data));
 
  esp_err_t result = esp_now_send(master.peer_addr, dataToBeSend, sizeof(dataToBeSend));
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
    esp_now_add_peer(&master);
  } else {
    Serial.println("Not sure what happened");
  }
}

// callback when data is recv from Master
// check if mac_addr matches master, others inpouts ignore
// also sets up master, in case master asks again than address will be set up again for new master
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(*mac_addr);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);

  // add protection
  if (*data == (uint8_t) 190){
    if((sendedIMyTypeToCentral && checkIfTwoAddressesAreSame(master.peer_addr, mac_addr)) || !sendedIMyTypeToCentral){ // prevent hijack of unit (very simple way)
      Serial.print("Got masters addr");
      master.channel = 1;
      master.encrypt = 0;
      counter=0;
      memcpy(master.peer_addr, mac_addr, sizeof(master.peer_addr)); // size if diffrent
      sendedIMyTypeToCentral = false;
      esp_err_t addStatus = esp_now_add_peer(&master);
      sendConfirmation();
      memcpy(&master_addr,&mac_addr,sizeof(mac_addr));
      lastTimeDataRecived = 0;
    }
  }
  if(*mac_addr == master_addr){
      if(*data == (uint8_t) 88){ // we recived ping from main station
        lastTimeDataRecived = millis();
      }else{
        lastTimeDataRecived = millis();
        // here goes code to copy to internal variables
        
     }
  }
}

// 
void sendConfirmation(){
  Serial.print("sending confirmation");
  uint8_t data = 000; // edit unit code here
  esp_err_t result = esp_now_send(master.peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
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
  } else {
    Serial.println("Not sure what happened");
  }
}

void deleteUnactiveMaster(){
  if(millis() < lastTimeDataRecived){
    lastTimeDataRecived = millis();
    
  }else if(millis() - lastTimeDataRecived > 240000){
    bool sendedIMyTypeToCentral = false;
    uint8_t master_addr = 0;  
    memcpy(&master, 0 , sizeof(master));
    
    esp_err_t delStatus = esp_now_del_peer(master.peer_addr);
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

}


void setup() {
  Serial.begin(115200);
  
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP_STA);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  initESPNow();
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  if (!EEPROM.begin(EEPROM_SIZE)){
    validityOfData = 2; // we cant read from EEPROM
    Serial.println("failed to initialise EEPROM");
  }else{ // if EERPROM is empty validity is set to 2, if data is loaded validity is set to 1
    loadDataFromEEPROM();
  }
}

int val;
byte count = 0;

void loop() {
  // delete master if he is unactive
  deleteUnactiveMaster();
  // check for unactive

  delay(500);
  if(count % 6 == 0){
    sendData();
    count = 0;
  }
  count++; 
  
}
