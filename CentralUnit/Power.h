// Class that takes care of battery/power in caravan.
// User most likely wont be able to change anything so communication with chip will be one way.


#ifndef POWER_H
#define POWER_H
#include "UnitAbstract.h"

class Power : public UnitAbstract{
  public:
    
    struct Data{
      bool connectionToPowerOutlet;
      double batteryState;
      double currentDrawn;
      bool charging;
     
    };

    Data data;
    Power(){
      
    }
    void updateDataOnNextion(){
      String command;
      // green plug, charging battery icon (even when charged)
      if(data.connectionToPowerOutlet){
        startEndNextionCommand(); 
        command= "textPower.txt=Připojena";
        Serial.print(command);
        startEndNextionCommand(); 
        command= "imgPower.pic=9";
        Serial.print(command);
        startEndNextionCommand();
        command= "imgBattery.pic=6";
        Serial.print(command);
        startEndNextionCommand();
      }else{
        startEndNextionCommand(); 
        command= "textPower.txt=Opojena";
        Serial.print(command);
        startEndNextionCommand(); 
        command= "imgPower.pic=8";
        Serial.print(command);
        startEndNextionCommand();
        command= "imgBattery.pic=5";
        Serial.print(command);
        startEndNextionCommand();
      }
      command= "textBattery.pic="+String(data.batteryState)+"%";
      Serial.print(command);
      startEndNextionCommand();
      command= "textDrawn.pic="+String(data.currentDrawn)+"A";
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
    bool getConnectionToPowerOutlet(){
      return data.connectionToPowerOutlet;
    }
    double getBatteryState(){
      return data.batteryState;  
    }
    double getCurrentDrawn(){
      return data.currentDrawn;
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
#endif POWER_H
