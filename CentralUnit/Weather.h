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
      command = "textWTemp.txt=\""+temperature+"°C\"";
      Serial2.print(command);
      startEndNextionCommand();
      command = "textWMinMax.txt=\""+temperatureMax+"°C ,"+temperatureMin+"°C\"";
      Serial2.print(command);
      startEndNextionCommand();
      // TODO: check field size
      if(windSpeed == 0){
        command = "textWWind.txt=\""+windDirection+" ("+String(windSpeed)+"m/s)\"";
      }else{
        command = "textWWind.txt=\""+windDirection+" ("+String(windSpeed)+"m/s)\"";
      }
      Serial2.print(command);
      startEndNextionCommand();
      // Add zataženo, oblačno
      // clouds == 0 ??
      String coverage;
      if(clouds == 100){
        coverage = "overcast";
      }else if(clouds > 87){
        coverage = "broken";
      }else if(clouds > 62){
        coverage = "mostly cloudy";
      }else if(clouds > 37){
        coverage = "partly cloudy";
      }else if(clouds > 12){
        coverage = "mostly sunny";
      }else{
        coverage = "sunny";
      }
      command = "textWCloud.txt=\""+coverage+"\"";
      Serial2.print(command);
      startEndNextionCommand();
      // make substring or whatever
      command = "textWLocation.txt="+location;
      Serial2.print(command);
      startEndNextionCommand();
      // set icon according to weather id provided by openweather
      switch(weatherID){
        // Cloudy or clear sky
        case 800: {
          if(hours >4 || hours < 20){
            startEndNextionCommand();
            String command = "imgWeather.pic=42";
            Serial2.print(command);
            startEndNextionCommand();
          }else{
            startEndNextionCommand();
            String command = "imgWeather.pic=43";
            Serial2.print(command);
            startEndNextionCommand();
          }
          break;
          }
        case 801:{
           if(hours >4 || hours < 20){
            startEndNextionCommand();
            String command = "imgWeather.pic=44";
            Serial2.print(command);
            startEndNextionCommand();
          }else{
            startEndNextionCommand();
            String command = "imgWeather.pic=45";
            Serial2.print(command);
            startEndNextionCommand();
          }
          break;
        }
        case 802: drawModrateCloud(); break;
        case 803: drawModrateCloud(); break;
        case 804:{
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
    }
    bool update(){
      return getWeather();
    }
    private:
      WiFiClient client;
      // for some reason IP cannot be created here and needs to be passed from main
      const char* servername ="api.openweathermap.org";  // remote server we will connect to
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
        Serial2.write(0xff);
        Serial2.write(0xff);
        Serial2.write(0xff);
      }


      bool getWeather(){
          Serial.println("WEATHER | UPDATE");
          String result ="";
          const int httpPort = 80;
          if (!client.connect(servername, httpPort)) {
            Serial.println("Cannot connect to server");
            return false;
          }
          Serial.println("WEATHER | GOT CONNECTION");
          // We now create a URI for the request
          String latStr = String(lat, 5);
          String lonStr = String(lon, 5);
          String url = "/data/2.5/weather?lat="+latStr+"&lon="+lonStr+"&units=metric&cnt=1&lang=cz&APPID=fabd54dece7005a11d0cd555f2384df9";
          // This will send the request to the server
          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
            "Host: " + String(servername) + "\r\n" +
            "Connection: close\r\n\r\n");
          unsigned long timeout = millis();
          // "wait" if client is not available
          while (client.available() == 0) {
            if (millis() - timeout > 5000) {
            client.stop();
            Serial.println("Client is not available");
            return false;
            }
          }
          // Read all the lines of the reply from server
          Serial.println("WEATHER | GETTING DATA FORM SERVER");
          while(client.available()) { // doesn't work
            result = client.readStringUntil('\r');
            //Serial.println("WEATHER | GOT DATA");
          }
          
          //String result = "{\"coord\":{\"lon\":14.4,\"lat\":50.16},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}],\"base\":\"stations\",\"main\":{\"temp\":295.02,\"pressure\":1023,\"humidity\":35,\"temp_min\":293.71,\"temp_max\":296.48},\"visibility\":10000,\"wind\":{\"speed\":2.6,\"deg\":280},\"clouds\":{\"all\":0},\"dt\":1567523276,\"sys\":{\"type\":1,\"id\":6848,\"message\":0.0082,\"country\":\"CZ\",\"sunrise\":1567484380,\"sunset\":1567532676},\"timezone\":7200,\"id\":3066636,\"name\":\"Roztoky\",\"cod\":200}";
          Serial.println(result);
          Serial.println("WEATHER | GOT DATA");
          // remove notation for array
          result.replace('[', ' ');
          result.replace(']', ' ');
          
          StaticJsonDocument<1024> jsonDoc;
          DeserializationError err = deserializeJson(jsonDoc, result);
          
          if (err != DeserializationError::Ok ){ // seems to work
            Serial.println("parseObject() failed");
            return false; 
          }  
          Serial.println("WEATHER | JSON CREATED");
          
          // need to use as<String>() syntex otherwise throws error, not sure why
          // error message:  ambiguous overload for 'operator=' (operand types are 'String' and 'ArduinoJson6114_000001::enable_if<true, ArduinoJson6114_000001::MemberProxy<ArduinoJson6114_000001::JsonDocument&, const char*> >::type {aka ArduinoJson6114_000001::MemberProxy<ArduinoJson6114_000001::JsonDocument&, const char*>}')
          // const char* val = jsonDoc["name"]; 
          
          location = jsonDoc["name"].as<String>(); // is empty
          temperature = jsonDoc["main"]["temp"].as<String>();
          weather = jsonDoc["weather"]["main"].as<String>();
          description = jsonDoc["weather"]["description"].as<String>();
          temperatureMax = jsonDoc["main"]["temp_max"].as<String>();
          temperatureMin = jsonDoc["main"]["temp_min"].as<String>();
          Serial.println("WEATHER | FILLED STRINGS");
          weatherID =  jsonDoc["weather"]["id"].as<int>();
          windDeg =  jsonDoc["wind"]["deg"].as<int>();
          windSpeed =  jsonDoc["wind"]["speed"].as<float>();
          clouds =  jsonDoc["clouds"]["all"].as<int>();
          setWindDirection();
          Serial.println("WEATHER | FILLED NUMBERS");
          
          Serial.print("WEATHER | WeatherID: ");
          Serial.println(weatherID);
          // same serial is used for nextion as for debug
          startEndNextionCommand();
          return true;
      }
      // check it, no need to test for speed zero (will be taken care of in sendDatatoNextion())
      void setWindDirection(){
        if(windDeg >= 338){
          windDirection = "North";
        }else if(windDeg >= 292){
          windDirection = "Northwest";
        }else if(windDeg >= 248){
          windDirection = "West";
        }else if(windDeg >= 202){
          windDirection = "Southwest";
        }else if(windDeg >= 158){
          windDirection = "South";
        }else if(windDeg >= 112){
          windDirection = "Southeast";
        }else if(windDeg >= 68){
          windDirection = "East";
        }else if(windDeg >= 22){
          windDirection = "Northeast";
        }else{
          windDirection = "North";
        }
      }
      
      void drawModrateCloud(){
        startEndNextionCommand();
        String command = "imgWeather.pic=46";
        Serial2.print(command);
        startEndNextionCommand();
      }
      void drawStrormWithRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=23";
        Serial2.print(command);
        startEndNextionCommand();        
      }
      void drawStorm(){
        startEndNextionCommand();
        String command = "imgWeather.pic=24";
        Serial2.print(command);
        startEndNextionCommand(); 
      }
      void drawStromWithDrizzle(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=25";
          Serial2.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=26";
          Serial2.print(command);
          startEndNextionCommand();
        }
      }

      void drawHeavyDrizzle(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=25";
          Serial2.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=26";
          Serial2.print(command);
          startEndNextionCommand();
        }
        
      }
      void drawDrizzle(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=29";
          Serial2.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=30";
          Serial2.print(command);
          startEndNextionCommand();
        }
      }
      void drawOtherRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=33";
        Serial2.print(command);
        startEndNextionCommand();
      }
      void drawRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=31";
        Serial2.print(command);
        startEndNextionCommand();
      }
      void drawHeavyRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=32";
        Serial2.print(command);
        startEndNextionCommand();
      }

      void drawSnow(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=34";
          Serial2.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=35";
          Serial2.print(command);
          startEndNextionCommand();
        }
      }
      
      void drawHeavySnow(){
        startEndNextionCommand();
        String command = "imgWeather.pic=36";
        Serial2.print(command);
        startEndNextionCommand();
      }
      
      void drawSnowWithRain(){
        startEndNextionCommand();
        String command = "imgWeather.pic=37";
        Serial2.print(command);
        startEndNextionCommand();
      }
      void drawLowVisibility(){
        startEndNextionCommand();
        String command = "imgWeather.pic=40";
        Serial2.print(command);
        startEndNextionCommand(); 
      }
      void drawLimitedVisibility(int hours){
        if(hours >4 || hours < 20){
          startEndNextionCommand();
          String command = "imgWeather.pic=38";
          Serial2.print(command);
          startEndNextionCommand();
        }else{
          startEndNextionCommand();
          String command = "imgWeather.pic=39";
          Serial2.print(command);
          startEndNextionCommand();
        }
      }
      void drawTornado(){
        startEndNextionCommand();
        String command = "imgWeather.pic=41";
        Serial2.print(command);
        startEndNextionCommand(); 
      }

      void drawUnknownVaules(){
        startEndNextionCommand();
        String command = "imgWeather.pic=48";
        Serial2.print(command);
        startEndNextionCommand();  
      }
};
#endif
