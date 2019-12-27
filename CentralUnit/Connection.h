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
      mikrotikIP = IPAddress(192,168,1,4);
      //if (client.connect(server, 23)) { // cant connect here
      //  Serial.print("connected");
      //  isTelnetConnectionRunning = true;
      //}
    }
    // presses button -> callbacks calls this function -> if it is successfull changes icon on nextion
    bool changeConnection(){
      if(!isTelnetConnectionRunning){
        Serial.print("connected");
        if (client.connect(server, 23)) {
            isTelnetConnectionRunning = true;
            return false;
          }
      } 
      if(isTelnetConnectionRunning){
        if(didIAuthorized){
          didIAuthorized = true;
          // calls function for command
          return true;
        }else{
          // authorizes and calls function for commnad
          return true;
        }
      }
      return false;  
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

    void reset(){
      isTelnetConnectionRunning = false;
      didIAuthorized = false;
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
      WiFiClient client; // possible this causes the crash
      IPAddress server;
      
      bool isTelnetConnectionRunning = false;
      bool didIAuthorized = false;
      
      void startEndNextionCommand(){
        Serial2.write(0xff);
        Serial2.write(0xff);
        Serial2.write(0xff);
      }
};
#endif
