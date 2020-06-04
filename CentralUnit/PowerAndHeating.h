// Class that takes care of battery/power in caravan.
// User most likely wont be able to change anything so communication with chip will be one way.


#ifndef POWERANDHEATING_H
#define POWERANDHEATING_H
#include "UnitAbstract.h"

class PowerAndHeating : public UnitAbstract {
  public:
    PowerAndHeating() {

    }

    void updateDataOnNextion() {
      String command;
      // green plug, charging battery icon (even when charged)
      if (data.bMasterRelayOn) {
        startEndNextionCommand();
        command = "textPower.txt=\"Connected\"";
        Serial2.print(command);
        startEndNextionCommand();
        command = "imgPower.pic=9";
        Serial2.print(command);
        startEndNextionCommand();
        command = "imgBattery.pic=6";
        Serial2.print(command);
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        command = "textPower.txt=\"Disconnected\"";
        Serial2.print(command);
        startEndNextionCommand();
        command = "imgPower.pic=8";
        Serial2.print(command);
        startEndNextionCommand();
        command = "imgBattery.pic=5";
        Serial2.print(command);
        startEndNextionCommand();
      }
      if (data.bLowTemperature) {
        startEndNextionCommand();
        command = "textHeating.txt=\"ON\"";
        Serial2.print(command);
        startEndNextionCommand();
        command = "imgHeating.pic=4";
        Serial2.print(command);
        startEndNextionCommand();
        command = "textAmperes.txt=\"" + String(data.dConsumption) + "A\"";
        Serial2.print(command);
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        command = "textHeating.txt=\"OFF\"";
        Serial2.print(command);
        startEndNextionCommand();
        command = "imgHeating.pic=10";
        Serial2.print(command);
        startEndNextionCommand();
        command = "textAmperes.txt=\"0A\"";
        Serial2.print(command);
        startEndNextionCommand();
      };
      command = "textBattery.txt=\"" + String(data.batteryState) + "%\"";
      Serial2.print(command);
      startEndNextionCommand();
      command = "textDrawn.txt=\"" + String(data.currentDraw) + "A\"";
      Serial2.print(command);
      startEndNextionCommand();
      command = "floorTempInfo.txt=\"number of heating cycles: " + String(data.iHeatingCircuits) + "\r"
      + "name bus 1" + String(tempSensorsBUS1_Name[0]) +" " + String(tempSensorsBUS1_Name[1]) + " " + String(tempSensorsBUS1_Name[2]) + " " + String(tempSensorsBUS1_Name[3]) + "\r"
      + "temp bus 1" + String(data.CurrentTempBUS1[0]) +" " + String(data.CurrentTempBUS1[1]) + " " + String(data.CurrentTempBUS1[2]) + " " + String(data.CurrentTempBUS1[3]) + "\r"
      + "name bus 1" + String(tempSensorsBUS2_Name[0]) +" " + String(tempSensorsBUS2_Name[1]) + " " + String(tempSensorsBUS2_Name[2]) + "\r"
      + "temp bus 1" + String(data.CurrentTempBUS2[0]) +" " + String(data.CurrentTempBUS2[1]) + " " + String(data.CurrentTempBUS2[2]) + "\r"
      + "heating: " + String(data.bLowTemperature) + "\r"
      + "cycle state heating: "  + String(data.bCircuitLowTemperature[0]) +" " + String(data.bCircuitLowTemperature[1]) + " " + String(data.bCircuitLowTemperature[2]) + "\r"
      + "cycle state support: "  + String(data.bCircuitLowSupportTemperature[0]) +" " + String(data.bCircuitLowSupportTemperature[1]) + " " + String(data.bCircuitLowSupportTemperature[2]) + "\r"
      + "consumption: " + String(data.dConsumption) + "\r"
      + "masterRealy: " + String(data.bMasterRelayOn) + "\r"
      + "\"";
      Serial2.print(command);
      startEndNextionCommand();
    }
    // clone whole structure, must ensure that new config is sent to sensor before it sends its data to prevent missmatch across what is shown at nextion and what has sensor unit
    // check how flow works
    bool updateYourData(const uint8_t *newData) {
      if (sizeof(newData) != sizeof(data)) {
        memcpy(&data, newData, sizeof(data));
        return true;
      }
      return false;

    };

    uint8_t getDataToBeSend() {
      uint8_t bs[sizeof(sendConf)];
      memcpy(bs, &sendConf, sizeof(sendConf));
      return *bs;
    }

    // geters for variables in scructure for testing purpouse
    bool getConnectionToPowerOutlet() {
      return data.bMasterRelayOn;
    }
    double getBatteryState() {
      return data.batteryState;
    }
    double getCurrentDraw() {
      return data.currentDraw;
    }

    bool setUpSendConf(bool heating, bool winter, bool cycle1, bool cycle2, bool cycle3, bool cycle4, int airTemp, int airTempTol, int floorTempMax, int limitFloorTemp) {
      if (sendConf.heating == heating &&
          sendConf.winter == winter &&
          sendConf.cycle1 == cycle1 &&
          sendConf.cycle2 == cycle2 &&
          sendConf.cycle3 == cycle3 &&
          sendConf.cycle4 == cycle4 &&
          sendConf.airTemp == airTemp &&
          sendConf.airTempTol == airTempTol &&
          sendConf.floorTempMax == floorTempMax &&
          sendConf.limitFloorTemp == limitFloorTemp)
        return false;

      sendConf.heating = heating;
      sendConf.winter = winter;
      sendConf.cycle1 = cycle1;
      sendConf.cycle2 = cycle2;
      sendConf.cycle3 = cycle3;
      sendConf.cycle4 = cycle4;
      sendConf.airTemp = airTemp;
      sendConf.airTempTol = airTempTol;
      sendConf.floorTempMax = floorTempMax;
      sendConf.limitFloorTemp = limitFloorTemp;
      return true;
    }

  private:

    struct SendConf {
      bool heating;
      bool winter;
      bool cycle1;
      bool cycle2;
      bool cycle3;
      bool cycle4;
      int airTemp;
      int airTempTol;
      int floorTempMax;
      int limitFloorTemp;
    };

    SendConf sendConf;

    struct Data {
      double batteryState;
      double currentDraw;
      bool charging;

      int iHeatingCircuits;
      float CurrentTempBUS1[4];
      int CurrentStatusBUS1[4];
      int iExternSensors = 3;
      float CurrentTempBUS2[3];
      boolean bLowTemperature;
      boolean bCircuitLowTemperature[3];
      boolean bCircuitLowSupportTemperature[3];
      double dConsumption;
      bool bMasterRelayOn;
    };

    double floorTempAverage = 0;
    double airTempAverage = 0;
    Data data;

    String tempSensorsBUS1_Name[4] = {
      "Back_290W",
      "Middle_400W",
      "Front_350W",
      "Reserve    "
    };

    String tempSensorsBUS2_Name[3] = {
      "OutSide",
      "Top Back",
      "Middle"
    };
};
#endif
