// on olimex page with caravan picture

#ifndef HEATING_H
#define HEATING_H
#include "UnitAbstract.h"
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Security : public UnitAbstract{
  public:
    
    struct Data{
      // dont know exact number of heat sensors
      double temperatures[6];
      bool isHeatingOn;
      double amperesMax;
     
      
    };
    
    static Data data;
    Security(){
      
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
#endif HEATING_H
