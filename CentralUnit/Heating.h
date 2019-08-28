// Class that takes care of floor heating in caravan.
// TODO: set goal temperature from mikrotik.
// Stores average of temp from class temperatures, which is needed for checking if destined temperature was reached by heating


#ifndef HEATING_H
#define HEATING_H
#include "UnitAbstract.h"
 
class Heating : public UnitAbstract{
  public:
    


    Heating(){
      
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
      float sum;
      for(int i; i < (sizeof(data.temperaturesFloor)/sizeof(data.temperaturesFloor[0])); i++){
        sum+= data.temperaturesFloor[i];
      }
      sum = sum/(sizeof(data.temperaturesFloor)/sizeof(data.temperaturesFloor[0]));
      
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
  private:
    struct Data{
      // dont know exact number of heat sensors
      // can be in seperate variables
      float temperaturesFloor[6];
      bool isHeatingOn;
      float amperesMax;
     
    };
    Data data;
};
#endif HEATING_H
