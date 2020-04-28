// NOTE: 
// Send all data after reciving new configuration
// this call contain some constants you can find those for your specific hardware via WaterTest
#include <M5Stack.h>
#include "Free_Fonts.h"
#include <esp_now.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <TinyGPS++.h>
#include <Time.h>           // to convert date to UNIX time stamp
#define EEPROM_SIZE 7

#define CHANNEL 1


esp_now_peer_info_t central;
esp_now_peer_info_t potencialCentral;
esp_now_peer_info_t emptyEspInfo;

bool sendedIMyTypeToCentral = false;
bool didCentralSendConfirmation = false;
bool checkingAgaintsEEPROMmaster = false;
bool isEEPROMinitialized = false;

byte noOfAttempts = 0;

TinyGPSPlus gps;

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
  time_t time;
};

boolean checkIfTwoAddressesAreSame(const uint8_t *addr1,const uint8_t *addr2){
  if(sizeof(addr1) != sizeof(addr2)){
    return false;
  }
  for(int i = 0; i < (sizeof(addr1)/sizeof(addr1[0])); i++){
    if(addr1[i] != addr2[i]) return false;
  }
  return true;
}

// prints given mac address 
void printAddress(const uint8_t addr[]){
  for (int i = 0; i < 6; ++i ) {
    Serial.print((uint8_t) addr[i], HEX);
    if (i != 5) Serial.print(":");
  }
  Serial.println();
}

// millis() counter resets every 50 days, gives time diffrence between millis() and sTime in argument
unsigned long getTimeDiffrence(const unsigned long sTime){
  if(millis() < sTime){
    return (ULONG_MAX - sTime) + millis();
  }
  return millis() - sTime;
}

// Init ESP Now with fallback
void initESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("SU | initESPNow | ESPNow Init Success");
  }
  else {
    Serial.println("SU | initESPNow | ESPNow Init Failed");
    ESP.restart();
  }
}

// first byte in/off EEPROM, 1 - 6 is masters mac
void storeDataInEEPROM(){
  if(EEPROM.read(0) == 0){ // limited number of writes
      EEPROM.write(0,1);
  }
  uint8_t test[6];
  for(int i=1 ; i < 7; i++ ){
    test[i-1] = EEPROM.read(i);  
  }
  if(!checkIfTwoAddressesAreSame(test, central.peer_addr)){
    Serial.println("SU | storeDataInEEPROM | storing new central mac into EEPROM");
    for(int i=1 ; i < 7; i++ ){
      EEPROM.write(i,central.peer_addr[i-1]);  
    }
  }
  // check if data is same
  EEPROM.commit();
}

// 
void sendMyTypeToCentral(){
  Serial.print("SU | sendMyTypeToCentral | sending my type to central, its mac is: "); printAddress(potencialCentral.peer_addr); Serial.println("");
  uint8_t data = 101;
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

int counter = 0;
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status != ESP_NOW_SEND_SUCCESS && !sendedIMyTypeToCentral && counter <10){ // try until data is send successfully
    Serial.println("SU | onDataSent | Sending info failed");
    delay(100);
    sendMyTypeToCentral();
    counter++;
  }else if (status == ESP_NOW_SEND_SUCCESS && !sendedIMyTypeToCentral){
    sendedIMyTypeToCentral = true;
    counter=0;
  }
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("SU | onDataSent | Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("SU | onDataSent | Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  
}

