// Class which takes care of water system in caravan.

#ifndef WATER_H
#define WATER_H
#include "UnitAbstract.h"

// unknown state of tank is when liters = 1000

class Water : public UnitAbstract{
  public:
    Water(){
      
    }
    
    void updateDataOnNextion(){
      String command;
      if(data.connectionToWaterSource){
        startEndNextionCommand(); 
        command= "textWater.txt=připojena";
        Serial2.print(command);
        startEndNextionCommand(); 
        command= "imgWater.pic=14";
        Serial2.print(command);
        startEndNextionCommand();
      }else{
        startEndNextionCommand(); 
        command= "textWater.txt=odpojena";
        Serial2.print(command);
        startEndNextionCommand(); 
        command= "imgWater.pic=13";
        Serial2.print(command);
        startEndNextionCommand(); 
      };
      startEndNextionCommand(); 
      switch(data.validityOfData){
        case 0:   
          command = "textLiters.txt=\""+String(data.litersRemaining)+"l\"";
          break;
        case 1: 
          command = "textLiters.txt=\""+String(data.litersRemaining)+"l?\"";
          break;
        case 2:
          command = "textLiters.txt=\"?EEPROM?\"";
          break;
        case 3:
          command = "textLiters.txt=\"?REFILL?\"";
          break;
      }
      Serial2.print(command);
      startEndNextionCommand(); 
      command= "textWaterTemp.txt="+String(data.temperature)+"°C";
      Serial2.print(command);
      startEndNextionCommand(); 
    }
    // clone whole structure, must ensure that new config is sent to sensor before it sends its data to prevent missmatch across what is shown at nextion and what has sensor unit
    // check how flow works
    void updateYourData(const uint8_t *newData){
      // newData << 32
      memcpy(&data, newData, sizeof(data));
    }
    
    uint8_t getDataToBeSend(){
      uint8_t bs[sizeof(data)]; 
      memcpy(bs, &data, sizeof(data));
      return *bs;
    }

    // geters for variables in scructure for testing purpouse 
    float getLitersRemaining(){
      return data.litersRemaining;
    }
    bool getConnectionToWaterSource(){
      return data.connectionToWaterSource;  
    }
    float getTemperature(){
      return data.temperature;
    }
    
  private:
    struct Data{
      bool connectionToWaterSource;
      byte validityOfData;
      float litersRemaining;
      float temperature;
    };
    Data data;
   
};
#endif WATER_H
