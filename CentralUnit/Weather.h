// class which takes care of gweather
#ifndef WEATHER_H
#define WEATHER_H
#include <Ethernet2.h>
#include <string>     // std::string, std::to_string
#include <ArduinoJson.h>    //https://github.com/bblanchon/ArduinoJson
#include <EEPROM.h>


class Weather {
  public:
    Weather(double newLat, double newLon) {
      lat = newLat;
      lon = newLon;
      //update();
    }

    void setNewPosition(double newLat, double newLon) {
      lat = newLat;
      lon = newLon;
    }
    void usePictureOnNextion(bool picture) { //
      displayPicture = picture;

    }

    void updateDataOnNextion() {
      String command;
      startEndNextionCommand();
      int hours = hour();
      command = "textWTemp.txt=\"" + temperature + "°C\"";
      Serial2.print(command);
      startEndNextionCommand();
      command = "textWMinMax.txt=\"" + temperatureMax + "°C," + temperatureMin + "°C\"";
      Serial2.print(command);
      startEndNextionCommand();
      // TODO: check field size
      if (windSpeed == 0) {
        command = "textWWind.txt=\"" + windDirection + " (" + String(windSpeed) + "m/s)\"";
      } else {
        command = "textWWind.txt=\"" + windDirection + " (" + String(windSpeed) + "m/s)\"";
      }
      Serial2.print(command);
      startEndNextionCommand();
      // Add zataženo, oblačno
      // clouds == 0 ??
      
      command = "textWCloud.txt=\"" + coverage + "\"";
      Serial2.print(command);
      startEndNextionCommand();
      // make substring or whatever
      command = "textWLocation.txt=\"" + location + "\"";
      Serial2.print(command);
      startEndNextionCommand();
      // set icon according to weather id provided by openweather
      if (displayPicture) {
        command = "vidWeather.aph=0"; // make video transparrent
        Serial2.print(command);
        startEndNextionCommand();
        Serial2.print("imgWeather.aph=127");
        startEndNextionCommand();
        switch (weatherID) {
          // Cloudy or clear sky
          case 800: {
              if (hours > 4 || hours < 20) {
                startEndNextionCommand();
                String command = "imgWeather.pic=42";
                Serial2.print(command);
                startEndNextionCommand();
              } else {
                startEndNextionCommand();
                String command = "imgWeather.pic=43";
                Serial2.print(command);
                startEndNextionCommand();
              }
              break;
            }
          case 801: {
              if (hours > 4 || hours < 20) {
                startEndNextionCommand();
                String command = "imgWeather.pic=44";
                Serial2.print(command);
                startEndNextionCommand();
              } else {
                startEndNextionCommand();
                String command = "imgWeather.pic=45";
                Serial2.print(command);
                startEndNextionCommand();
              }
              break;
            }
          case 802: drawModrateCloud(); break;
          case 803: drawModrateCloud(); break;
          case 804: {
              startEndNextionCommand();
              String command = "imgWeather.pic=47";
              Serial2.print(command);
              startEndNextionCommand();
              break;
            }
          // Strom
          case 200: drawStrormWithRain(); break;
          case 201: drawStrormWithRain(); break;
          case 202: drawStrormWithRain(); break;
          case 210: drawStorm(); break;
          case 211: drawStorm(); break;
          case 212: drawStorm(); break;
          case 221: drawStorm(); break;
          case 230: drawStromWithDrizzle(hours); break;
          case 231: drawStromWithDrizzle(hours); break;
          case 232: drawStromWithDrizzle(hours); break;

          // Drizzle
          case 300: drawDrizzle(hours);  break;
          case 301: drawDrizzle(hours); break;
          case 302: drawDrizzle(hours); break;
          case 310: drawDrizzle(hours);  break;
          case 311: drawDrizzle(hours); break;
          case 312: drawHeavyDrizzle(hours);  break;
          case 313: drawHeavyDrizzle(hours);  break;
          case 314: drawHeavyDrizzle(hours);  break;
          case 321: drawHeavyDrizzle(hours);  break;

          // Rain
          case 500: drawRain();  break;
          case 501: drawRain();  break;
          case 502: drawHeavyRain();  break;
          case 503: drawHeavyRain();  break;
          case 504: drawHeavyRain();  break;
          case 511: drawOtherRain();  break;
          case 520: drawOtherRain();  break;
          case 521: drawOtherRain();  break;
          case 522: drawHeavyRain();  break;
          case 531: drawOtherRain();  break;

          // Snow
          case 600: drawSnow(hours);  break;
          case 601: drawSnow(hours);  break;
          case 602: drawHeavySnow();  break;
          case 611: drawHeavySnow();  break;
          case 612: drawHeavySnow();  break;
          case 613: drawHeavySnow(); break;
          case 615: drawSnowWithRain();  break;
          case 616: drawSnowWithRain(); break;
          case 620: drawHeavySnow();  break;
          case 621: drawHeavySnow();  break;
          case 622: drawHeavySnow();  break;

          // Atmosphere
          case 701: drawLimitedVisibility(hours);  break;
          case 711: drawLowVisibility();  break;
          case 721: drawLimitedVisibility(hours);  break;
          case 731: drawLowVisibility();  break;
          case 741: drawLimitedVisibility(hours);  break;
          case 751: drawLowVisibility();  break;
          case 761: drawLowVisibility();  break;
          case 762: drawLowVisibility();  break;
          case 771: drawLimitedVisibility(hours);  break;
          case 781: drawTornado();  break;

          default: drawUnknownVaules(); break;
        }
      } else {
        command = "imgWeather.aph=0"; // make video transparrent
        Serial2.print(command);
        startEndNextionCommand();
        Serial2.print("vidWeather.aph=127");
        startEndNextionCommand();
        
        Serial.print("WEATHER | dispaly | displayng: "); Serial.println(weatherID);
        if (weatherID >= 500 && weatherID <= 531) vidRain();
        else if (weatherID >= 300 && weatherID <= 321) vidDrizzle(hours);
        else if (weatherID >= 200 && weatherID <= 232) vidStorm();
        else if (weatherID == 615 || weatherID == 616) vidSnowRain();
        else if ((weatherID >= 602 && weatherID <= 613) || (weatherID >= 620 && weatherID <= 622)) vidSnow();
        else if (weatherID == 600 || weatherID == 601) vidLightSnow(hours);
        else if (weatherID == 701 || weatherID == 721 || weatherID == 741) vidBroken(hours);
        else if (weatherID == 801) vidMostlyClear(hours);
        else if (weatherID == 800) vidClear(hours);
        else if (weatherID == 802 || weatherID == 803 || weatherID == 804 || weatherID == 711 || weatherID == 731 || weatherID == 761 || weatherID == 781) vidOvercast();
        else drawUnknownVaules();
      }
    }