void sendData() {
  Serial.println();
  Serial.println("SU | sendData | Sending data");
  SendRecvDataStruct data;
  
  data.hdop = hdop;
  data.numberOfSatellites = numberOfSatellites;
  data.latitude = latitude;
  data.longitude = longitude;
  data.time = now();
  
  uint8_t dataToBeSend[sizeof(data)];
  memcpy(dataToBeSend, &data, sizeof(data));
  Serial.print("SU | sendData | Size of dataToBeSend is: "); Serial.println(sizeof(dataToBeSend));
  
  esp_err_t result = esp_now_send(central.peer_addr, dataToBeSend, sizeof(dataToBeSend));
  
  Serial.print("SU | sendData | Send Status: ");
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
  Serial.print("SU | onDataRecv | Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("SU | onDataRecv | Last Packet Recv Data: "); Serial.println(*data);

  
  if (*data == (uint8_t) 92){
    if(checkIfTwoAddressesAreSame(potencialCentral.peer_addr, mac_addr) || (!isEEPROMinitialized && checkIfTwoAddressesAreSame(potencialCentral.peer_addr, emptyEspInfo.peer_addr))){ // after OR -- we recived info EEPROM was down yet we didn't foud any centaral so potencialCentral wouldn't be empty
      Serial.println("SU | onDataRecv | Set up central");
      counter=0;
      memcpy(central.peer_addr, mac_addr, sizeof(central.peer_addr)); // size if diffrent,  sa d sa dsa sad 
      didCentralSendConfirmation = true;
      esp_err_t addStatus = esp_now_add_peer(&central);
      lastTimeDataRecived = millis();    
      Serial.print("SU | onDataRecv | Centrals mac address is: "); printAddress(central.peer_addr); Serial.println("");
      storeDataInEEPROM(); // save new central into EEPROM
    }else  {
      Serial.println("SU | onDataRecv | got 92 from unit I wasn't expecting");  
    }
  }
  if(checkIfTwoAddressesAreSame(mac_addr, central.peer_addr)){
      Serial.println("SU | onDataRecv | got some data");
      lastTimeDataRecived = millis();
      if(*data != (uint8_t) 88){ // check if message is not just a ping
        // NEW CONFIGURATION IS PROCESSED HERE
     }
  }
}

long startTime = -1; // we give 10 seconds to central to respod, then we will try diffrent cenral with same name, values wont be so big that we would have to worry about unsigned long max size is bigger


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

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1,16,17);
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_STA);
  // This is the mac address of the Slave in AP Mode
  Serial.print("SU | SETUP | STA MAC: "); Serial.println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  
  initESPNow();
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);
  
  potencialCentral = emptyEspInfo;
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
        //potencialCentral.peer_addr[i] = (uint8_t) EEPROM.read(i+1);// first byte is to check
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
  pinMode(25, OUTPUT);                      // speaker pin
  digitalWrite(25, LOW);
  
  M5.begin();  
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.setFreeFont(FSB12);   
  ScanForCentral();
}

int val;
byte count = 0;

// This custom version of delay() ensures that the gps object is being "fed".

static void smartDelay(unsigned long ms){
  unsigned long start = millis();
  do
  {
    while (Serial2.available())
      gps.encode(Serial2.read());
  } while (getTimeDiffrence(start) < ms);
}

void loop() {
  
  numberOfSatellites =  gps.satellites.value();
  latitude = gps.location.lat();
  longitude = gps.location.lng();
  hdop = gps.hdop.value() ;
  setTime(gps.time.hour(),gps.time.minute(),gps.time.second(),gps.date.day(),gps.date.month(),gps.date.year());
  
  Serial.println("");
  Serial.print("SU | LOOP | numberOfSatellites is: "); Serial.println(numberOfSatellites);
  Serial.print("SU | LOOP | hdop is:               "); Serial.println(hdop);
  Serial.print("SU | LOOP | latitude is:           "); Serial.println(latitude);
  Serial.print("SU | LOOP | longitude is:          "); Serial.println(longitude);
  Serial.print("SU | LOOP | hour is:               "); Serial.println(gps.time.hour());
  Serial.println("");
  
  if(!didCentralSendConfirmation){
    if((startTime == -1 && !checkingAgaintsEEPROMmaster) || (getTimeDiffrence(startTime) > (checkingAgaintsEEPROMmaster ? 15000 : 10000))){ // we waited too long to get a
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
  //delay(1000);
  smartDelay(1000);
}
