// Class takes care of wheel preasure.
// Is not currently displayed on Nextion.
#ifndef WHEELS_H
#define WHEELS_H
#include "UnitAbstract.h"
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Wheels : public UnitAbstract{
  public:
    // edit later


    Wheels(){
      
    }

    void updateDataOnNextion(){
      
    }
    // clone whole structure, must ensure that new config is sent to sensor before it sends its data to prevent missmatch across what is shown at nextion and what has sensor unit
    // check how flow works
    void updateYourData(const uint8_t *newData){
      memcpy(&data, newData, sizeof(data));
    };
    
    uint8_t getDataToBeSend(){
      uint8_t bs[sizeof(data)]; 
      memcpy(bs, &data, sizeof(data));
      return *bs;
    }
    
  private:
    struct Data{     
    };
    Data data;

};
#endif WHEELS_H
