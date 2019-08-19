// abstarct definition of unit

#ifndef UNITABSTRACT_H
#define UNITABSTRACT_H
#include <esp_now.h>
#include "NextionObject.h"
#include <EEPROM.h>
#include <cstring>
using namespace std;

class UnitAbstract {
   public:
      // cant create final Data data due to unknown type of data in structure
      struct Data;
      // Object will send its data to olimex 
      virtual void updateDataOnNextion(NextionObject nextion) = 0;
      // Object will fetch config from Olimex
      virtual void fetchNewConfigFromNextion() = 0;
      // Object will recive new data from main that origins from sensor unit
      
      virtual void updateYourData(uint8_t newData) = 0;
      // Object will convert scruct to uint8_t, body cannot be specified here due to unkown size at compiling
      virtual uint8_t getDataToBeSend() = 0;
};
#endif UNITABSTRACT_H
