// Weather object 
// inspired by http://educ8s.tv/esp32-weather-station/`
// uses ArduinoJSON v6 (inspiration uses v5)
// https://arduinojson.org/v6/api/jsondocument/
// Display on Nextion Location!!!!!
// to get sample optput enter https://api.openweathermap.org/data/2.5/weather?lat=50.1636703&lon=14.3836175&units=metric&cnt=1&APPID=fabd54dece7005a11d0cd555f2384df9
// to browser


// display
// 1. temp
// 2. temp min, max
// 3. windDirection (windSpeed)
// 4. clouds (in %)
// 5. location
#ifndef WEATHER_H
#define WEATHER_H
#include <WiFi.h>
#include <string>     // std::string, std::to_string
#include <ArduinoJson.h>    //https://github.com/bblanchon/ArduinoJson
#include <EEPROM.h>


class Weather{
  public:    
    
    Weather(float newLat, float newLon){
      getWeather();
      lat = newLat;
      lon = newLon;
    }


    void setNewPosition(float newLat, float newLon){
      lat = newLat;
      lon = newLon;
    }
    // also will change configuration on mikrotik trought telnet
    void updateDataOnNextion(){
      
    }
    
    private: 
      WiFiClient client;
      // for some reason IP cannot be created here and needs to be passed from main
      char* servername ="api.openweathermap.org";  // remote server we will connect to
      String result;
      float lat;
      float lon;

      // weather info
      int weatherID;
      int windDeg;
      
      int clouds;

      // wind ($windDiresciton ($windSpeed ms1))
      
      String windDirection;
      float windSpeed;
      
      String location;
      // all in one
      String temperature;
      String temperatureMax;
      String temperatureMin;
      
      String weather;
      String description;
      String idString;
      String timeS;
          
      void endNextionCommand()
      {
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);
      }

      
      bool getWeather(){
          String result ="";
          const int httpPort = 80;
          if (!client.connect(servername, httpPort)) {
            return false;
          }
          // We now create a URI for the request
          // replace with yours APPID
          String latStr = String(lat, 5);
          String lonStr = String(lon, 5);
          String url = "/data/2.5/weather?lat="+latStr+"&lon="+lonStr+"&units=metric&cnt=1&lang=cz&APPID=fabd54dece7005a11d0cd555f2384df9";
          // This will send the request to the server
          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
            "Host: " + servername + "\r\n" +
            "Connection: close\r\n\r\n");
          unsigned long timeout = millis();
          // "wait" if client is not available
          while (client.available() == 0) {
            if (millis() - timeout > 5000) {
            client.stop();
            return false;
            }
          }

          // Read all the lines of the reply from server
          while(client.available()) {
            result = client.readStringUntil('\r');
          }

          // remove notation for array
          result.replace('[', ' ');
          result.replace(']', ' ');

          
          char jsonArray [result.length()+1];
          result.toCharArray(jsonArray,sizeof(jsonArray));
          jsonArray[result.length() + 1] = '\0';

          StaticJsonDocument<1024> jsonDoc;
          // check if it works
          DeserializationError err = deserializeJson(jsonDoc, jsonArray);
          if (err != DeserializationError::Ok ){
            Serial.println("parseObject() failed");
            return false;
          }
          JsonObject root = jsonDoc.as<JsonObject>();
         
          // const char* is so obstructed in JsonObject that you need conversion to const char* to convert it to string
          location = String((const char*)root["list"]["sys"]["name"]);
          temperature = String((const char*)root["list"]["main"]["temp"]);
          weather = String((const char*)root["list"]["weather"]["main"]);
          description = String((const char*)root["list"]["weather"]["description"]);
          temperatureMax = String((const char*)root["list"]["main"]["temp_max"]);
          temperatureMin = String((const char*)root["list"]["main"]["temp_min"]);
          timeS = String((const char*)root["list"]["dt_txt"]);
          
          weatherID = String((const char*)root["list"]["weather"]["id"]).toInt();
          windDeg = String((const char*)root["list"]["wind"]["deg"]).toInt();
          windSpeed = String((const char*)root["list"]["wind"]["speed"]).toInt();
          clouds = String((const char*)root["list"]["clouds"]["all"]).toInt();
          setWindDirection();
          
          Serial.print("\nWeatherID: ");
          Serial.print(weatherID);
          // same serial is used for nextion as for debug
          endNextionCommand();

      }

      void setWindDirection(){
        if(windSpeed != 0){
          return; 
        }
        if(windDeg >= 338){
          windDirection = "Severní";  
        }else if(windDeg >= 292){
          windDirection = "Severozápadní"; 
        }else if(windDeg >= 248){
          windDirection = "Západní";
        }else if(windDeg >= 202){
          windDirection = "Jihozápadní"; 
        }else if(windDeg >= 158){
          windDirection = "Jižní"; 
        }else if(windDeg >= 112){
          windDirection = "Jihovýchodní"; 
        }else if(windDeg >= 112){
          windDirection = "Východní"; 
        }else if(windDeg >= 68){
          windDirection = "Severovýchodní"; 
        }else if(windDeg >= 22){
          windDirection = "Severní"; 
        }
        
      }

};
#endif WEATHER_H
