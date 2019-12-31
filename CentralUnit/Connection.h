// Class that takes care of swithcing connection between LTE (when caravan is not parked at home) and using home WiFi.
// Switching is done trought telnet with mikrotik switch.
 
#ifndef CONNECTION_H
#define CONNECTION_H
#include <WiFi.h>
#include <EEPROM.h>
#include <Ethernet2.h>


class Connection{
  private: 
    IPAddress mikrotikIP;
    EthernetClient client;
    
    String commands;
    String prompt;
    String sbuffer;
    String MikroTikPrompt = "[admin@MikroTik] > ";
    int  loggedIn = 0;
      
    bool isTelnetConnectionRunning = false;
    bool didIAuthorized = false;
     
    void startEndNextionCommand(){
      Serial2.write(0xff);
      Serial2.write(0xff);
      Serial2.write(0xff);
    }  

    // millis() counter resets every 50 days, gives time diffrence between millis() and sTime in argument
    unsigned long getTimeDiffrence(unsigned long sTime){
      if(millis() < sTime){
        return (ULONG_MAX - sTime) + millis();
      }
      return millis() - sTime;
    }
    
    
    void setUpVariables(){
      if (client.available() > 0) {
        char c = client.read();
        if (c < 32 || c > 128 ) {
          prompt = "";
          if (c==13 && loggedIn==1){
            sbuffer.replace("[9999B", "");
            Serial.println(sbuffer);
            sbuffer = "";
          }
          } else
          {    
          prompt+=c;
          if (loggedIn == 1) sbuffer+=c;
          //Serial.print(prompt);
        }
        commands+="0x" + String(c,HEX)+" ";
    //Serial.println(commands);
    //Serial.println(prompt);
    //Serial.println(loggedIn);
      }
    }

    unsigned long timeout = 0;
    bool authorize(){
      disconnect();
      Serial.println("CO | authorize");
      if (client.connect(mikrotikIP, 23)) {
        Serial.println("CO | authorize | connected");
      }else{
        Serial.println("CO | authorize | connection failed");
      }
      timeout = millis();
      while(getTimeDiffrence(timeout) < 5000){
        
        setUpVariables();
        //Serial.println(commands);
        if (commands == "0xff 0xfd 0x18 0xff 0xfd 0x20 0xff 0xfd 0x23 0xff 0xfd 0x27 ") {
          Serial.println();
          Serial.println("CO | authorize | Received Phrase 1");
          byte buf[] = {255, 251, 24, 255, 251, 31};
          client.write(buf, sizeof(buf));
          commands = "";
        }
        if (commands == "0xff 0xfd 0x1f ") {
          Serial.println();
          Serial.println("CO | authorize | Received Phrase 2");
          byte buf[] = {255, 252, 32, 255, 252, 35,255,252,39};
          client.write(buf, sizeof(buf));
          commands = "";
        }
        if (commands == "0xff 0xfa 0x18 0x1 0xff 0xf0 ") {
          Serial.println();
          Serial.println("CO | authorize | Received Phrase 3");
          byte buf[] = {255,250,31,0,120,0,30,255,240};
          client.write(buf, sizeof(buf));
          byte buf2[] = {255,250,39,0,255,240,255,250,24,0,65,78,83,73,255,240};
          client.write(buf2, sizeof(buf2));
          commands = "";
        }
        if (commands == "0xff 0xfb 0x3 0xff 0xfd 0x1 0xff 0xfb 0x5 0xff 0xfd 0x21 ") {
          Serial.println();
          Serial.println("CO | authorize | Received Phrase 4");
          byte buf[] = {255,250};
          client.write(buf, sizeof(buf));
          byte buf2[] = {255,252,1,255,254,5,255,252,33};
          client.write(buf2, sizeof(buf2));
          commands = "";
        }
        if (prompt == "Login: "){
          Serial.println();
          Serial.println("CO | authorize | Login!!!");
          client.println("admin+tc");
          commands = "";
          prompt = "";  
        }
        if (prompt == "Password:"){
          Serial.println();
          Serial.println("CO | authorize | Password!!!");
          client.println("Sboj3169a");
          loggedIn = 1;
          commands = "";
          prompt = "";  
          return true;
        }
      }     
      return false;
    }

