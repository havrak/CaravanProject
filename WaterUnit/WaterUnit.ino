// NOTE:
// Send all data after reciving new configuration
#include <esp_now.h>
#include <WiFi.h>
#include <EEPROM.h>

#define EEPROM_SIZE 64
#define CHANNEL 1

esp_now_peer_info_t master;
boolean sendedIMyTypeToCentral = false;
uint8_t master_addr;

bool connectionToWaterSource;
float litersRemaining;
float temperature;
bool topTankSensor;
bool bottomTankSensor;
int pulseCounter;

// send and recive 
struct SendRevData{
      bool connectionToWaterSource;
      float litersRemaining;
      float temperature;
};

// Init ESP Now with fallback
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

void setup() {
  Serial.begin(115200);
  Serial.println("ESPNow/Basic/Slave Example");
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP_STA);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  if (!EEPROM.begin(EEPROM_SIZE)){
    Serial.println("failed to initialise EEPROM");
  }
  }

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status != ESP_NOW_SEND_SUCCESS && sendedIMyTypeToCentral == false){ // try until data is send successfully
    Serial.println("Sending info failed");
    sendConfirmation();
  }else{
    sendedIMyTypeToCentral = true;
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

int pos=0;
void sendData() {
   // no need to send all data

  SendRevDataStruct data;
  data.connectionToWaterSource = ;
  data.litersRemaining = ;
  data.temperature = ;
  
  uint8_t dataToBeSend[sizeof(SendRecvData)];
  memcpy(dataToBeSend, &data, sizeof(data));
  
  esp_err_t result = esp_now_send(peer_addr, bs, sizeof(bs));
 
  esp_err_t result = esp_now_send(master.peer_addr, &data, sizeof(data));
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

// callback when data is recv from Master
// check if mac_addr matches master, others inpouts ignore
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(*mac_addr);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);


  if (*data == (uint8_t) 193){
      master.channel = 1;
      master.encrypt = 0;
      memcpy(master.peer_addr, mac_addr, sizeof(mac_addr)+8); // size if diffrent

      esp_err_t addStatus = esp_now_add_peer(&master);
      sendConfirmation();
      memcpy(&master_addr,&mac_addr,sizeof(mac_addr));
  }
  if(*mac_addr == master_addr){
  	  // procede
  }
}
void sendConfirmation(){
  Serial.println();
  uint8_t data = 101;
  esp_err_t result = esp_now_send(master.peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
  Serial.print("Send Status: ");
  if (result == ESP_OK) {
    Serial.println("Success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
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

void loop() {
  // Chill
}
