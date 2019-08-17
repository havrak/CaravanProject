// will be class with definition of all units

#ifndef WATER_H
#define WATER_H
#include "UnitAbstract.h"
#include <esp_now.h>
#include "Olimex.h"
//
class Water : public UnitAbstract{
  public:
    struct Data{
      int i;
      int b;
    };
    static Data data;
    Water();
    void updataDataOnOlimex(Olimex olimex);
    void fetchNewConfigFromOlimex();
    void updateYourData(uint8_t newData);
    uint8_t getDataToBeSend();

  // getry
};
#endif WATER_H
