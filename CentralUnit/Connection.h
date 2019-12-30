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
    EthernetClient client; // possible this causes the crash
    IPAddress server;
    
    String password = "heslo";
    String commands;
    String prompt;
    String sbuffer;
    String MikroTikPrompt = "[admin@MikroTik] > ";
    bool loggedIn = false;
      
    bool isTelnetConnectionRunning = false;
    bool didIAuthorized = false;
     
    void startEndNextionCommand(){
      Serial2.write(0xff);
      Serial2.write(0xff);
      Serial2.write(0xff);
    }  
  
  
    
    void setUpVariables(){
      if (client.available() > 0) {
        char c = client.read();
        client.read();
        if (c < 32 || c > 128 ) {
          prompt = "";
          if (c==13 && loggedIn){
            sbuffer.replace("[9999B", "");
            Serial.println(sbuffer);
            sbuffer = "";
          }
        } else{    
          prompt+=c;
          if (loggedIn) sbuffer+=c;
          //Serial.print(prompt);
        }
        commands+="0x" + String(c,HEX)+" ";
        //Serial.println(commands);
        //Serial.println(prompt);
        //Serial.println(loggedIn);
      }
    }
    bool authorize(){
      if (client.connect(server, 23)) { // cant connect here
        Serial.print("connected");
      }
      setUpVariables();
      if (commands == "0xff 0xfd 0x18 0xff 0xfd 0x20 0xff 0xfd 0x23 0xff 0xfd 0x27 ") {
        Serial.println();
        Serial.println("CO | authorize | Received Phrase 1");
        byte buf[] = {255, 251, 24, 255, 251, 31};
        client.write(buf, sizeof(buf));
        commands = "";
      }
      setUpVariables();
      if (commands == "0xff 0xfd 0x1f ") {
        Serial.println();
        Serial.println("CO | authorize | Received Phrase 2");
        byte buf[] = {255, 252, 32, 255, 252, 35,255,252,39};
        client.write(buf, sizeof(buf));
        commands = "";
      }
      setUpVariables();
      if (commands == "0xff 0xfa 0x18 0x1 0xff 0xf0 ") {
        Serial.println();
        Serial.println("CO | authorize | Received Phrase 3");
        byte buf[] = {255,250,31,0,120,0,30,255,240};
        client.write(buf, sizeof(buf));
        byte buf2[] = {255,250,39,0,255,240,255,250,24,0,65,78,83,73,255,240};
        client.write(buf2, sizeof(buf2));
        commands = "";
      } 
      setUpVariables();
      if (prompt == "Login: "){
        Serial.println();
        Serial.println("CO | authorize | Login!!!");
        client.println("admin+tc");
        commands = "";
        prompt = "";  
      }
      setUpVariables();
      if (prompt == "Password:"){
        Serial.println();
        Serial.println("CO | authorize | Password!!!");
        client.println(password);
        loggedIn = true;
        commands = "";
        prompt = "";  
        return true;
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
        loggedIn = false;  
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
    
    // presses button -> callbacks calls this function -> if it is successfull changes icon on nextion
    bool changeConnection(){
      if(authorize()){
        setUpVariables();
        if (MikroTikPrompt.substring(0,prompt.length()) == prompt) {
          if ( MikroTikPrompt == prompt ) {
            String command;
            if (isConnectionLTE = !isConnectionLTE){ 
              Serial.println("CO | changeConnection | Connection is LTE");
              //client.println("/interface ethernet poe set ether2 poe-out=force");
              client.println("/interface ethernet poe monitor ether4 once");
              command="textConnection.txt=\"LTE\"";
            }
            else{ 
              Serial.println("CO | changeConnection | Connection is AP");
              command="textConnection.txt=\"AP\"";
              //client.println("/interface ethernet poe set ether3 poe-out=force");
              client.println("/interface ethernet poe set ether4 poe-out=force");
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
