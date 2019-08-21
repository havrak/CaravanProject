// Olimex takes care of it directly, so it cant be based on Unitabsctracet (method updateYourData)
// On Nextion caravan image will be shown
// will use bluetooth libary

#ifndef TEMPERATURES_H
#define TEMPERATURES_H
#include <EEPROM.h>
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Temperatures{
  public:
    struct Data{

    };

    Data data;
    Temperatures(){
      
    }
    void updateData(){
      
    }
    void fetchNewConfigFromNextion(){
      
    }
    // will ask sensors directly not trought CentralUnit file, wil
    void updateYourData(){
      
    };

};
#endif TEMPERATURES_H
