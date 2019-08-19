#ifndef WHEELS_H
#define WHEELS_H
#include "UnitAbstract.h"
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Wheels : public UnitAbstract{
  public:
    // edit later
    struct Data{
      bool connectionToWaterSource;
      double litersRemaining;
      double temperature;
     
      
    };
    Data data;
    
    Wheels(){
      
    }
    void updateDataOnNextion(NextionObject nextion){
      
    }
    void fetchNewConfigFromNextion(){
      
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
    /*
    double getLitersRemaining(){
      return data.litersRemaining;
    }
    boolean getConnectionToWaterSource(){
      return data.connectionToWaterSource;  
    }
    double getTemperature(){
      return data.temperature;
    }
    */
};
#endif WHEELS_H
