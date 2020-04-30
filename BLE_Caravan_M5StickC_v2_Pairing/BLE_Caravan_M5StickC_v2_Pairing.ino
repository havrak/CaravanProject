
/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

//#define OLED
#define M5Stick
//#define M5StackCore
#define Mulivan_summer
//#define Multivan_winter
//#define Caravan  

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#ifdef OLED
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#endif

#include <esp_now.h>
#include <WiFi.h>

// Global copy of slave
esp_now_peer_info_t slave;
#define CHANNEL 1
#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0

#ifdef OLED
SSD1306  display(0x3c, 5, 4);
#endif

#ifdef M5Stick
#include <M5StickC.h>
#endif

#ifdef M5StackCore
#include <M5Stack.h>
#endif

#ifdef Mulivan_summer
 String FLTyreID = "000358";
 String FRTyreID = "00036D";
 String RLTyreID = "000104";
 String RRTyreID = "0001C4";
#endif

#ifdef Mulivan_winter
 String FLTyreID = "3004E7";
 String FRTyreID = "100878";
 String RLTyreID = "400212";
 String RRTyreID = "200613";
#endif

#ifdef Caravan
 String FLTyreID = "10028D";
 String FRTyreID = "200323";
 String RLTyreID = "3000DB";
 String RRTyreID = "400058";
#endif
         
          //Caravan
          // FL tyre 10028D
          // FR tyre 200323
          // RL tyre 3000DB
          // RR tyre 400058

          //Multak
          // FL tyre 000358
          // FR tyre 00036D
          // RL tyre 000104
          // RR tyre 0001C4

struct TMPS_STRUCT {
     char  Unit[20]; 
     float FLPreassure; 
     float FRPreassure;
     float RLPreassure;
     float RRPreassure;
     float FLTemperature; 
     float FRTemperature; 
     float RLTemperature; 
     float RRTemperature;
     int  FLBatt;
     int  FRBatt;
     int  RLBatt;
     int  RRBatt;     
     long LastScanTime;
     long FLScanTime;
     long FRScanTime;
     long RLScanTime;
     long RRScanTime;  
 }; 

TMPS_STRUCT  TMPS_Data;

void(* resetFunc) (void) = 0; //declare reset function @ address 0


// Init ESP Now with fallback
void InitESPNow() {
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// Scan for slaves in AP mode
void ScanForSlave() {
  int8_t scanResults = WiFi.scanNetworks();
  // reset on each scan
  bool slaveFound = 0;
  memset(&slave, 0, sizeof(slave));

 // Serial.println("");
  if (scanResults == 0) {
 //   Serial.println("No WiFi devices in AP Mode found");
  } else {
 //   Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

  //    if (PRINTSCANRESULTS) {
  //      Serial.print(i + 1);
  //      Serial.print(": ");
  //      Serial.print(SSID);
  //      Serial.print(" (");
  //      Serial.print(RSSI);
  //      Serial.print(")");
  //      Serial.println("");
  //    }
      delay(10);
      // Check if the current device starts with `Slave`
/*      if (SSID.indexOf("Slave") == 0) {
        // SSID of interest
  //      Serial.println("Found a Slave.");
  //      Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x%c",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            slave.peer_addr[ii] = (uint8_t) mac[ii];
            Serial.print(BSSIDstr);
          }
        }

        slave.channel = CHANNEL; // pick a channel
        slave.encrypt = 0; // no encryption

        slaveFound = 1;
        // we are planning to have only one slave in this example;
        // Hence, break after we find one, to be a bit efficient
        break;
      }
*/

//* Node M5stack 
      int mac[6];
      mac[0]= 0xB4;
      mac[1]= 0xE6;
      mac[2]= 0x2D;
      mac[3]= 0xFA;
      mac[4]= 0x5D;
      mac[5]= 0xE6 ;
     



/* Silver M5stack
      int mac[6];
      mac[0]= 0xA4;
      mac[1]= 0xCF;
      mac[2]= 0x12;
      mac[3]= 0x6D;
      mac[4]= 0x71;
      mac[5]= 0xE9 ;
*/     

      for (int ii = 0; ii < 6; ++ii ) {
        slave.peer_addr[ii] = (uint8_t) mac[ii];
      }
      slave.channel = CHANNEL; // pick a channel
      slave.encrypt = 0; // no encryption

      slaveFound = 1;
      // we are planning to have only one slave in this example;
      // Hence, break after we find one, to be a bit efficient
      break;        
    }
  }

  if (slaveFound) {
    Serial.println("Slave Found, processing..");
  } else {
    Serial.println("Slave Not Found, trying again.");
  }

  // clean up ram
  WiFi.scanDelete();
  if (millis() > 600000) {
    Serial.println("Rebooting");    
    resetFunc();  //call reset
  }
}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
bool manageSlave() {
  if (slave.channel == CHANNEL) {
    if (DELETEBEFOREPAIR) {
      deletePeer();
    }

    Serial.print("Slave Status: ");
    const esp_now_peer_info_t *peer = &slave;
    const uint8_t *peer_addr = slave.peer_addr;
    // check if the peer exists
    bool exists = esp_now_is_peer_exist(peer_addr);
    if ( exists) {
      // Slave already paired.
      Serial.println("Already Paired");
      return true;
    } else {
      // Slave not paired, attempt pair
      esp_err_t addStatus = esp_now_add_peer(peer);
      if (addStatus == ESP_OK) {
        // Pair success
        Serial.println("Pair success");
        return true;
      } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
        // How did we get so far!!
        Serial.println("ESPNOW Not Init");
        return false;
      } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
        Serial.println("Invalid Argument");
        return false;
      } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
        Serial.println("Peer list full");
        return false;
      } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
        Serial.println("Out of memory");
        return false;
      } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
        Serial.println("Peer Exists");
        return true;
      } else {
        Serial.println("Not sure what happened");
        return false;
      }
    }
  } else {
    // No slave found to process
    Serial.println("No Slave found to process");
    return false;
  }
}

