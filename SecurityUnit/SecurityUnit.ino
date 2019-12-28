// NOTE: 
// Send all data after reciving new configuration
// this call contain some constants you can find those for your specific hardware via WaterTest
#include <M5Stack.h>
#include "Free_Fonts.h"
#include <esp_now.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <TinyGPS++.h>
// TODO: check size, check if EEPROM size is in bytes/bits
#define EEPROM_SIZE 256
#define CHANNEL 1


esp_now_peer_info_t central;
esp_now_peer_info_t potentialCentral;

bool sendedIMyTypeToCentral = false;
bool didCentralSendConfirmation = false;
uint8_t central_addr;
byte noOfAttempts = 0;

TinyGPSPlus gps;

bool eepromOn = false;
int lastTimeDataRecived = 0;
uint32_t numberOfSatellites;
double latitude;
double longitude;
int32_t  hdop ;
  
struct SendRecvDataStruct{
  uint32_t numberOfSatellites;
  double latitude;
  double longitude;
  int32_t  hdop;
};

boolean checkIfTwoAddressesAreSame(const uint8_t *addr1,const uint8_t *addr2){
  if(sizeof(addr1) != sizeof(addr2)){
    Serial.println("diffrent size");
    return false;
  }
  for(int i = 0; i < (sizeof(addr1)/sizeof(addr1[0])); i++){
    if(addr1[i] != addr2[i]) return false;
  }
  return true;
}

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

// counting speed is diffrent than what it should be
// what to set when water is refiling
void storeDataInEEPROM(){

}

void loadDataFromEEPROM(){

}

int counter = 0;
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status != ESP_NOW_SEND_SUCCESS && !sendedIMyTypeToCentral && counter <10){ // try until data is send successfully
    Serial.println("Sending info failed");
    delay(100);
    sendMyTypeToCentral();
    counter++;
  }else if (status == ESP_NOW_SEND_SUCCESS && !sendedIMyTypeToCentral){
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
  Serial.println();
  Serial.println("Sending data");
  SendRecvDataStruct data;
  
  data.hdop = hdop;
  data.numberOfSatellites = numberOfSatellites;
  data.latitude = latitude;
  data.longitude = longitude;
  
  uint8_t dataToBeSend[sizeof(data)];
  memcpy(dataToBeSend, &data, sizeof(data));
  Serial.print("Size of packet will be:  "); Serial.println(sizeof(data));
  Serial.print("Size of dataToBeSend is: "); Serial.println(sizeof(dataToBeSend));
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
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(*mac_addr);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);

  // add protection
  
  if (*data == (uint8_t) 92){
    if(checkIfTwoAddressesAreSame(potentialCentral.peer_addr, mac_addr)){ // prevent hijack of unit (very simple way)
      Serial.print("SU | onDataRecv | Set up central");
      central.channel = 1;
      central.encrypt = 0;
      counter=0;
      memcpy(central.peer_addr, mac_addr, sizeof(central.peer_addr)); // size if diffrent
      sendedIMyTypeToCentral = false;
      esp_err_t addStatus = esp_now_add_peer(&central);
      memcpy(&central_addr,&mac_addr,sizeof(mac_addr));
      lastTimeDataRecived = 0;    
    }else if (checkIfTwoAddressesAreSame(central.peer_addr, mac_addr)){ } else {
      Serial.println("SU | onDataRecv | got 88 from unit I wasn't expecting");  
    }
    //}
  }
  
  if(*mac_addr == central_addr){
      lastTimeDataRecived = millis();
      if(*data != (uint8_t) 88){ // check if message is not just a ping
        // NEW CONFIGURATION IS PROCESSED HERE
     }
  }
}

unsigned long startTime = -1; // we give 10 seconds to central to respod, then we will try diffrent cenral with same name


