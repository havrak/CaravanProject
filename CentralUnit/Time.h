#ifndef TIME_H
#define TIME_H

#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// will update its data evry 250 calls (if it works)
class Time{
  public:
    Time(){
      timeClient =  NTPClient(Udp)
      timeClient.begin();
      timeClient.setTimeOffset(timeOffset);
      timeClient.forceUpdate();
    }

    // also will change configuration on mikrotik trought telnet
    void updateDataOnNextion(){
      if(refreshCounter == 250 || !wasLastRefreshSuccess){
          updateYourData()
          refreshcounter=0;
      }
      // The formattedDate comes with the following format:
      // 2018-05-28T16:00:13Z
      // will be send to olimex
      formattedDate = timeClient.getFormattedDate();
      Serial.println(formattedDate);

      // Extract date
      int splitT = formattedDate.indexOf("T");
      dayStamp = formattedDate.substring(0, splitT);
      Serial.print("DATE: ");
      Serial.println(dayStamp);
      // Exract time
      timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
      Serial.print("HOUR: ");
      Serial.println(timeStamp);
      refreshcounter++;
      }
    private: 
      bool wasLastRefreshSuccess = false;;
      int refreshCounter = 0;
      String formattedDate;
      String dayStamp;
      String timeStamp;
      int timeOffset = 7200;
      
      WiFiClient client;
      WiFiUDP Udp;
      NTPClient timeClient;

      void updateYourData(){
        while(!timeClient.update()) {
        timeClient.setTimeOffset(timeOffset);
        wasLastRefreshSuccess = timeClient.forceUpdate();
        }
      }
#endif TIME_H
