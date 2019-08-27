// Class that takes care of swithcing connection between LTE (when caravan is not parked at home) and using home WiFi.
// Switching is done trought telnet with mikrotik switch.
 
#ifndef CONNECTION_H
#define CONNECTION_H
#include <WiFi.h>
#include <EEPROM.h>


class Connection{
  public:
    bool isConnectionLTE;
    double Uplink;
    // diffrent evaluation based on source AP, LTE
    double SignalStrenght;
    
    Connection(){
      mikrotikIP = IPAddress(1,1,1,1);
      if (client.connect(server, 23)) {
        isTelnetConnectionRunning = true;
      }
    }
    void updateYourData(){
      if(!isTelnetConnectionRunning){
        if (client.connect(server, 23)) {
          isTelnetConnectionRunning = true;
          }
      } 
      // updates only uplink and and signal strenght
      if(isTelnetConnectionRunning){
        // checks if isConnectionLTE corresponds, if not change configuration imediately
        // here requests will be done  
      }

    }
    // also will change configuration on mikrotik trought telnet
    void updateDataOnNextion(){
      String command;
      if(isTelnetConnectionRunning){
        startEndNextionCommand(); 
        if(isConnectionLTE){
          command= "textConnection.txt=LTE";
        }else{
          command= "textConnection.txt=AP";
        }
        Serial.print(command);
        startEndNextionCommand(); 
        command= "textUplink.txt="+String(Uplink)+"Mb/s";
        Serial.print(command);
        startEndNextionCommand(); 
      }else{
        startEndNextionCommand(); 
        command = "textConnection.txt=No connection";
        Serial.print(command);
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
    private: 
      IPAddress mikrotikIP;
      WiFiClient client;
      // for some reason IP cannot be created here and needs to be passed from main
      IPAddress server;
      bool isTelnetConnectionRunning = false;
      
      void startEndNextionCommand(){
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);
      }
};
#endif CONNECTION_H
