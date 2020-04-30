// Class takes care of wheel preasure.
// Not currently displayed on Nextion.
#ifndef WHEELS_H
#define WHEELS_H
#include "UnitAbstract.h"
class Wheels : public UnitAbstract{
  public:
    // edit later


    Wheels(){
      
    }

    void updateDataOnNextion(){
      
    }
    // clone whole structure, must ensure that new config is sent to sensor before it sends its data to prevent missmatch across what is shown at nextion and what has sensor unit
    // check how flow works
    bool updateYourData(const uint8_t *newData){
      if(sizeof(newData) != sizeof(data)){
        memcpy(&data, newData, sizeof(data));
        return true;
      }
      return false;
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
#endif
