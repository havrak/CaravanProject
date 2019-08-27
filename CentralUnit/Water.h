// Class which takes care of water system in caravan.

#ifndef WATER_H
#define WATER_H
#include "UnitAbstract.h"

// unknown state of tank is when liters = 1000

class Water : public UnitAbstract{
  public:
    
    struct Data{
      bool connectionToWaterSource;
      float litersRemaining;
      float temperature;
    };
    
    Data data;
    Water(){
      
    }
    
    void updateDataOnNextion(){
      String command;
      if(data.connectionToWaterSource){
        startEndNextionCommand(); 
        command= "textWater.txt=připojena";
        Serial.print(command);
        startEndNextionCommand(); 
        command= "imgWater.pic=14";
        Serial.print(command);
        startEndNextionCommand();
      }else{
        startEndNextionCommand(); 
        command= "textWater.txt=odpojena";
        Serial.print(command);
        startEndNextionCommand(); 
        command= "imgWater.pic=13";
        Serial.print(command);
        startEndNextionCommand(); 
      };
      if(data.litersRemaining != 1000){
        startEndNextionCommand(); 
        command= "textLiters.txt="+String(data.litersRemaining)+"l";
        Serial.print(command);
        startEndNextionCommand(); 
      }else{
        startEndNextionCommand(); 
        command= "textLiters.txt=neznámý";
        Serial.print(command);
        startEndNextionCommand(); 
      }
      startEndNextionCommand(); 
      command= "textWaterTemp.txt="+String(data.temperature)+"°C";
      Serial.print(command);
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
    
    void setEstablishedConnection(bool state){
      isEstablishedConnectionToUnit = state;
    }
  private:
    bool isEstablishedConnectionToUnit;
    
    void startEndNextionCommand(){
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);
    }
};
#endif WATER_H
