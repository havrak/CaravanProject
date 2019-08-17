#include "Water.h"
#include <cstring>
#include "UnitAbstract.h"
using namespace std;

struct test{
  int a;
  int b; 
};

test t;
Water::Water(){
  
}

void Water::updataDataOnOlimex(Olimex olimex){
  
}

void Water::fetchNewConfigFromOlimex(){
  
}

void Water::updateYourData(uint8_t){
  
}

uint8_t Water::getDataToBeSend(){
  uint8_t bs[sizeof(data)]; 
  memcpy(bs, &data, sizeof(data));
  return *bs;    
}