void ScanForCentral() {
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
      if (SSID.indexOf("CARAVAN_CENTRAL_UNIT") == 0) {
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          esp_now_peer_info_t temp;
          for (int ii = 0; ii < 6; ++ii ) {
            temp.peer_addr[ii] = (uint8_t) mac[ii];
          }
          if(startTime == -1 || (millis() - startTime > 10 000 && checkIfTwoAddressesAreSame(potentialCentral.peer_addr, temp.peer_addr))  ){
            // we have mac, now we create temp slave and attemp to pair
            for (int ii = 0; ii < 6; ++ii ) {
              potentialCentral.peer_addr[ii] = (uint8_t) mac[ii];
            }
          
            potentialCentral.channel = 1; // pick a channel
            potentialCentral.encrypt = 0;
            // attempts to pair to found slave
          
            attempToPair();
            startTime = millis();
          }
        }
      }
      sendMyTypeToCentral();
    }
  }
  // clean up ram
  WiFi.scanDelete();
}

void attempToPair() {
  Serial.print("Processing: ");
  
  for (int ii = 0; ii < 6; ++ii ) {
    Serial.print((uint8_t) potentialCentral.peer_addr[ii], HEX);
    if (ii != 5) Serial.print(":");
  }
  Serial.print(", Status:");

  // check if the peer exists
  if (!esp_now_is_peer_exist(potentialCentral.peer_addr)) {
    Serial.println("Pairing");
    esp_err_t addStatus = esp_now_add_peer(&potentialCentral);
    if (addStatus == ESP_OK) {
      Serial.println("Paired");
      sendMyTypeToCentral();
    } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
      Serial.println("ESPNOW Not Init");
      initESPNow();
      if(noOfAttempts < 8){
        attempToPair();
      }
    } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Add Peer - Invalid Argument");
      if(noOfAttempts < 8) attempToPair(); 
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
      Serial.println("Peer list full");
      if(noOfAttempts < 8) attempToPair(); 
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("Out of memory"); 
      if(noOfAttempts < 8) attempToPair(); 
      noOfAttempts++;
    } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
      Serial.println("Peer Exists");
      if(noOfAttempts < 8) attempToPair(); 
      noOfAttempts++;
    } else {
      Serial.println("Not sure what happened");
      if(noOfAttempts < 8) attempToPair(); 
      noOfAttempts++;
    }
    delay(100);
  }
  if(noOfAttempts >= 8) return false;
  return true;
}

// 
void sendMyTypeToCentral(){
  Serial.print("sending my type to central");
  uint8_t data = 101;
  esp_err_t result = esp_now_send(potentialCentral.peer_addr, &data, sizeof(data)); // number needs to be same with what slave is expecting
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

void deleteUnactiveCentral(){
  if(millis() < lastTimeDataRecived){
    lastTimeDataRecived = millis();
    
  }else if(millis() - lastTimeDataRecived > 240000){
    bool sendedIMyTypeToCentral = false;
    uint8_t central_addr = 0;  
    memcpy(&central, 0 , sizeof(central));
    
    esp_err_t delStatus = esp_now_del_peer(central.peer_addr);
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
  Serial2.begin(9600, SERIAL_8N1,16,17);
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_STA);;w
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  initESPNow();
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);
  
  

  
  
  M5.begin();  
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.setFreeFont(FSB12);   
  
}

int val;
byte count = 0;

static void printInt(unsigned long val, bool valid, int len)
{
  //char sz[32] = "*****************";
  char sz[32] = "";
  if (valid) sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i) sz[i] = ' ';
  if (len > 0) sz[len-1] = ' ';
  M5.Lcd.print(sz);
  smartDelay(0);
}

// This custom version of delay() ensures that the gps object
// is being "fed".

static void smartDelay(unsigned long ms){
  unsigned long start = millis();
  do
  {
    while (Serial2.available())
      gps.encode(Serial2.read());
  } while (millis() - start < ms);
}

void loop() {
  numberOfSatellites =  gps.satellites.value();
  latitude = gps.location.lat();
  longitude = gps.location.lng();
  hdop = gps.hdop.value() ;
  printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
  Serial.print("numberOfSatellites is: "); Serial.println(numberOfSatellites);
  Serial.print("hdop is:               "); Serial.println(hdop);
  Serial.print("latitude is:           "); Serial.println(latitude);
  Serial.print("longitude is:          "); Serial.println(longitude);
  
  deleteUnactiveCentral();
  if(!didCentralSendConfirmation){
    ScanForCentral();
  }
  
  if(count % 3 == 0){
    count = 0;
    sendData();
  }
  count++; 
  delay(1000);
}
