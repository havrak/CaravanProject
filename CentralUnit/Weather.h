// Weather object 
// inspired by http://educ8s.tv/esp32-weather-station/`
// Display on Nextion Location!!!!!
// to get sample optput enter https://api.openweathermap.org/data/2.5/weather?lat=50.1636703&lon=14.3836175&units=metric&cnt=1&APPID=fabd54dece7005a11d0cd555f2384df9
// to browser

#ifndef WEATHER_H
#define WEATHER_H
#include <WiFi.h>
#include <ArduinoJson.h>    //https://github.com/bblanchon/ArduinoJson
#include <EEPROM.h>


class Weather{
  public:    
    
    Weather(float lat, float lon){
      getWeather();
      this.lat = lat;
      this.lon = lon;
    }


    void setNewPosition(float lat, float lon){
      this.lat = lat;
      this.lon = lon;
    }
    // also will change configuration on mikrotik trought telnet
    void updateDataOnNextion(){
      
    }
    // will do 
    void updateYourDataFromMikrotik(){
      
    };

    // geters for variables in scructure for testing purpouse 
    byte getTypeOfConnection(){
      return typeOfConnection;
    }
    double getUpLink(){
      return UpLink;  
    }
    double getSignalStrenght(){
      return SignalStrenght;
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
      float windSpeed;
      String location;
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
            return;
          }
          // We now create a URI for the request
          // replace with yours APPID
          String url = "/data/2.5/weather?lat="+lat+"&lon="+lon+"&units=metric&cnt=1&APPID=fabd54dece7005a11d0cd555f2384df9";
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

          StaticJsonBuffer<1024> json_buf;
          JsonObject &root = json_buf.parseObject(jsonArray);
          if (!root.success())
          {
            Serial.println("parseObject() failed");
            return false;
          }

          location = root["list"]["sys"]["name"];
          temperature = root["list"]["main"]["temp"];
          weather = root["list"]["weather"]["main"];
          description = root["list"]["weather"]["description"];
          temperatureMax = root["list"]["main"]["temp_max"];
          temperatureMIn = root["list"]["main"]["temp_min"];
          timeS = root["list"]["dt_txt"];
          weatherID = root["list"]["weather"]["id"].toInt();
          wingDeg = root["list"]["wind"]["deg"].toInt();
          wingSpeed = root["list"]["wind"]["speed"].toInt();
          clouds = root["list"]["clouds"]["all"].toInt();
          
          Serial.print("\nWeatherID: ");
          Serial.print(weatherID);
          // same serial is used for nextion as for debug
          endNextionCommand();

      }

};
#endif WEATHER_H