    // updates weather data from OpenWeather
    // retirn true if update was succesfull
    bool update() {

      EthernetClient client;
      //Serial.println("WEATHER | UPDATE");
      String result = "";
      if (!client.connect(servername, 80)) {
        Serial.println("WEATHER | update | Cannot connect to server");
        return false;
      }
      //Serial.println("WEATHER | update | GOT CONNECTION");
      // We now create a URI for the request
      String latStr = String(lat, 5);
      String lonStr = String(lon, 5);
      String url = "/data/2.5/weather?lat=" + latStr + "&lon=" + lonStr + "&units=metric&cnt=1&lang=cz&APPID=fabd54dece7005a11d0cd555f2384df9";
      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + String(servername) + "\r\n" +
                   "Connection: close\r\n\r\n");
      unsigned long timeout = millis();
      // "wait" if client is not available
      while (client.available() == 0) {
        if (millis() - timeout < 5000) {
          if (millis() - timeout < 0) timeout = millis();
        } else {
          break;
          return false;
        }
      }
      // Read all the lines of the reply from server
      Serial.println("WEATHER | update | GETTING DATA FORM SERVER");
      while (client.available()) { // doesn't work
        result = client.readStringUntil('\r');
        //Serial.println("WEATHER | GOT DATA");
      }

      //String result = "{\"coord\":{\"lon\":14.4,\"lat\":50.16},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}],\"base\":\"stations\",\"main\":{\"temp\":295.02,\"pressure\":1023,\"humidity\":35,\"temp_min\":293.71,\"temp_max\":296.48},\"visibility\":10000,\"wind\":{\"speed\":2.6,\"deg\":280},\"clouds\":{\"all\":0},\"dt\":1567523276,\"sys\":{\"type\":1,\"id\":6848,\"message\":0.0082,\"country\":\"CZ\",\"sunrise\":1567484380,\"sunset\":1567532676},\"timezone\":7200,\"id\":3066636,\"name\":\"Roztoky\",\"cod\":200}";
      //Serial.println("WEATHER | GOT DATA");
      // remove notation for array
      result.replace('[', ' ');
      result.replace(']', ' ');

      StaticJsonDocument<1024> jsonDoc;
      DeserializationError err = deserializeJson(jsonDoc, result);

      if (err != DeserializationError::Ok ) { // seems to work
        //Serial.println("WEATHER | update | DERSERIZATION FAILED");
        return false;
      }
      //Serial.println("WEATHER | update | JSON CREATED");

      location = jsonDoc["name"].as<String>(); // is empty
      temperature = jsonDoc["main"]["temp"].as<String>();
      weather = jsonDoc["weather"]["main"].as<String>();
      description = jsonDoc["weather"]["description"].as<String>();
      temperatureMax = jsonDoc["main"]["temp_max"].as<String>();
      temperatureMin = jsonDoc["main"]["temp_min"].as<String>();
      weatherID =  jsonDoc["weather"]["id"].as<int>();
      windDeg =  jsonDoc["wind"]["deg"].as<int>();
      windSpeed =  jsonDoc["wind"]["speed"].as<float>();
      clouds =  jsonDoc["clouds"]["all"].as<int>();
      setWindDirection();
      setCoverage();
      //Serial.println("WEATHER | update | FILLED STRINGS and NUMBERS");