void deletePeer() {
  const esp_now_peer_info_t *peer = &slave;
  const uint8_t *peer_addr = slave.peer_addr;
  esp_err_t delStatus = esp_now_del_peer(peer_addr);
  Serial.print("Slave Delete Status: ");
  if (delStatus == ESP_OK) {
    // Delete success
    Serial.println("Success");
  } else if (delStatus == ESP_ERR_ESPNOW_NOT_INIT) {
    // How did we get so far!!
    Serial.println("ESPNOW Not Init");
  } else if (delStatus == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (delStatus == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else {
    Serial.println("Not sure what happened");
  }
}

uint8_t data = 0;
// send data
void sendData() {
  data++;
  const uint8_t *peer_addr = slave.peer_addr;

     sprintf(TMPS_Data.Unit , "%s" , "Caravan Hobby650");
     TMPS_Data.LastScanTime = millis();

  uint8_t bs[sizeof(TMPS_Data)]; 
  memcpy(bs, &TMPS_Data, sizeof(TMPS_Data)); 
  Serial.print("Sending: "); Serial.println(data);
  esp_err_t result = esp_now_send(peer_addr, bs, sizeof(bs));
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

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


int scanTime = 30; //In seconds
int temperature;
int preassure;
int batt;
float ftemperature;
float fpreassure;
int   ibatt;
char TempHex[10] = "9806";
char manData[40];
char FRpreassure[10] = "---";
char FLpreassure[10] = "---";
char RRpreassure[10] = "---";
char RLpreassure[10] = "---";
char FRtemperature[10] = "---";
char FLtemperature[10] = "---";
char RRtemperature[10] = "---";
char RLtemperature[10] = "---";


int StrToHex(char str[])
{
  return (int) strtol(str, 0, 16);
}



char * TurnByteOrder (char instr[])
{
  int i;
  static char outstr[40] ;
  i=0;
  memset(outstr, 0, sizeof(outstr));
  while  (i < strlen(instr)) {
    outstr[i] = instr[strlen(instr)-2-i];
    outstr[i+1] = instr[strlen(instr)-1-i];      
    i++;
    i++;    
  }
  i--;
  outstr[strlen(instr)+1] = '\0';
  return outstr;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        preassure = 0;
        temperature = 0;
        batt = 0;
        if (strstr (advertisedDevice.getName().c_str(),"TPMS")) {
          char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
          strcpy (manData,pHex);
          char *temperatureHex = manData + 24;
          temperatureHex[4] = '\0';
          Serial.printf("TempHex >%s< \n", temperatureHex);
          temperature = StrToHex(TurnByteOrder(temperatureHex));
  
          char *preassureHex = manData + 16;
          preassureHex[8] = '\0';
          Serial.printf("PreassureHex >%s< \n", preassureHex);
          preassure = StrToHex(TurnByteOrder(preassureHex));

          char *battHex = manData + 32;
          battHex[2] = '\0';
          Serial.printf("BattHex >%s< \n", battHex);
          batt = StrToHex(TurnByteOrder(battHex));
          
          fpreassure = (float)preassure / 100000;
          ftemperature = (float)temperature / 100;
          ibatt = batt;


          if (ftemperature > 327) {
            ftemperature = ftemperature - 655.35; 
          }

          
          if (strstr (advertisedDevice.getName().c_str(),"100878")) {
//          if (strstr (advertisedDevice.getName().c_str(),"10028D")) {

            sprintf(FLpreassure , "%4.2f" , fpreassure);
            sprintf(FLtemperature , "%4.1f" , ftemperature);
            TMPS_Data.FLPreassure=fpreassure; 
            TMPS_Data.FLTemperature=ftemperature; 
            TMPS_Data.FLBatt = ibatt;
            TMPS_Data.FLScanTime = millis();

          }

           if (strstr (advertisedDevice.getName().c_str(),"3004E7")) {
//          if (strstr (advertisedDevice.getName().c_str(),"200323")) {

            sprintf(FRpreassure , "%4.2f" , fpreassure);
            sprintf(FRtemperature , "%4.1f" , ftemperature);
            TMPS_Data.FRPreassure=fpreassure; 
            TMPS_Data.FRTemperature=ftemperature;
            TMPS_Data.FRBatt = ibatt;
            TMPS_Data.FRScanTime = millis();
             
          }

          if (strstr (advertisedDevice.getName().c_str(),"200613")) {
//          if (strstr (advertisedDevice.getName().c_str(),"3000DB")) {

            sprintf(RLpreassure , "%4.2f" , fpreassure);
            sprintf(RLtemperature , "%4.1f" , ftemperature);
            TMPS_Data.RLPreassure=fpreassure; 
            TMPS_Data.RLTemperature=ftemperature;
            TMPS_Data.RLBatt = ibatt;            
            TMPS_Data.RLScanTime = millis();
             
          }

          if (strstr (advertisedDevice.getName().c_str(),"400212")) {
//          if (strstr (advertisedDevice.getName().c_str(),"400058")) {

            sprintf(RRpreassure , "%4.2f" , fpreassure);
            sprintf(RRtemperature , "%4.1f" , ftemperature);
            TMPS_Data.RRPreassure=fpreassure; 
            TMPS_Data.RRTemperature=ftemperature;
            TMPS_Data.RRBatt = ibatt;            
            TMPS_Data.RRScanTime = millis();  
                  
          }
          
        }
      Serial.printf("Advertised Device: %s P:%i T:%i\n",advertisedDevice.toString().c_str(),preassure,temperature);

      if (advertisedDevice.haveRSSI()){
        Serial.printf("Rssi: %d \n", (int)advertisedDevice.getRSSI());
      }

/*
      #ifdef OLED
        display.clear();
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_LEFT);      
        display.drawString(0, 0, "FL");
        display.drawString(0, 16, "FR");
        display.drawString(0, 32, "RL");
        display.drawString(0, 48, "RR");
        display.drawString(22, 0, ":");
        display.drawString(22, 16, ":");
        display.drawString(22, 32, ":");
        display.drawString(22, 48, ":");
        display.setTextAlignment(TEXT_ALIGN_RIGHT);      
        display.drawString(70, 0, FLpreassure);
        display.drawString(70, 16, FRpreassure);
        display.drawString(70, 32, RLpreassure);
        display.drawString(70, 48, RRpreassure);
        display.setTextAlignment(TEXT_ALIGN_RIGHT);      
        display.drawString(120, 0, FLtemperature);
        display.drawString(120, 16, FRtemperature);
        display.drawString(120, 32, RLtemperature);
        display.drawString(120, 48, RRtemperature);
        display.display();
      #endif
  */
    }
};


void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");

  TMPS_Data.LastScanTime = 0;
  TMPS_Data.FLScanTime = 0;
  TMPS_Data.FRScanTime = 0;
  TMPS_Data.RLScanTime = 0;
  TMPS_Data.RRScanTime = 0;  

  
  #ifdef M5StackCore
      M5.begin();
      M5.Lcd.setTextColor(TFT_YELLOW); 
      M5.Lcd.setFreeFont(FSB12);   
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.println("FL");
      M5.Lcd.println("FR");
      M5.Lcd.println("RL");
      M5.Lcd.println("RR");
  #endif

  #ifdef  M5Stick
      M5.begin();
      M5.Lcd.setRotation(3);
      M5.Lcd.setTextColor(TFT_YELLOW); 
      M5.Lcd.setTextSize(1);      
      M5.Lcd.setTextFont(2);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.setTextDatum(ML_DATUM);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.println("FL");
      M5.Lcd.println("FR");
      M5.Lcd.println("RL");
      M5.Lcd.println("RR");
  #endif

  #ifdef OLED
    display.init();
    display.flipScreenVertically();
    display.setContrast(255);
    display.clear();
  #endif    

  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value  
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");

  WiFi.mode(WIFI_STA);
  Serial.println("ESPNow/Basic/Master Example");
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //delay(2000);
///*
          TMPS_Data.FLPreassure=3.10; 
          TMPS_Data.FLTemperature=21;
          TMPS_Data.FLBatt = 81;
          TMPS_Data.FLScanTime = 1;
          TMPS_Data.FRPreassure=3.20; 
          TMPS_Data.FRTemperature=22;
          TMPS_Data.FRBatt = 82;
          TMPS_Data.FRScanTime = 1;
          TMPS_Data.RLPreassure=3.30; 
          TMPS_Data.RLTemperature=23;
          TMPS_Data.RLBatt = 83;
          TMPS_Data.RLScanTime = 1;
          TMPS_Data.RRPreassure=3.40; 
          TMPS_Data.RRTemperature=24;
          TMPS_Data.RRBatt = 84;
          TMPS_Data.RRScanTime = 1;
          TMPS_Data.LastScanTime = 1;
  //*/        

  
  Serial.println("========================New Scan ==================================================================");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  //strcpy (FRpreassure,"3,1");


  ScanForSlave();
  // If Slave is found, it would be populate in `slave` variable
  // We will check if `slave` is defined and then we proceed further
  if (slave.channel == CHANNEL) { // check if slave channel is defined
    // `slave` is defined
    // Add slave as peer if it has not been added already
    bool isPaired = manageSlave();
    if (isPaired) {
      // pair success or already paired
      // Send data to device
      sendData();
    } else {
      // slave pair failed
      Serial.println("Slave pair failed!");
    }
  }
  else {
    // No slave found to process
  }

      #ifdef OLED
        display.clear();
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_LEFT);      
        display.drawString(0, 0, "FL");
        display.drawString(0, 16, "FR");
        display.drawString(0, 32, "RL");
        display.drawString(0, 48, "RR");
        display.drawString(22, 0, ":");
        display.drawString(22, 16, ":");
        display.drawString(22, 32, ":");
        display.drawString(22, 48, ":");
        display.setTextAlignment(TEXT_ALIGN_RIGHT);      
        display.drawString(70, 0, FLpreassure);
        display.drawString(70, 16, FRpreassure);
        display.drawString(70, 32, RLpreassure);
        display.drawString(70, 48, RRpreassure);
        display.setTextAlignment(TEXT_ALIGN_RIGHT);      
        display.drawString(120, 0, FLtemperature);
        display.drawString(120, 16, FRtemperature);
        display.drawString(120, 32, RLtemperature);
        display.drawString(120, 48, RRtemperature);
        display.display();
      #endif

      #ifdef M5Stick
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.setTextColor(TFT_YELLOW); 
        M5.Lcd.setTextSize(1); 
        M5.Lcd.setTextFont(2);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print("FL");
        M5.Lcd.setCursor(0, 16);
        M5.Lcd.print("FR");
        M5.Lcd.setCursor(0, 32);
        M5.Lcd.print("RL");
        M5.Lcd.setCursor(0, 48);
        M5.Lcd.print("RR");
        M5.Lcd.setCursor(22, 0);
        M5.Lcd.print(":");
        M5.Lcd.setCursor(22, 16);
        M5.Lcd.print(":");
        M5.Lcd.setCursor(22, 32);
        M5.Lcd.print(":");
        M5.Lcd.setCursor(22, 48);
        M5.Lcd.print(":");
        M5.Lcd.setTextDatum(MR_DATUM);
        M5.Lcd.setCursor(70, 0);
        M5.Lcd.print(FLpreassure);
        M5.Lcd.setCursor(70, 16);
        M5.Lcd.print(FRpreassure);
        M5.Lcd.setCursor(70, 32);
        M5.Lcd.print(RLpreassure);
        M5.Lcd.setCursor(70, 48);
        M5.Lcd.print(RRpreassure);
        M5.Lcd.setTextDatum(MR_DATUM);
        M5.Lcd.setCursor(120, 0);
        M5.Lcd.print(FLtemperature);
        M5.Lcd.setCursor(120, 16);
        M5.Lcd.print(FRtemperature);
        M5.Lcd.setCursor(120, 32);
        M5.Lcd.print(RLtemperature);
        M5.Lcd.setCursor(120, 48);
        M5.Lcd.print(RRtemperature);
      #endif
      pBLEScan->clearResults();
}
