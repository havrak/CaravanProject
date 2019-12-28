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
      Serial.print("WATER | updateDataOnNextion");
      String command;
      if(data.connectionToWaterSource){
        startEndNextionCommand(); 
        command= "textWater.txt=\"Connected\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command= "imgWater.pic=14";
        Serial2.print(command);
        startEndNextionCommand();
      }else{
        startEndNextionCommand(); 
        command= "textWater.txt=\"Disconnected\"";
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
    bool updateYourData(const uint8_t *newData){
      // newData << 32
      Serial.print("updateYourData Water | Recieved new data, size is: ");
      Serial.println(sizeof(newData));
      if(sizeof(newData) != sizeof(data)){
        memcpy(&data, newData, sizeof(data));
          Serial.print("WATER | updateYourData | connectionToWaterSource:  "); Serial.println(data.connectionToWaterSource);
          Serial.print("WATER | updateYourData | litersRemaining:          "); Serial.println(data.litersRemaining); 
          Serial.print("WATER | updateYourData | temperature:              "); Serial.println(data.temperature); 
          Serial.print("WATER | updateYourData | validityOfData:           "); Serial.println(data.validityOfData);
          Serial.print("WATER | updateYourData | heating;                  "); Serial.println(data.heating);
        return true;
      }
      return false;
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
      bool heating;
    };
    Data data;
   
};
#endif