      //Serial.print("WEATHER | WeatherID: ");
      //Serial.println(weatherID);
      // same serial is used for nextion as for debug
      startEndNextionCommand();
      return true;
    }

  private:
    const char* servername = "api.openweathermap.org";
    String result;
    float lat;
    float lon;
    bool displayPicture;

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
    String coverage;
     
    String weather;
    String description;
    String idString;
    String timeS;

    void startEndNextionCommand()
    {
      Serial2.write(0xff);
      Serial2.write(0xff);
      Serial2.write(0xff);
    }
    
    // sets coverage from clouds
    void setCoverage(){
      if (clouds == 100) {
        coverage = "overcast";
      } else if (clouds > 87) {
        coverage = "broken";
      } else if (clouds > 62) {
        coverage = "mostly cloudy";
      } else if (clouds > 37) {
        coverage = "partly cloudy";
      } else if (clouds > 12) {
        coverage = "mostly sunny";
      } else {
        coverage = "sunny";
      }  
    }
    
    // sets wind direction from angle of wind
    void setWindDirection() {
      if (windDeg >= 338) {
        windDirection = "North";
      } else if (windDeg >= 292) {
        windDirection = "Northwest";
      } else if (windDeg >= 248) {
        windDirection = "West";
      } else if (windDeg >= 202) {
        windDirection = "Southwest";
      } else if (windDeg >= 158) {
        windDirection = "South";
      } else if (windDeg >= 112) {
        windDirection = "Southeast";
      } else if (windDeg >= 68) {
        windDirection = "East";
      } else if (windDeg >= 22) {
        windDirection = "Northeast";
      } else {
        windDirection = "North";
      }
    }

    void vidRain() {
      startEndNextionCommand();
      Serial2.print("vidWeather.vid=0");
      startEndNextionCommand();
    }
    void vidDrizzle(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=1");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=2");
        startEndNextionCommand();
      }
    }

    void vidStorm() {
      startEndNextionCommand();
      Serial2.print("vidWeather.vid=3");
      startEndNextionCommand();
    }
    void vidSnowRain() {
      startEndNextionCommand();
      Serial2.print("vidWeather.vid=4");
      startEndNextionCommand();
    }

    void vidSnow() {
      startEndNextionCommand();
      Serial2.print("vidWeather.vid=5");
      startEndNextionCommand();
    }

    void vidLightSnow(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=7");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=6");
        startEndNextionCommand();
      }
    }

    void vidBroken(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=8");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=9");
        startEndNextionCommand();
      }
    }

    void vidClear(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial.print("WEATHER | vidClear| hours:");
        Serial.println(hours);
        Serial2.print("vidWeather.vid=10");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=11");
        startEndNextionCommand();
      }
    }

    void vidMostlyClear(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=12");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("vidWeather.vid=13");
        startEndNextionCommand();
      }
    }

    void vidOvercast() {
      startEndNextionCommand();
      Serial2.print("vidWeather.vid=14");
      startEndNextionCommand();
    }

    void drawModrateCloud() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=46");
      startEndNextionCommand();
    }
    void drawStrormWithRain() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=23");
      startEndNextionCommand();
    }
    void drawStorm() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=24");
      startEndNextionCommand();
    }
    void drawStromWithDrizzle(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=25");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=26");
        startEndNextionCommand();
      }
    }

    void drawHeavyDrizzle(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=25");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=26");
        startEndNextionCommand();
      }

    }
    void drawDrizzle(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=29");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=30");
        startEndNextionCommand();
      }
    }
    void drawOtherRain() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=33");
      startEndNextionCommand();
    }
    void drawRain() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=31");
      startEndNextionCommand();
    }
    void drawHeavyRain() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=32");
      startEndNextionCommand();
    }

    void drawSnow(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=34");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=35");
        startEndNextionCommand();
      }
    }

    void drawHeavySnow() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=36");
      startEndNextionCommand();
    }

    void drawSnowWithRain() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=37");
      startEndNextionCommand();
    }
    void drawLowVisibility() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=40");
      startEndNextionCommand();
    }
    void drawLimitedVisibility(int hours) {
      if (hours > 4 || hours < 20) {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=38");
        startEndNextionCommand();
      } else {
        startEndNextionCommand();
        Serial2.print("imgWeather.pic=39");
        startEndNextionCommand();
      }
    }
    void drawTornado() {
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=41");
      startEndNextionCommand();
    }

    void drawUnknownVaules() {
      startEndNextionCommand(); // will be called both from video and picture;
      Serial2.print("vidWeather.aph=0");
      startEndNextionCommand();
      Serial2.print("imgWeather.aph=127");
      startEndNextionCommand();
      Serial2.print("imgWeather.pic=48");
      startEndNextionCommand();
    }
};
#endif
