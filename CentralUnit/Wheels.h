// Class takes care of wheel preasure.
// Not currently displayed on Nextion.
#ifndef WHEELS_H
#define WHEELS_H
#include "UnitAbstract.h"
class Wheels : public UnitAbstract {
  public:
    // edit later


    Wheels() {

    }

    void updateDataOnNextion() {
        String command;
        startEndNextionCommand(); 
        command = "textFLPreassure.txt=\"" + String(data.FLPreassure) + "kPa\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textFRPreassure.txt=\"" + String(data.FRPreassure) + "kPa\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textRLPreassure.txt=\"" + String(data.RLPreassure) + "kPa\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textRRPreassure.txt=\"" + String(data.RRPreassure) + "kPa\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textFLTemperature.txt=\"" + String(data.FLTemperature) + "°C\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textFRTemperature.txt=\"" + String(data.FRTemperature) + "°C\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textRLTemperature.txt=\"" + String(data.RLTemperature) + "°C\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textRRTemperature.txt=\"" + String(data.RRTemperature) + "°C\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textFLBatt.txt=\"" + String(data.FLBatt) + "%\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textFRBatt.txt=\"" + String(data.FRBatt) + "%\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textRLBatt.txt=\"" + String(data.RLBatt) + "%\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textRRBatt.txt=\"" + String(data.RRBatt) + "%\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textFLScanTime.txt=\"" + String(getTimeDiffrence(FLScanTime)) + "ms\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textFRScanTime.txt=\"" + String(getTimeDiffrence(RLScanTime)) + "ms\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textRLScanTime.txt=\"" + String(getTimeDiffrence(RLScanTime)) + "ms\"";
        Serial2.print(command);
        startEndNextionCommand(); 
        command = "textRRScanTime.txt=\"" + String(getTimeDiffrence(RRScanTime)) + "ms\"";
        Serial2.print(command);
        startEndNextionCommand(); 
    }
    // clone whole structure, must ensure that new config is sent to sensor before it sends its data to prevent missmatch across what is shown at nextion and what has sensor unit
    // check how flow works
    bool updateYourData(const uint8_t *newData) {
      if (sizeof(newData) != sizeof(data)) {
        memcpy(&data, newData, sizeof(data));
        FLScanTime = millis()-data.FLScanTime; 
        FRScanTime = millis()-data.FRScanTime;
        RLScanTime = millis()-data.RLScanTime;
        RRScanTime = millis()-data.RRScanTime;
        return true;
      }
      return false;
    };

    uint8_t getDataToBeSend() {
      uint8_t bs[sizeof(data)];
      memcpy(bs, &data, sizeof(data));
      return *bs;
    }

  private:
    struct Data {
      float FLPreassure;
      float FRPreassure;
      float RLPreassure;
      float RRPreassure;
      float FLTemperature;
      float FRTemperature;
      float RLTemperature;
      float RRTemperature;
      int  FLBatt;
      int  FRBatt;
      int  RLBatt;
      int  RRBatt;
      long LastScanTime;
      long FLScanTime;
      long FRScanTime;
      long RLScanTime;
      long RRScanTime;
    };
    Data data;
    long FLScanTime;
    long FRScanTime;
    long RLScanTime;
    long RRScanTime;
};
#endif
