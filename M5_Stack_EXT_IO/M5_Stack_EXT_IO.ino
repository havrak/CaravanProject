#include <M5Stack.h>
#include "PCA9554.h"  // Load the PCA9554 Library
#include "ClosedCube_TCA9548A.h"
#include "ADS1100.h"


#define PaHub_I2C_ADDRESS  0x70

ClosedCube::Wired::TCA9548A tca9548a;

PCA9554 ioCon1(0x20);  // Create an object at this address

ADS1100 ads;

uint8_t res;
void setup()
{
  uint8_t returnCode = 0;
  uint8_t channel = 0;

  M5.begin();
  M5.Power.begin();
  Wire.begin();
  M5.Lcd.clear(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(40, 30);
  M5.Lcd.print("UNIT_IO + ADC EXAMPLE\n");
  M5.Lcd.setTextColor(YELLOW,BLACK);
  M5.Lcd.setCursor(40, 70);
  M5.Lcd.println("PortA Hub Addr: 0x70");
  M5.Lcd.setCursor(25, 95);
  M5.Lcd.println("Ext_IO Ch: 0 Addr: 0x20");
  M5.Lcd.setCursor(25, 115);
  M5.Lcd.println("ADC1   Ch: 1 Addr: 0x48");
  M5.Lcd.setCursor(25, 135);
  M5.Lcd.println("ADC2   Ch: 2 Addr: 0x48");
  
  
  tca9548a.address(PaHub_I2C_ADDRESS);
  returnCode = tca9548a.selectChannel(channel);
    if( returnCode == 0 ) {
      Serial.printf("SDA:%d\r\n", SDA);
      Serial.printf("SCL:%d\r\n", SCL);
      
        ioCon1.portMode0(ALLOUTPUT); //Set the port as all output 
    }

  returnCode = tca9548a.selectChannel(1);
    if( returnCode == 0 ) {
      ads.getAddr_ADS1100(ADS1100_DEFAULT_ADDRESS);   // 0x48, 1001 000 (ADDR = GND)

      //ads.setGain(GAIN_ONE);          // 1x gain(default)
      // ads.setGain(GAIN_TWO);       // 2x gain
      //ads.setGain(GAIN_FOUR);      // 4x gain
      ads.setGain(GAIN_EIGHT);     // 8x gain

      ads.setMode(MODE_CONTIN);       // Continuous conversion mode (default)
      // ads.setMode(MODE_SINGLE);    // Single-conversion mode
  
      //ads.setRate(RATE_8);            // 8SPS (default)
      // ads.setRate(RATE_16);        // 16SPS
      //ads.setRate(RATE_32);        // 32SPS
      ads.setRate(RATE_128);       // 128SPS
  
      ads.setOSMode(OSMODE_SINGLE);   // Set to start a single-conversion
  
      ads.begin();
      Serial.println("ADC ch:0 Initialized");

    }

  //ioCon1.twiWrite(21, 22);
  //delay(10);
  //res = 1;
  //ioCon1.twiRead(res);
  //Serial.printf("res:%d\r\n", res);

}

void loop()
{
   byte error;
   int8_t address;

  tca9548a.selectChannel(0);
  // write single, the same read
  ioCon1.digitalWrite0(0, LOW);
  ioCon1.digitalWrite0(1, LOW);
  ioCon1.digitalWrite0(2, LOW);
  ioCon1.digitalWrite0(3, LOW);
  ioCon1.digitalWrite0(4, LOW);
  ioCon1.digitalWrite0(5, LOW);
  ioCon1.digitalWrite0(6, LOW);
  ioCon1.digitalWrite0(7, LOW);
  delay(1000);
  ioCon1.digitalWrite0(0, HIGH);
  ioCon1.digitalWrite0(1, HIGH);
  ioCon1.digitalWrite0(2, HIGH);
  ioCon1.digitalWrite0(3, HIGH);
  ioCon1.digitalWrite0(4, HIGH);
  ioCon1.digitalWrite0(5, HIGH);
  ioCon1.digitalWrite0(6, HIGH);
  ioCon1.digitalWrite0(7, HIGH);
  delay(1000);

  // write 0-7 HIGHT
//  Serial.println(ioCon1.digitalWritePort0(0xff));
//  delay(200);

  // write 0-7 LOW
//  Serial.println(ioCon1.digitalWritePort0(0x00));
//  delay(200);

  //// write Port, the same read
  for (byte i = 0; i < 8; i++) {
    ioCon1.digitalWritePort0((1 << i));
    tca9548a.selectChannel(1);

    address = ads.ads_i2cAddress;
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
        int16_t result;

        Serial.println("Getting Differential Reading from ADS1100");
        Serial.println(" ");
        delay(50);
        result = ads.Measure_Differential();
        Serial.print("Digital Value of Analog Input between Channel 0 and 1: ");
        Serial.println(result);
        //M5.Lcd.fillScreen(BLACK);
        char data[20] = { 0 };
        sprintf(data, "%d       ", result);
        //M5.Lcd.drawCentreString(data, 120, 160, 4);
        M5.Lcd.setCursor(10, 190);
        M5.Lcd.printf("ADC1:%d       ", result);
        Serial.println(" ");
        Serial.println("        ***************************        ");
        Serial.println(" ");
    }
    else
    {
        Serial.println("ADS1100 Disconnected!");
        Serial.println(" ");
        Serial.println("        ************        ");
        Serial.println(" ");
        M5.Lcd.setTextFont(4);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.drawString("No Found ADC 1 sensor.",20, 100, 4);
    }


    tca9548a.selectChannel(2);

    address = ads.ads_i2cAddress;
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
        int16_t result;

        Serial.println("Getting Differential Reading from ADS1100");
        Serial.println(" ");
        delay(50);
        result = ads.Measure_Differential();
        Serial.print("Digital Value of Analog Input between Channel 0 and 1: ");
        Serial.println(result);
        //M5.Lcd.fillScreen(BLACK);
        char data[20] = { 0 };
        sprintf(data, "%d       ", result);
        //M5.Lcd.drawCentreString(data, 120, 160, 4);
        M5.Lcd.setCursor(180, 190);
        M5.Lcd.printf("ADC2:%d       ", result);
        Serial.println(" ");
        Serial.println("        ***************************        ");
        Serial.println(" ");
    }
    else
    {
        Serial.println("ADS1100 Disconnected!");
        Serial.println(" ");
        Serial.println("        ************        ");
        Serial.println(" ");
        M5.Lcd.setTextFont(4);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.drawString("No Found ADC 2 sensor.",20, 100, 4);
    }


    
    tca9548a.selectChannel(0);
    delay(200);
  }
}
