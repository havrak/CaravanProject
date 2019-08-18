// Olinex takes care if it directly, so it cant be based on Unitabsctracet (method updateYourData)

#ifndef Connection_H
#define Connection_H
#include "Olimex.h"
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Connection{
  public:
    
    struct Data{
      //0 - AP, 1 - LTE 
      bool typeOfConnection;
      double UpLink;
      // diffrent evaluation based on source AP, LTE
      double SignalStrenght;
     
      
    };
    
    static Data data;
    Connection(){
      
    }
    void updateDataOnOlimex(Olimex olimex){
      
    }
    void fetchNewConfigFromOlimex(){
      
    }
    // clone whole structure, must ensure that new config is sent to sensor before it sends its data to prevent missmatch across what is shown at nextion and what has sensor unit
    // check how flow works
    void updateYourData(uint8_t newData);
    uint8_t getDataToBeSend(){
      uint8_t bs[sizeof(data)]; 
      memcpy(bs, &data, sizeof(data));
      return *bs;
    }

    // geters for variables in scructure for testing purpouse 
    double getLitersRemaining(){
      return litersRemaining;
    }
    boolean getConnectionToWaterSource(){
      return connectionToWaterSource;  
    }
    double getTemperature(){
      return temperature;
    }
};
#endif WHEELS_H
