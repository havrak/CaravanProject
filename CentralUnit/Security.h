// Clas that takes care of GPS and accelorometer.
// Prevents caravan from being stolen.
// gps coordinates will be forwarded to weather.

#ifndef SECURITY_H
#define SECURITY_H
#include "UnitAbstract.h"
class Security : public UnitAbstract{
  public:
    

    Security(){
      
    }

    void updateDataOnNextion(){
      String command;
      if(isPositionKnown){
        startEndNextionCommand(); 
        command= "textGPState.txt=\"ON\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command= "imgGPSState.pic=4";
        Serial2.print(command);
        startEndNextionCommand();
        command= "textNOS.txt=\""+String(data.numberOfSatellites)+"ks\"";
        Serial2.print(command);
        startEndNextionCommand();
        command= "textAccuraccy.txt=\""+String(((double) data.hdop)/100.0)+"\"";
        Serial2.print(command);
        startEndNextionCommand();
      }else{
        startEndNextionCommand(); 
        command= "textGPState.txt=\"OFF\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command= "imgGPSState.pic=10";
        Serial2.print(command);
        startEndNextionCommand();
        command= "textNOS.txt=\"0\"";
        Serial2.print(command);
        startEndNextionCommand();
        command= "textAccuraccy.txt=\"0\"";
        Serial2.print(command);
        startEndNextionCommand();    
      }  
    }
    
    // clone whole structure, must ensure that new config is sent to sensor before it sends its data to prevent missmatch across what is shown at nextion and what has sensor unit
    // check how flow works
    bool updateYourData(const uint8_t *newData){
      if(sizeof(newData) != sizeof(data)){
        memcpy(&data, newData, sizeof(data));
        Serial.print("SECURITY | updateYourData | isPositionKnown:          "); Serial.println(isPositionKnown);
        Serial.print("SECURITY | updateYourData | numberOfSatellites:       "); Serial.println(data.numberOfSatellites); 
        Serial.print("SECURITY | updateYourData | latitude:                 "); Serial.println(data.latitude); 
        Serial.print("SECURITY | updateYourData | longitude:                "); Serial.println(data.longitude);
        Serial.print("SECURITY | updateYourData | hdop:                     "); Serial.println(data.hdop);
        if(data.numberOfSatellites >=3) isPositionKnown = true;
        updateDataOnNextion();
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
    byte getNumberOfSatellites(){
      return data.numberOfSatellites;  
    }
    int32_t getHdop(){
      return data.hdop;
    }
    float getLatitude(){
      return data.latitude;
      
    }
    float getLongitude(){
      return data.longitude;  
    }
    bool getIsPositionKnown(){
      return isPositionKnown;  
    }
    
    // return time from GPS, is adjusted from lastTimeRecieved
    time_t getTime(){
      return data.time+ ((int) getTimeDiffrence(lastTimeRecived)/1000);
    }
  private:
    struct Data{
      uint32_t numberOfSatellites;
      double latitude;
      double longitude;
      int32_t  hdop;
      time_t time;
    };
    boolean isPositionKnown = false;
    Data data;
};
#endif
