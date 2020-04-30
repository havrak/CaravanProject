#include <M5Stack.h>
#include "Free_Fonts.h"
#include <SPI.h>
#include <Ethernet2.h>
#define SCK 18
#define MISO 19
#define MOSI 23
#define CS 26

//  01 05 00 01 02 00 9d 6a
char uart_buffer[8] = {0x01, 0x05, 0x00, 0x01, 0x02, 0x00, 0x9d, 0x6a};
char uart_rx_buffer[8] = {0};

char Num = 0;
char stringnum = 0;
unsigned long W5500DataNum = 0;
unsigned long Send_Num_Ok = 0;
unsigned long Rec_Num = 0;
unsigned long Rec_Num_Ok = 0;
long starttime;

String MikroTikPrompt = "[admin@MikroTik] > ";
IPAddress  telnetServer(10, 18, 11, 240);
EthernetClient client;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress nm(255, 255, 255, 0); 
IPAddress gw(10, 18, 11, 254);
IPAddress dnsss( 8, 8, 8, 8); 
IPAddress ip(10, 18, 11, 197);

 

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
//EthernetServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);
  // Open serial communications and wait for port to open:
  M5.begin(true, false, true);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  SPI.begin(SCK, MISO, MOSI, -1);
  delay(1000);
  Ethernet.init(CS);
  // start the Ethernet connection and the server:
  delay(1000); 
  Ethernet.begin(mac, ip, dnsss, gw , nm);
  //server.begin();
  //Serial.print("server is at ");
  //Serial.println(Ethernet.localIP());
  M5.Lcd.setFreeFont(FSS9);
  M5.Lcd.println(" ");
  M5.Lcd.println("M5Stack W5500 Test");
  M5.Lcd.print(Ethernet.localIP());
  delay(1000);
  M5.Lcd.println("");
  M5.Lcd.println("connecting...");
//  while(!client){
//    ; // wait until there is a client connected to proceed
//  }
  if (client.connect(telnetServer, 23)) {
    M5.Lcd.println("connected");
//   client.println("Arn.Preg:123:");
//   client.println();
  } else {
    M5.Lcd.println("connection failed");
  }
  delay (1000);
}

int value = 0;
String commands;
String prompt;
String sbuffer;
int LoggedIn = 0;
int CommandNo;

void loop(){
  if (client.available() > 0) {
    char c = client.read();
    if (c < 32 || c > 128 ) {
      prompt = "";
      if (c==13 && LoggedIn==1){
        sbuffer.replace("[9999B", "");
        Serial.println(sbuffer);
        sbuffer = "";
      }
    } else
    {    
      prompt+=c;
      if (LoggedIn == 1) sbuffer+=c;
      //Serial.print(prompt);
    }
    commands+="0x" + String(c,HEX)+" ";
    //Serial.println(commands);
    //Serial.println(prompt);
    //Serial.println(LoggedIn);
  }
 //  M5.Lcd.print(commands);  
  Serial.print(commands);
  if (commands == "0xff 0xfd 0x18 0xff 0xfd 0x20 0xff 0xfd 0x23 0xff 0xfd 0x27 ") {
      Serial.println();
      Serial.println("CO | authorize | Received Phrase 1");
      byte buf[] = {255, 251, 24, 255, 251, 31};
      client.write(buf, sizeof(buf));
      commands = "";
  }
  if (commands == "0xff 0xfd 0x1f ") {
      Serial.println();
      Serial.println("CO | authorize | Received Phrase 2");
      //byte buf[9];
      byte buf[] = {255, 252, 32, 255, 252, 35,255,252,39};
      client.write(buf, sizeof(buf));
      commands = "";
  }

  if (commands == "0xff 0xfa 0x18 0x1 0xff 0xf0 ") {
      Serial.println();
      Serial.println("CO | authorize | Received Phrase 3");
      //byte buf[9];
      byte buf[] = {255,250,31,0,120,0,30,255,240};
      client.write(buf, sizeof(buf));
      // second zero was 00
      byte buf2[] = {255,250,39,0,255,240,255,250,24,0,65,78,83,73,255,240};
      client.write(buf2, sizeof(buf2));
      commands = "";
  } 
  if (commands == "0xff 0xfb 0x3 0xff 0xfd 0x1 0xff 0xfb 0x5 0xff 0xfd 0x21 ") {
      Serial.println();
      Serial.println("CO | authorize | Received Phrase 4");
      byte buf[] = {255,250};
      client.write(buf, sizeof(buf));
      //yte buf2[9];
      byte buf2[] = {255,252,1,255,254,5,255,252,33};
      client.write(buf2, sizeof(buf2));
      commands = "";
  }
  if (prompt == "Login: "){
    Serial.println();
    Serial.println("CO | authorize | Login!!!");
    client.println("admin+tc");
    commands = "";
    prompt = "";  
  }
  if (prompt == "Password:"){
    Serial.println();
    Serial.println("CO | authorize | Password!!!");
    client.println("heslo");
    LoggedIn = 1;
    commands = "";
    prompt = "";  
    value = 1;
    CommandNo = 1;
    starttime = millis();
  }
 
  if (LoggedIn == 1 && commands.length()>0) {
    if (MikroTikPrompt.substring(0,prompt.length()) == prompt) {
      if ( MikroTikPrompt == prompt ) {
        //Serial.println(commands);
        if (CommandNo == 1) client.println("/interface ethernet poe monitor ether4 once");
        if (CommandNo == 2) {
          client.println("/interface ethernet poe set ether4 poe-out=force");
          delay(1000);
        }
        if (CommandNo == 3) client.println("/interface ethernet poe monitor ether4 once");
        if (CommandNo == 4) {
          client.println("/interface ethernet poe set ether4 poe-out=off");       
          delay(1000);         
        }
        if (CommandNo == 2) client.println("quit");
        CommandNo++;      
      }
      commands = "";     
    } else {
      commands = "";
      prompt = "" ;
    }
  }
  if (value == 1 && millis()-starttime>25000) {
    Serial.println();
    Serial.println("exit!!!");
    client.println("");
    client.println("quit");
    value = 0;
  }
  if (!client.connected()) {
    M5.Lcd.println();
    M5.Lcd.println("disconnecting.");
    client.stop();
    for(;;)
      ;
  }
  //delay (10);
} 
