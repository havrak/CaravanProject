// Take care of heat sensors all around carava, these sensors will communicate with olimex directly so it isn't based on UnitAbstract.

#ifndef TEMPERATURES_H
#define TEMPERATURES_H
#include <EEPROM.h>
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Temperatures{
  public:

    Temperatures(){
      
    }
    void updateData(){
      
    }
    void fetchNewConfigFromNextion(){
      
    }
    // will ask sensors directly not trought CentralUnit file, wil
    void updateYourData(){

    };
  private:
    void startEndNextionCommand(){
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);
    }

};

#endif TEMPERATURES_H
