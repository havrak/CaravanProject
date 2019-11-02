
// Problem: after write client is never available
#include <ETH.h>
#include <WiFi.h>

#define DEBUG_PRINT(x)  Serial.println (x)

WiFiClient client;

bool isTelnetConnectionRunning = false;
bool didIAuthorized = false;

IPAddress server(192,168,1,4);

void setup() {
   // seems to be needed
  //WiFi.config(ip_static, ip_gateway, ip_subnet, ip_dns1, ip_dns2);

  WiFi.begin("Atingo", "GoodByeD2016");
  while (WiFi.status() != WL_CONNECTED) { // add counetr and kill
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  //WiFi.onEvent(WiFiEvent);
  ETH.begin();
  WiFi.mode(WIFI_STA);

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  setPromptChar('>');
//  if(client.connect(server,23)){
//    client.write("admin");
//    while(client.available() == 0) delay(1);
//    Serial.println("Client is ava");
//  } 
  if(login(server, "pi", "raspberry",23)){
    Serial.println("Got past loggin");
    sendCommand("ls");
  }
  disconnect();


}
int i = 0;
void loop() {
  //Serial.println("C");
  //if(client.connect({192,168,1,6},23)){
  //  Serial.println("connected");
  //}
  //Serial.println("Cc");
  //IPAddress ip(79,125,105,113);
  //Serial.print("1");
  //if(client.connect("duckduckgo.com",80)){
  //  Serial.println("Connected");
  //}
  //Serial.print("2");
}


// private:
const uint8_t NEGOTIATION_DELAY = 100;

////////////////CONFIGURATION////////////////////////////////////////////////////////////////////////////////////////////////////
//how long the command sent may be long
const uint8_t MAX_OUT_BUFFER_LENGTH = 150;
//how long you'll wait for an expected answer from the server
const unsigned int LISTEN_TOUT = 5000;
//how long, after a "prompt char" is received you can confirm it's the real prompt and not just part of the server's answer
const uint16_t PROMPT_REC_TOUT = 300;

char m_promptChar = '>'; 

bool login(IPAddress serverIpAddress, const char* username, const char* password, uint8_t port){

  Serial.print('\n'); // prints \n
  Serial.print('\r'); // pritns form read

  DEBUG_PRINT(F("login|connecting..."));
  if(client.connect(serverIpAddress, port)){ // tries to connect to server
    // this shloud work even with wifi client
    DEBUG_PRINT(F("login|connected!"));
    //here there will be the initial negotiation
    //listenUntil(':');
    listen();
    DEBUG_PRINT(F("login|sending username"));
    client.write("pi");
    //if (!send(username, false)) return false;
    while(client.available() == 0) delay(1);
    Serial.println("Client is ava");
    //if (!send(password, false)) return false;
    //if (!this->send(username, false)) return false;
    DEBUG_PRINT(F("login|username sent"));
    listenUntil(':');
    DEBUG_PRINT(F("login|sending password"));
    if (!send(password, false)) return false;

    #ifdef MT_VM
    //mikrotik router with demo license
    listenUntil('!');
    send("", false);
    #endif

    return waitPrompt();
    //listen();
    //return true;


  }
  else{
    DEBUG_PRINT(F("login|connection failed!"));
    return false;
  }
}


bool sendCommand(const char* cmd){
  send(cmd,false);
  if (strcmp(cmd, "exit") != 0){ // if command is not exit we will wait for prompt
    return waitPrompt();
  }
  else{ // else just disconnect
    disconnect();
    return true;
  }

}
void disconnect(){
  client.stop();
}

bool send(const char* buf, bool waitEcho){
  DEBUG_PRINT(F("send|START"));
  uint8_t l_size = strnlen(buf, MAX_OUT_BUFFER_LENGTH);
  Serial.println(l_size);
  if(l_size == MAX_OUT_BUFFER_LENGTH){
    DEBUG_PRINT(F("send|BAD INPUT"));
    return false;
  }

  char l_outBuffer[MAX_OUT_BUFFER_LENGTH];
  strlcpy(l_outBuffer, buf, MAX_OUT_BUFFER_LENGTH);
  if(strlcat(l_outBuffer, "\r\n", MAX_OUT_BUFFER_LENGTH) >= MAX_OUT_BUFFER_LENGTH){
    DEBUG_PRINT(F("send|BAD INPUT"));
    return false;
  }

  l_size = strnlen(l_outBuffer, MAX_OUT_BUFFER_LENGTH);
  for (uint8_t i = 0; i < l_size; ++i){
    if(l_outBuffer[i] > 0){
      client.write(l_outBuffer[i]);
      Serial.println(l_outBuffer[i]);
      // does not run
      if (waitEcho){
        while (client.available() == 0) delay (1);
        char inByte = client.read();
        DEBUG_PRINT(F("send|inByte"));
        Serial.println(inByte);
      }
    }
    client.write(""); // send empty 
  }

  //this->print('\r');
  return true;
}

void negotiate(){
  // byte was IAC  (Interpret as Command)
  byte verb, opt;
  byte outBuf[3] = {255, 0, 0};

    // == -1 or == 128
  DEBUG_PRINT(F("negotiate|server:IAC"));
  verb = client.read (); // we will read next byte from client
  //if (verb == - 1) return; //     ?? how can it be -1, when we are able to reach 255
  switch (verb) { // we got trought possible telnet "erros"
    case 255:// some weard situation
      //...no it isn't!
      DEBUG_PRINT(F("negotiate|server:IAC escape"));
      Serial.print(char (verb));
    break;
    case 251:
      //to a WILL statement...
      DEBUG_PRINT(F("negotiate|server:WILL"));
      opt = client.read();
      //if (opt == -1) break;
      DEBUG_PRINT(F("negotiate|server opt: "));
      DEBUG_PRINT(opt);
      //always answer DO!
      outBuf[1] = 253;
      outBuf[2] = opt;
      // in WiFi only one argument
      //this->client->write(outBuf, 3); // writes data in outBuff
      // replace with only one arg
      client.write(outBuf[0]); // writes data in outBuff
      client.write(outBuf[1]); // writes data in outBuff
      client.write(outBuf[2]); // writes data in outBuff
      client.flush();          // we discard what client didn't read
      DEBUG_PRINT(F("negotiate|client:IAC"));
      DEBUG_PRINT(F("negotiate|client:DO"));
    break;
    case 252:
      DEBUG_PRINT(F("negotiate|server:WONT"));
    break;
    case 253:
      //to a DO request...
      DEBUG_PRINT(F("negotiate|server:DO"));
      opt = client.read();
      //if (opt == -1) break;
      DEBUG_PRINT(F("negotiate|server opt: "));
      DEBUG_PRINT(opt);
      //alway answer WONT!
      outBuf[1] = 252;
      outBuf[2] = opt;
      client.write(outBuf[0]); // writes data in outBuff
      client.write(outBuf[1]); // writes data in outBuff
      client.write(outBuf[2]); // writes data in outBuff
      client.flush();
      DEBUG_PRINT(F("negotiate|client:IAC"));
      DEBUG_PRINT(F("negotiate|client:WONT"));
    break;
    case 254:
      DEBUG_PRINT(F("negotiate|server:DONT"));
    break;
  }

}

void listen(){
  while (client.available() == 0) delay(1); // wait until client is avaiable
  byte inByte;
  unsigned long startMillis = millis();

  while(1){ // read unit we have something to read
    if (client.available() > 0){
      startMillis = millis(); // we are still reading prevent timeout
      inByte = client.read (); // we read byte form client
      Serial.println(inByte);
      if (inByte <= 0){ // unexpected situation, client error????
        //DEBUG_PRINT(F("listen|what?"));
      }
      else if(inByte == 255){// if client sends 255 we will negotiate with client
        negotiate();
      }
      else{
        //is stuff to be displayed
        Serial.print(char(inByte)); // otherwise we will just dispaly char
      }
    }
    else if (millis() - startMillis > LISTEN_TOUT){ // exit loop in case of timeout
      DEBUG_PRINT(F("listen|TIMEOUT!!!"));
      return;
    }
  }
}

bool listenUntil(char c){

  byte inByte;

  DEBUG_PRINT(F("listenUntil|WAITING FOR CLIENT"));

  while (client.available() == 0) delay (1); // does not ever happen
  DEBUG_PRINT(F("listenUntil|CLIENT AVAILABLE"));

  do {
    if(client.available() > 0){
      inByte = client.read();
      if (inByte <= 0){
        //DEBUG_PRINT(F("listen|what?"));
      }
      else if(inByte == 255){
        negotiate();
      }
      else{
        //is stuff to be displayed
        Serial.print(char(inByte));
      }
      if (char(inByte) == c){
        DEBUG_PRINT(F("listenUntil|TERMINATOR RECEIVED"));
        return true;
      }
    }
  }while (1);

}

bool waitPrompt(){

  bool l_bLoop = false;
  unsigned long startMillis = millis();

  do
  {
    if (!listenUntil(m_promptChar)) return false; //return false if prompt chara is not found
    char l_lastByte = client.read();
    DEBUG_PRINT(F("waitPrompt|L_lastbyte"));
    Serial.println(l_lastByte);

    do
    {
      l_bLoop = client.available() > 0;
      if (l_bLoop){
        DEBUG_PRINT(F("waitPrompt|FALSE PROMPT DETECTED"));
        Serial.print('\r');
        //this->print('\n');
        break;
      }
    }while(millis()-startMillis < PROMPT_REC_TOUT);

  }while(l_bLoop);

  //this->print('\n');
  //this->print('\r');
  DEBUG_PRINT(F("waitPrompt|END"));
  return true;
}

void setPromptChar(char c){
  m_promptChar = c;
}
