// Take care of heat sensors all around carava, these sensors will communicate with olimex directly so it isn't based on UnitAbstract.
// also will update data on second page


#ifndef TEMPERATURES_H
#define TEMPERATURES_H
#include <EEPROM.h>
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Temperatures{
  public:
    float temperatures[];
    float average;
    
    Temperatures(){
      
    }
    void updateDataOnNextion(){
      String command;
      startEndNextionCommand();
      command= "textAirTemp.txt=\""+String(average)+"°C\"";
      Serial2.print(command);
      startEndNextionCommand(); 
    }
    // will ask sensors directly not trought CentralUnit file, wil
    void updateYourData(){
      average = 0;
      for(int i; i < (sizeof(temperatures)/sizeof(temperatures[0])); i++){
        average+= temperatures[i];
      }
      average = average/(sizeof(temperatures)/sizeof(temperatures[0]));
      
    };

    float getAverageOfAirTemp(){
      
      
    }
  private:
    void startEndNextionCommand(){
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);
    }

};

#endif TEMPERATURES_H
