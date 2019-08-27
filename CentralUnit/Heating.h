// Class that takes care of floor heating in caravan.
// TODO: set goal temperature from mikrotik.

#ifndef HEATING_H
#define HEATING_H
#include "UnitAbstract.h"
 
class Heating : public UnitAbstract{
  public:
    
    struct Data{
      // dont know exact number of heat sensors
      // can be in seperate variables
      float temperaturesFloor[6];
      bool isHeatingOn;
      float amperesMax;
     
    };

    Data data;
    Heating(){
      
    }
    void updateYourData(){
      
    }
    void updateDataOnNextion(){
      String command;
      if(data.isHeatingOn){
        startEndNextionCommand(); 
        command= "textHeating.txt=topí se";
        Serial.print(command);
        startEndNextionCommand(); 
        command= "imgHeating.pic=4";
        Serial.print(command);
        startEndNextionCommand();
        command= "textAmperes.txt="+String(data.amperesMax)+"A";
        Serial.print(command);
        startEndNextionCommand();
      }else{
        startEndNextionCommand(); 
        command= "textHeating.txt=netopí se";
        Serial.print(command);
        startEndNextionCommand(); 
        command= "imgHeating.pic=10";
        Serial.print(command);
        startEndNextionCommand();
        command= "textAmperes.txt=0A";
        Serial.print(command);
        startEndNextionCommand(); 
      };
      // calc average of floors temp
      startEndNextionCommand(); 
      double sum;
      for(int i; i < 6; i++){
        sum+= data.temperaturesFloor[i];
      }
      sum = sum/6;
      
      command= "textFloorTemp.txt="+String(sum)+"°C";
      Serial.print(command);
      startEndNextionCommand(); 
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

    // geters for variables in scructure for testing purpouse 
    bool getIsHeatingOn(){
      return data.isHeatingOn;  
    }
    float getAmperesMax(){
      return data.amperesMax;
    }

    float* getTemperaturesFloor(){
      return data.temperaturesFloor;  
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
#endif HEATING_H
