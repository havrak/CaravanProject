// Olimex takes care of it directly, so it cant be based on Unitabstract (method updateYourData)
// Will comunicate with mikrotik directly
// TODO: check telnet port

#ifndef CONNECTION_H
#define CONNECTION_H
#include "Olimex.h"
#include <WiFi.h>
#include <EEPROM.h>


class Connection{
  public:    
    //0 - OFF, 1 - LTE, 2 - AP
    byte typeOfConnection;
    double UpLink;
    // diffrent evaluation based on source AP, LTE
    double SignalStrenght;

    Connection(IPAddress ip){
      server = ip;
      if (client.connect(server, 23)) {
        isTelnetConnectionRunning = true;
      }
    }
    void updateDataOnOlimex(Olimex olimex){
      
    }
    // also will change configuration on mikrotik trought telnet
    void fetchNewConfigFromOlimex(){
      
    }
    // will do 
    void updateYourData(){
      
    };

    // geters for variables in scructure for testing purpouse 
    byte getTypeOfConnection(){
      return typeOfConnection;
    }
    double getUpLink(){
      return UpLink;  
    }
    double getSignalStrenght(){
      return SignalStrenght;
    }
    private: 
      WiFiClient client;
      // for some reason IP cannot be created here and needs to be passed from main
      IPAddress server;
      bool isTelnetConnectionRunning = false;
};
#endif CONNECTION_H
