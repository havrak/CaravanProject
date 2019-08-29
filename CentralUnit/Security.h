// Clas that takes care of GPS and accelorometer.
// Prevents caravan from being stolen.
// gps coordinates will be forwarded to weather.

#ifndef SECURITY_H
#define SECURITY_H
#include "UnitAbstract.h"
// all calculation of water levels will be done on sensor unit
// first portion of structure are data from unit, second is configuration (seperated by space)
class Security : public UnitAbstract{
  public:
    

    Security(){
      
    }

    void updateDataOnNextion(){
      String command;
      if(data.state){
        startEndNextionCommand(); 
        command= "textGPSState.txt=\"Zapnuto\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command= "imgGPSState.pic=4";
        Serial2.print(command);
        startEndNextionCommand();
        command= "textNOS.txt=\""+String(data.numberOfSatellites)+"ks\"";
        Serial2.print(command);
        startEndNextionCommand();
        command= "textAccuracy.txt=\""+String(data.accuraccy)+"\"";
        Serial2.print(command);
        startEndNextionCommand();
      }else{
        startEndNextionCommand(); 
        command= "textGPSState.txt=\"Vypnuto\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command= "imgGPSState.pic=10";
        Serial2.print(command);
        startEndNextionCommand();
        command= "textNOS.txt=0";
        Serial2.print(command);
        startEndNextionCommand();
        command= "textAccuracy.txt=\"0\"";
        Serial2.print(command);
        startEndNextionCommand();    
      }  
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

    // geters for variables in scructure for testing purpouse 
    bool getState(){
      return data.state;
    }
    byte getNumberOfSatellites(){
      return data.numberOfSatellites;  
    }
    double getAccuraccy(){
      return data.accuraccy;
    }
    float getLatitude(){
      return data.latitude;
      
    }
    float getLongitude(){
      return data.longitude;  
    }
  private:
    struct Data{
      bool state;
      byte numberOfSatellites;
      float latitude;
      float longitude;
      float accuraccy;
    };
    Data data;
};
#endif SECURITY_H
