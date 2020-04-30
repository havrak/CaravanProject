/**
   ESPNOW - Basic communication - Slave
   Date: 26th September 2017
   Author: Arvind Ravulavaru <https://github.com/arvindr21>
   Purpose: ESPNow Communication between a Master ESP32 and a Slave ESP32
   Description: This sketch consists of the code for the Slave module.
   Resources: (A bit outdated)
   a. https://espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf
   b. http://www.esploradores.com/practica-6-conexion-esp-now/

   << This Device Slave >>

   Flow: Master
   Step 1 : ESPNow Init on Master and set it in STA mode
   Step 2 : Start scanning for Slave ESP32 (we have added a prefix of `slave` to the SSID of slave for an easy setup)
   Step 3 : Once found, add Slave as peer
   Step 4 : Register for send callback
   Step 5 : Start Transmitting data from Master to Slave

   Flow: Slave
   Step 1 : ESPNow Init on Slave
   Step 2 : Update the SSID of Slave with a prefix of `slave`
   Step 3 : Set Slave in AP mode
   Step 4 : Register for receive callback and wait for data
   Step 5 : Once data arrives, print it in the serial monitor

   Note: Master and Slave have been defined to easily understand the setup.
         Based on the ESPNOW API, there is no concept of Master and Slave.
         Any devices can act as master or salve.
*/

#include <esp_now.h>
#include <WiFi.h>
#include <M5Stack.h>
#include "Free_Fonts.h"

#define CHANNEL 1


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

char FLpreassure[10];
char FRpreassure[10];
char RLpreassure[10];
char RRpreassure[10];
char FLTemp[10];
char FRTemp[10];
char RLTemp[10];
char RRTemp[10];
char FLBatt[5];
char FRBatt[5];
char RLBatt[5];
char RRBatt[5];
int  FLpreassure_rest;
int  FRpreassure_rest;
int  RLpreassure_rest;
int  RRpreassure_rest;
boolean FLValidData;
boolean FRValidData;
boolean RLValidData;
boolean RRValidData;

long LastReceiveTime;
long ProgressBarTime;
int ProgressBarCount;
int iDisplayStatus = 0;
String  ProgressBarString = String("");
String sPrevValues;
String sNewValues;

  TMPS_STRUCT TPMS_Data;



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

// config AP SSID
void configDeviceAP() {
  char* SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 1);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with Hidden AP: " + String(SSID));
  }
}


void configDeviceAP_Visible() {
  char* SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with Visible AP: " + String(SSID));
  }
}




void setup() {
  M5.begin();
  Serial.begin(115200);
  Serial.println("ESPNow/Basic/Slave Example");

  pinMode(39,INPUT);
  pinMode(38,INPUT);
  pinMode(37,INPUT);
  M5.Lcd.setTextColor(TFT_YELLOW); 
  M5.Lcd.setFreeFont(FSB12);   
  //M5.Lcd.setCursor(0, 0);
  //M5.Lcd.println();          // Free fonts plot with the baseline (imaginary line the letter A would sit on)
  // as the datum, so we must move the cursor down a line from the 0,0 position
  //M5.Lcd.print("Serif Bold 9pt");
  //delay (5000);
  LastReceiveTime = 0;
  ProgressBarTime = 0;
  ProgressBarCount = 0;
  String  ProgressBarString = String("");
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];

  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Data Length: "); Serial.println(sizeof(TPMS_Data));  
  Serial.println("");

  if ( sizeof(TPMS_Data) == 88 ) {
    memcpy(&TPMS_Data, data, sizeof(TPMS_Data));   
    Serial.print("Last Packet Recv Data: "); Serial.println(TPMS_Data.Unit);
    LastReceiveTime = millis();
  }
}