    // will terminate connection with mikrotik
    void disconnect(){
      if(loggedIn){
        Serial.println();
        Serial.println("exit!!!");
        client.println("");
        client.println("quit");
        commands = "";
        prompt = "";
        sbuffer = "";
        loggedIn = 0;  
      }  
      if (!client.connected()) {
        M5.Lcd.println();
        M5.Lcd.println("disconnecting.");
        client.stop();    
      }
    }
    
  public:
    bool isConnectionLTE;
    bool inConnectionKnown = false;
    double Uplink;
    // diffrent evaluation based on source AP, LTE
    double SignalStrenght;
    
    Connection(){
      mikrotikIP = IPAddress(10, 18, 11, 240);
    }

    // sets color of "Pripojeni" to yellow
    void displayUnknownState(){
      if(!inConnectionKnown){  
        startEndNextionCommand(); 
        String command= "t22.pco=65504";
        Serial2.print(command);
        startEndNextionCommand(); 
      }
    }
    
    // return true if connection is LTE
    int getStateOfConnection(){
      commands = "";
      prompt = "";
      sbuffer = "";
      loggedIn = false; 
      Serial.println("CO | getStateOfConnection");
      //client.println("/interface ethernet poe monitor ether4 once");
      if(authorize()){
        bool sendCommad = false;
        timeout = millis();
        Serial.println("CO | getStateOfConnection | authorized");
        while(getTimeDiffrence(timeout) < 5000){
          setUpVariables();
          prompt.replace("[9999B", "");
          if (MikroTikPrompt.substring(0,prompt.length()) == prompt) {
            if (MikroTikPrompt == prompt && !sendCommad) {
              sendCommad = true;
              Serial.println("Got prompt");
              client.println("/interface ethernet poe monitor ether4 once");
            }
          }
          sbuffer.trim();
          if(sbuffer.indexOf("poe-out:forced-on") == 0){
            Serial.println("Forced On");  
            return 1;
          }else if(sbuffer.indexOf("poe-out:forced-off") == 0){
            return 0;
          }
        }
        disconnect();
      }
      client.stop();
      return -1;
    }

    
    // presses button -> callbacks calls this function -> if it is successfull changes icon on nextion
    bool changeConnection(){
      commands = "";
      prompt = "";
      sbuffer = "";
      loggedIn = false;
      Serial.println("CO | changeConnection");
      if(authorize()){
        setUpVariables();
        if (MikroTikPrompt.substring(0,prompt.length()) == prompt) {
          if ( MikroTikPrompt == prompt ) {
            String command;
            if (isConnectionLTE = !isConnectionLTE){ 
              Serial.println("CO | changeConnection | Connection is LTE");
              //client.println("/interface ethernet poe set ether4 poe-out=off");
              //client.println("/interface ethernet poe monitor ether4 once");
              command="textConnection.txt=\"LTE\"";
            }
            else{ 
              Serial.println("CO | changeConnection | Connection is AP");
              command="textConnection.txt=\"AP\"";
              //client.println("/interface ethernet poe set ether3 poe-out=force");
              //client.println("/interface ethernet poe set ether4 poe-out=force");
            }
            inConnectionKnown = true;
            startEndNextionCommand();
            Serial2.print(command);
            startEndNextionCommand();
            command= "t22.pco=65535";
            Serial2.print(command);
            startEndNextionCommand(); 
          }
        }
        client.stop();
        disconnect();
      }
    }
    
    // also will change configuration on mikrotik trought telnet
    void updateDataOnNextion(){
      String command;
      if(isTelnetConnectionRunning){
        startEndNextionCommand(); 
        if(isConnectionLTE){
          command= "textConnection.txt=\"LTE\"";
        }else{
          command= "textConnection.txt=\"AP\"";
        }
        Serial2.print(command);
        startEndNextionCommand(); 
        command= "textState.txt=\""+String(Uplink)+"Mb/s\"";
        Serial2.print(command);
        startEndNextionCommand(); 
      }else{
        startEndNextionCommand(); 
        command = "textConnection.txt=\"No connection\"";
        Serial2.print(command);
        startEndNextionCommand(); 
      };
      
    }
    
    bool getIsConnectionLTE(){
      return isConnectionLTE;
    }
    
    void setIsConnectionLTE(bool newState){
      isConnectionLTE= newState;
    }
      
    double getUplink(){
      return Uplink;  
    }
    double getSignalStrenght(){
      return SignalStrenght;
    }
  
};
#endif
