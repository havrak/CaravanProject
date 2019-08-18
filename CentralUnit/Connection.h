// Olinex takes care if it directly, so it cant be based on Unitabsctracet (method updateYourData)

#ifndef CONNECTION_H
#define CONNECTION_H
#include "Olimex.h"
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Connection{
  public:
    struct Data{
      // device will remember its last configuration, object will recive it from unit after it is swithched on
      //0 - OFF, 1 - LTE, 2 - AP
      byte typeOfConnection;
      double UpLink;
      // diffrent evaluation based on source AP, LTE
      double SignalStrenght;

      // on off 
      bool stateOfDevice;
      
    };
    
    Data data;
    Connection(){
      
    }
    void updateDataOnOlimex(Olimex olimex){
      
    }
    void fetchNewConfigFromOlimex(){
      
    }
    // clone whole structure, must ensure that new config is sent to sensor before it sends its data to prevent missmatch across what is shown at nextion and what has sensor unit
    // check how flow works
    void updateYourData(uint8_t newData){
      
    };
    
    uint8_t getDataToBeSend(){
      uint8_t bs[sizeof(data)]; 
      memcpy(bs, &data, sizeof(data));
      return *bs;
    }

    // geters for variables in scructure for testing purpouse 
    byte getTypeOfConnection(){
      return data.typeOfConnection;
    }
    double getUpLink(){
      return data.UpLink;  
    }
    double getSignalStrenght(){
      return data.SignalStrenght;
    }
    bool getStateOfDevice(){
      return data.stateOfDevice;
    }
};
#endif CONNECTION_H