void loop() {
int val=0;
/*
  TPMS_Data.FLPreassure = 3.1;
  TPMS_Data.FRPreassure = 3.3;
  TPMS_Data.RLPreassure = 3.8;
  TPMS_Data.RRPreassure = 3.9;
  TPMS_Data.FLTemperature = 24.1; 
  TPMS_Data.FRTemperature = 28.9; 
  TPMS_Data.RLTemperature = 30.1; 
  TPMS_Data.RRTemperature = 45.1; 
*/
  
  if (LastReceiveTime ==0) {
    if (iDisplayStatus == 0) {
      M5.Lcd.fillScreen(TFT_BLACK);
      M5.Lcd.setTextColor(TFT_YELLOW); 
      M5.Lcd.setFreeFont(FSB12);
      M5.Lcd.setTextDatum(TL_DATUM);
      M5.Lcd.setCursor(0, 25);
      M5.Lcd.print("Waiting for");
      M5.Lcd.setCursor(0, 55);
      M5.Lcd.print("Signal");
      iDisplayStatus = 1;
    }
  }

  if (ProgressBarTime < LastReceiveTime) {
    ProgressBarTime = LastReceiveTime;
    ProgressBarCount = ProgressBarCount + 1;
    if (ProgressBarCount == 26) {
      ProgressBarCount =1;
      sPrevValues="";
    }
    ProgressBarString = String("");
    for (int i=1; i <= ProgressBarCount; i++){
    ProgressBarString = String( ProgressBarString + "-" );
    }
  }

  if (LastReceiveTime>0){
    if (millis()-LastReceiveTime>60000) {
      
      if (iDisplayStatus == 1 || iDisplayStatus == 3) {    
        M5.Lcd.fillScreen(TFT_BLACK);      
        M5.Lcd.setTextColor(TFT_YELLOW); 
        M5.Lcd.setTextSize(1); 
        M5.Lcd.setTextFont(4);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print("Waiting for");
        M5.Lcd.setCursor(0, 32);
        M5.Lcd.print("Signal Lost");
        iDisplayStatus = 2;        
      }
    } else {
        if (TPMS_Data.FLPreassure == 0) {
           if (FLValidData == false){
            sprintf(FLpreassure , "%s" , "-.-");
           }
        } else {
          sprintf(FLpreassure , "%2.1f" , round((TPMS_Data.FLPreassure-0.05)*10)/10);
          FLpreassure_rest = round((TPMS_Data.FLPreassure-0.005)*100)-(round((TPMS_Data.FLPreassure-0.05)*10)*10);
          FLValidData = true;
        }
        if(TPMS_Data.FRPreassure == 0) {
          if (FRValidData == false){
           sprintf(FRpreassure , "%s" , "-.-");
          }
        } else {
          sprintf(FRpreassure , "%2.1f" , round((TPMS_Data.FRPreassure-0.05)*10)/10);
          FRpreassure_rest = round((TPMS_Data.FRPreassure-0.005)*100)-(round((TPMS_Data.FRPreassure-0.05)*10)*10);
          FRValidData = true;
        }
        if(TPMS_Data.RLPreassure == 0) {
          if (RLValidData == false){
           sprintf(RLpreassure , "%s" , "-.-");
          }
        } else {
          sprintf(RLpreassure , "%2.1f" , round((TPMS_Data.RLPreassure-0.05)*10)/10);
          RLpreassure_rest = round((TPMS_Data.RLPreassure-0.005)*100)-(round((TPMS_Data.RLPreassure-0.05)*10)*10);
          RLValidData = true;
        }
        if(TPMS_Data.RRPreassure == 0) {
          if (RRValidData == false){
           sprintf(RRpreassure , "%s" , "-.-");
          }
        } else {
          sprintf(RRpreassure , "%2.1f" , round((TPMS_Data.RRPreassure-0.05)*10)/10);
          RRpreassure_rest = round((TPMS_Data.RRPreassure-0.005)*100)-(round((TPMS_Data.RRPreassure-0.05)*10)*10);
          RRValidData = true;
        }
      
        if(TPMS_Data.FLTemperature == 0){
           if (FLValidData == false){
            sprintf(FLTemp , "%s" , "--");
           }
        } else {
         sprintf(FLTemp , "%3.0f" , TPMS_Data.FLTemperature);
        }
        if(TPMS_Data.FRTemperature == 0){
           if (FRValidData == false){
             sprintf(FRTemp , "%s" , "--");
           }
        } else {
          sprintf(FRTemp , "%3.0f" , TPMS_Data.FRTemperature);
        }
        if(TPMS_Data.RLTemperature == 0){
           if (RLValidData == false){
             sprintf(RLTemp , "%s" , "--");
           }
        } else {
          sprintf(RLTemp , "%3.0f" , TPMS_Data.RLTemperature);
        }
        if(TPMS_Data.RRTemperature == 0){
           if (RRValidData == false){          
             sprintf(RRTemp , "%s" , "--");
           }
        } else {
          sprintf(RRTemp , "%3.0f" , TPMS_Data.RRTemperature);
        }

        if(TPMS_Data.FLBatt == 0){
           if (FLValidData == false){
            sprintf(FLBatt , "%s" , "--%");
           }
        } else {
         sprintf(FLBatt , "%2d%%" , TPMS_Data.FLBatt);
        }

        if(TPMS_Data.FRBatt == 0){
           if (FRValidData == false){
            sprintf(FRBatt , "%s" , "--%");
           }
        } else {
         sprintf(FRBatt , "%2d%%" , TPMS_Data.FRBatt);
        }

        if(TPMS_Data.RLBatt == 0){
           if (RLValidData == false){
            sprintf(RLBatt , "%s" , "--%");
           }
        } else {
         sprintf(RLBatt , "%2d%%" , TPMS_Data.RLBatt);
        }

        if(TPMS_Data.RRBatt == 0){
           if (RRValidData == false){
            sprintf(RRBatt , "%s" , "--%");
           }
        } else {
         sprintf(RRBatt , "%2d%%" , TPMS_Data.RRBatt);
        }

/*
        if(TPMS_Data.LastScanTime-TPMS_Data.FLScanTime>600000){
           sprintf(FLpreassure , "%s" , "-.-");
           sprintf(FLTemp , "%s" , "--");          
        }
        
        if(TPMS_Data.LastScanTime-TPMS_Data.FRScanTime>600000){
           sprintf(FRpreassure , "%s" , "-.-");
           sprintf(FRTemp , "%s" , "--");          
        }

        if(TPMS_Data.LastScanTime-TPMS_Data.RLScanTime>600000){
           sprintf(RLpreassure , "%s" , "-.-");
           sprintf(RLTemp , "%s" , "--");          
        }

        if(TPMS_Data.LastScanTime-TPMS_Data.RRScanTime>600000){
           sprintf(RRpreassure , "%s" , "-.-");
           sprintf(RRTemp , "%s" , "--");          
        }

        sprintf(
 */
        
        sNewValues = (String)FLpreassure + (String)FRpreassure + (String)RLpreassure + (String)RRpreassure + (String)FLTemp + (String)FRTemp + (String)RLTemp + (String)RRTemp + (String)FLBatt + (String)FRBatt + (String)RLBatt + (String)RRBatt;
        if (iDisplayStatus == 1 || iDisplayStatus == 2) {    
          M5.Lcd.fillScreen(TFT_BLACK);
          iDisplayStatus = 3;
        }
        if(sNewValues != sPrevValues) {
           sPrevValues = sNewValues;
           M5.Lcd.fillScreen(TFT_BLACK);
        }
        M5.Lcd.setTextColor(TFT_YELLOW); 
        M5.Lcd.setTextSize(2); 
        M5.Lcd.setTextFont(4);
        M5.Lcd.setTextDatum(BL_DATUM);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print(FLpreassure);
        M5.Lcd.setTextFont(2);
        M5.Lcd.print(FLpreassure_rest);

        M5.Lcd.setTextFont(4);
        M5.Lcd.setCursor(170, 0);
        M5.Lcd.print(FRpreassure);
        M5.Lcd.setTextFont(2);
        M5.Lcd.print(FRpreassure_rest);

        M5.Lcd.setTextFont(4);
        M5.Lcd.setCursor(0, 50);
        M5.Lcd.print(RLpreassure);
        M5.Lcd.setTextFont(2);
        M5.Lcd.print(RLpreassure_rest);

        M5.Lcd.setTextFont(4);
        M5.Lcd.setCursor(170, 50);
        M5.Lcd.print(RRpreassure);
        M5.Lcd.setTextFont(2);
        M5.Lcd.print(RRpreassure_rest);

        
        M5.Lcd.setTextSize(2); 
        M5.Lcd.setTextFont(1);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.setCursor(85, 8);
        M5.Lcd.print(FLTemp);
        M5.Lcd.print(char(247));
        M5.Lcd.print("C");
        M5.Lcd.setCursor(255, 8);
        M5.Lcd.print(FRTemp);
        M5.Lcd.print(char(247));
        M5.Lcd.print("C");
        M5.Lcd.setCursor(85, 58);
        M5.Lcd.print(RLTemp);
        M5.Lcd.print(char(247));
        M5.Lcd.print("C");        
        M5.Lcd.setCursor(255, 58);
        M5.Lcd.print(RRTemp);
        M5.Lcd.print(char(247));
        M5.Lcd.print("C");



        //M5.Lcd.setTextSize(2); 
        //M5.Lcd.setTextFont(1);
        //M5.Lcd.setTextDatum(ML_DATUM);
        //M5.Lcd.setCursor(120, 8);
        //M5.Lcd.print(FLBatt);
        //M5.Lcd.setCursor(285, 8);
        //M5.Lcd.print(FRBatt);
        //M5.Lcd.setCursor(120, 58);
        //M5.Lcd.print(RLBatt);
        //M5.Lcd.setCursor(285, 58);
        //M5.Lcd.print(RRBatt);
        
        M5.Lcd.setCursor(0, 100);
        M5.Lcd.print(ProgressBarString);      

        M5.Lcd.setTextColor(TFT_YELLOW,BLACK); 
        M5.Lcd.setCursor(0, 120);
        M5.Lcd.print("Button A :");
        val=digitalRead(39);
        M5.Lcd.println(val);
        if (val==0) {
            Serial.println("Disconnect");
            WiFi.softAPdisconnect(true);
            delay(1000);
            WiFi.mode(WIFI_AP);
            // configure device AP mode
            configDeviceAP_Visible();
            // This is the mac address of the Slave in AP Mode
            Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
            // Init ESPNow with a fallback logic
            InitESPNow();
            // Once ESPNow is successfully Init, we will register for recv CB to
            // get recv packer info.
            esp_now_register_recv_cb(OnDataRecv);
            LastReceiveTime ==0;
            iDisplayStatus == 0;
            sprintf(FLpreassure , "%s" , "-.-");            
            sprintf(FRpreassure , "%s" , "-.-");            
            sprintf(RLpreassure , "%s" , "-.-");            
            sprintf(RRpreassure , "%s" , "-.-");            
            M5.Lcd.fillScreen(TFT_BLACK);

        }
        M5.Lcd.print("Button B :");
        val=digitalRead(38);
        M5.Lcd.println(val);
        M5.Lcd.print("Button C :");
        val=digitalRead(37);
        M5.Lcd.println(val);

        if (val==0) {
            Serial.println("Disconnect");
            WiFi.softAPdisconnect(true);
            delay(1000);
            WiFi.mode(WIFI_AP);
            // configure device AP mode
            configDeviceAP();
            // This is the mac address of the Slave in AP Mode
            Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
            // Init ESPNow with a fallback logic
            InitESPNow();
            // Once ESPNow is successfully Init, we will register for recv CB to
            // get recv packer info.
            esp_now_register_recv_cb(OnDataRecv);
            LastReceiveTime ==0;
            iDisplayStatus == 0;            
            sprintf(FLpreassure , "%s" , "-.-");            
            sprintf(FRpreassure , "%s" , "-.-");            
            sprintf(RLpreassure , "%s" , "-.-");            
            sprintf(RRpreassure , "%s" , "-.-");            
            M5.Lcd.fillScreen(TFT_BLACK);

        }


    }
  }

//  if(millis()<32000 && millis()>31000) {
//   LastReceiveTime = millis(); 
//  }
//    
//  Serial.print("Time: "); Serial.println(millis());


}
