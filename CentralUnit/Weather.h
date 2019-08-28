// Weather class takes care of weather, which is provided by api.openweathermap.org.
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
    
    void updateDataOnNextion(int hours){
      String command;
      startEndNextionCommand();
      command = "textWTemp.txt="+temperature+"°C";
      Serial.print(command);
      startEndNextionCommand();
      command = "textWMinMax.txt="+temperatureMax+"°C ,"+temperatureMin+"°C";
      Serial.print(command);
      startEndNextionCommand();
      // check field size
      command;
      if(windSpeed == 0){
        command = "textWWind.txt="+windDirection+" ("+String(windSpeed)+"m/s)";
      }else{
        command = "textWWind.txt="+windDirection+" ("+String(windSpeed)+"m/s)";
      }
      Serial.print(command);
      startEndNextionCommand();
      // Add zataženo, oblačno
      // clouds == 0 ??
      String coverage;
      if(clouds == 100){
        coverage = "zataženo";
      }else if(clouds > 87){
        coverage = "skoro zataženo";
      }else if(clouds > 62){
        coverage = "oblačno";
      }else if(clouds > 37){
        coverage = "polojasno";
      }else if(clouds > 12){
        coverage = "skoro jasno";
      }else{
        coverage = "jasno";
      }
      command = "textWCloud.txt="+coverage;
      Serial.print(command);
      startEndNextionCommand();
      // make substring or whatever
      command = "textWLocation.txt="+location;
      Serial.print(command);
      startEndNextionCommand();
      // set icon according to weather id provided by openweather
      switch(weatherID){
        // Cloudy or clear sky
        case 800: {
          if(hours >4 || hours < 20){
            startEndNextionCommand();
            String command = "imgWeather.pic=42";
            Serial.print(command);
            startEndNextionCommand();
          }else{
            startEndNextionCommand();
            String command = "imgWeather.pic=43";
            Serial.print(command);
            startEndNextionCommand();
          }
          break;
          }
        case 801:{
           if(hours >4 || hours < 20){
            startEndNextionCommand();
            String command = "imgWeather.pic=44";
            Serial.print(command);
            startEndNextionCommand();
          }else{
            startEndNextionCommand();
            String command = "imgWeather.pic=45";
            Serial.print(command);
            startEndNextionCommand();
          }
          break;
        }
        case 802: drawModrateCloud(); break;
        case 803: drawModrateCloud(); break;
        case 804:{
          startEndNextionCommand();
          String command = "imgWeather.pic=47";
          Serial.print(command);
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

      // why int?????
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

      void startEndNextionCommand()
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
          windSpeed = String((const char*)root["list"]["wind"]["speed"]).toFloat();
          clouds = String((const char*)root["list"]["clouds"]["all"]).toInt();
          setWindDirection();

          Serial.print("\nWeatherID: ");
          Serial.print(weatherID);
          // same serial is used for nextion as for debug
          startEndNextionCommand();

      }
      // check it, no need to test for speed zero (will be taken care of in sendDatatoNextion())
      void setWindDirection(){
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
        }else if(windDeg >= 68){
          windDirection = "Východní";
        }else if(windDeg >= 22){
          windDirection = "Severovýchodní";
        }else{
          windDirection = "Severní";
        }
      }
      
      void drawModrateCloud(){
        startEndNextionCommand();
        String command = "imgWeather.pic=46";
        Serial.print(command);
        startEndNextionCommand();
      }
      void drawStrormWithRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=23";
        Serial.print(command);
        startEndNextionCommand();        
      }
      void drawStorm(){
        startEndNextionCommand();
        String command = "imgWeather.pic=24";
        Serial.print(command);
        startEndNextionCommand(); 
      }
      void drawStromWithDrizzle(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=25";
          Serial.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=26";
          Serial.print(command);
          startEndNextionCommand();
        }
      }

      void drawHeavyDrizzle(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=25";
          Serial.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=26";
          Serial.print(command);
          startEndNextionCommand();
        }
        
      }
      void drawDrizzle(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=29";
          Serial.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=30";
          Serial.print(command);
          startEndNextionCommand();
        }
      }
      void drawOtherRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=33";
        Serial.print(command);
        startEndNextionCommand();
      }
      void drawRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=31";
        Serial.print(command);
        startEndNextionCommand();
      }
      void drawHeavyRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=32";
        Serial.print(command);
        startEndNextionCommand();
      }

      void drawSnow(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=34";
          Serial.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=35";
          Serial.print(command);
          startEndNextionCommand();
        }
      }
      
      void drawHeavySnow(){
        startEndNextionCommand();
        String command = "imgWeather.pic=36";
        Serial.print(command);
        startEndNextionCommand();
      }
      
      void drawSnowWithRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=37";
        Serial.print(command);
        startEndNextionCommand();
      }
      void drawLowVisibility(){
        startEndNextionCommand();
        String command = "imgWeather.pic=40";
        Serial.print(command);
        startEndNextionCommand(); 
      }
      void drawLimitedVisibility(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=38";
          Serial.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=39";
          Serial.print(command);
          startEndNextionCommand();
        }
      }
      void drawTornado(){
        startEndNextionCommand();
        String command = "imgWeather.pic=41";
        Serial.print(command);
        startEndNextionCommand(); 
      }

      void drawUnknownVaules(){
        startEndNextionCommand();
        String command = "imgWeather.pic=48";
        Serial.print(command);
        startEndNextionCommand();  
      }
};
#endif WEATHER_H
