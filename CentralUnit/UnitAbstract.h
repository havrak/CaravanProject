// abstarct definition of sensor unit

#ifndef UNITABSTRACT_H
#define UNITABSTRACT_H
#include <esp_now.h>
#include <EEPROM.h>
#include <cstring>
using namespace std;

class UnitAbstract {
  public:
    // cant create final Data data due to unknown type of data in structure
    struct Data;
      
    // Data wchich will forward CentralUnit form Nextion display
    // struct NextionConfiguration;
      
    // Object will send its data to olimex 
    virtual void updateDataOnNextion() = 0;
    // Object will recive new data from main that origins from sensor unit
      
    virtual void updateYourData(const uint8_t *newData) = 0;
    // Object will convert scruct to uint8_t, body cannot be specified here due to unkown size at compiling
    virtual uint8_t getDataToBeSend() = 0;

    // unactive sensors will be deleted
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
#endif UNITABSTRACT_H
