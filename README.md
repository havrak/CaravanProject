# IOT caravan

IOT project using chips ESP32 as sensor unit and OLIMEX ESP32 Gateway as central unit.
On OLIMEX is also connected Nextion display (7 inches) which serves as for now only interface between user and system.
Point of project is to automatized certain tasks in caravan like controlling heating or water tank refillment.
More info will be added, including which components are used.

## libaries used
+ NTPClient (https://github.com/taranais/NTPClient)
+ ITEADLIB_Arduino_Nextion (https://github.com/itead/ITEADLIB_Arduino_Nextion)
	+ Requies: espsoftwareserial (https://github.com/plerup/espsoftwareserial)
+ Time (https://github.com/PaulStoffregen/Time)
+ ArduinoJson (https://github.com/bblanchon/ArduinoJson)
+ Timezone (https://github.com/JChristensen/Timezone)
+ Ethernet2 (zip from: https://github.com/m5stack/M5Stack/tree/master/examples/Modules/W5500)
+ DallasTemperature, M5Stack,OneWire
+ TinyGPS++ (http://arduiniana.org/libraries/tinygpsplus/)
