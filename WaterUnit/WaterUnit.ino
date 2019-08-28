// NOTE: 
// Send all data after reciving new configuration
// 

// Store in EEPROM with theirs respective adresses in parentheses: byte(0) -- will be one if something was written, literRemaiding(1,4), pulseCounter(5,8), releOpen(9)
//


// char data[sizeof(float)];
// float f = 0.6f;
// memcpy(data, &f, sizeof f);    // send data
// float g;
// memcpy(&g, data, sizeof g);    // receive data


#include <esp_now.h>
#include <WiFi.h>
#include <EEPROM.h>

#define EEPROM_SIZE 64
#define CHANNEL 1

#define ANALOG_PIN 14 // preasure - connected to analog pin 3
                       // outside leads to ground and +5V 

esp_now_peer_info_t master;
bool sendedIMyTypeToCentral = false;
uint8_t master_addr;

bool releOpen = false;
bool connectionToWaterSource;

const float maxVolumeOfTank = 40.7;
const float remainderWhenLowSensorHitted = 3;

float litersRemaining;
float temperature;

bool connectionToWater;
bool topTankSensor;
bool bottomTankSensor;
int pulseCounter;
byte validityOfData;

// send and recive 
struct SendRecvDataStruct{
  bool lowSensor;
  bool connectionToWaterSource;
  // 0 - data is valid, system hit top and counters were reseted, 1 - data could be valid, but was loaded from EEPROM, 2 - EEPROM was empty thus we can't guess state of tank, 3 - tank wasnt filled to its full capacity
  byte validityOfData;
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

// counting speed is diffrent than what it should be
// what to set when water is refiling
void AddPulse() {
  // evry liter / half a leter
  if(releOpen){
    // here when water is refilling
  }else{
    pulseCounter++;
    litersRemaining -= (pulseCounter/750);
  }
  // evry +- liter we want to 
  if(pulseCounter% 750 == 0){
    storeDataInEEPROM();
  }
} 
// Store in EEPROM with theirs respective adresses in parentheses: byte(0) -- will be one if something was written, literRemaiding(1,4), pulseCounter(5,8), releOpen(9)
//


// char data[sizeof(float)];
// float f = 0.6f;
// memcpy(data, &f, sizeof f);    // send data
// float g;
// memcpy(&g, data, sizeof g);    // receive data
void storeDataInEEPROM(){
  if(EEPROM.read(0) == 0){
      EEPROM.write(0,1);
  }
  // split data into bytes
  char temp[sizeof(litersRemaining)];
  memcpy(temp, &litersRemaining, sizeof(litersRemaining));
  for (int i = 0; i < (sizeof(temp)/ sizeof(temp[0])); i++){
    EEPROM.write(i+1,temp[i]);
  }
  temp[sizeof(pulseCounter)];
  memcpy(temp, &pulseCounter, sizeof(pulseCounter));
  for (int i = 0; i < (sizeof(temp)/ sizeof(temp[0])); i++){
    EEPROM.write(i+5,temp[i]);
  }
  /*
  if(releOpen == true){
    write(9, 1);
  }else{
    write(9, 0);
  }
  */
  EEPROM.write(9,releOpen);
  
}

void loadDataFormEEPROM(){
  
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

void sendData() {
   // no need to send all data

  SendRecvDataStruct data;
  data.connectionToWaterSource = connectionToWaterSource;
  data.litersRemaining = litersRemaining;
  data.temperature = temperature;
  
  uint8_t dataToBeSend[sizeof(data)];
  memcpy(dataToBeSend, &data, sizeof(data));
 
  esp_err_t result = esp_now_send(master.peer_addr, dataToBeSend, sizeof(dataToBeSend));
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
// also sets up master, in case master asks again than address will be set up again for new master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(*mac_addr);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);


  if (*data == (uint8_t) 190){
      master.channel = 1;
      master.encrypt = 0;
      memcpy(master.peer_addr, mac_addr, sizeof(mac_addr)+8); // size if diffrent
      sendedIMyTypeToCentral = false;
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
  // 1 or 3
  if(EEPROM.read(0) == 0){
    validityOfData = 2;
  }else{
    validityOfData = 1;
    loadDataFormEEPROM();
  }
   
  pinMode(4, OUTPUT);             // rele (LOW vypnuto, HIGH zapnuto
  pinMode(15, INPUT);             // snímač hladiny Horni, HIGH sepnuto
  pinMode(13, INPUT);             // snímač hladiny spodní, HIGH sepnuto
  pinMode(5, INPUT);              // Prutokomer impulsy
  attachInterrupt(5, AddPulse, FALLING);
}

void loop() {
  int val;
  //maxVolumeOfTank = 40.7
  // remainderWhenLowSensorHitted
  connectionToWater = (analogRead(ANALOG_PIN) > 250) ? true : false;
  topTankSensor = (digitalRead(13) == HIGH) ? true : false;
  bottomTankSensor = (digitalRead(15) == HIGH) ? true : false;
  // take care of temperature
  if(connectionToWater && !topTankSensor && temperature > 4){
    // we can refill tank
    digitalWrite(4, LOW);
    releOpen = true;
    // add value for refilling
  }else if(releOpen && topTankSensor){
    // close reffiling of tank
    digitalWrite(4, HIGH);
    releOpen = false;  
    litersRemaining = maxVolumeOfTank;
    validityOfData = 0;
  }else if(!connectionToWater && releOpen){
    // we were refilling but lost connection to water source
    // we dont known what is current state of tank
    validityOfData = 4;
  }else if(bottomTankSensor == true){
    litersRemaining = remainderWhenLowSensorHitted;
    validityOfData = 0;
  }
  
}
