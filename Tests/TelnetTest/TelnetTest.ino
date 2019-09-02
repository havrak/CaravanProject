#include <ETH.h>
#include <WiFi.h>
#include <TelnetClient.h>
#include <ETH.h>        // for ethernet to work

WiFiClient client;
telnetClient tc(client); 

IPAddress mikrotikIP;

IPAddress server(192,168,88,1);
bool isTelnetConnectionRunning = false;
bool didIAuthorized = false;



void setup() {
  Serial.begin(115200);
  ETH.begin();
  Serial.println("nected");
  //WiFi.status() == WL_CONNECTED ? Serial.println("connected"): NULL;
  if (client.connect(server, 23)) {
    Serial.println("Connected");
  }
  tc.setPromptChar('>');
  
  if(tc.login(server, "admin", "SppO1234")){        //tc.login(mikrotikRouterIp, "admin", "", 1234) if you want to specify a port different than 23
    Serial.println("Got past loggin");
    tc.sendCommand("/interface print");
    //tc.sendCommand("print");
    //tc.sendCommand("address");
    //tc.sendCommand("print");
  }
  tc.disconnect();


}

void loop() {
  /*
  if(!isTelnetConnectionRunning){
    if (client.connect(server, 23)) {
      isTelnetConnectionRunning = true;
    }
  } 
  Serial.print(isTelnetConnectionRunning);
  if(isTelnetConnectionRunning){
    if(!didIAuthorized){
      client.write("krystof");
      client.write("roztoky");
      didIAuthorized = true;
    }
  }
  client.write("/interface ethernet poe set ether4 poe-out=off");
  */
}
