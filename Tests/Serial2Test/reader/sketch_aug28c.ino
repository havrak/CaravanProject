char incomingByte = 0; // for incoming serial data
String text;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // opens serial port, sets data rate to 9600 bps2
}

void loop() {
  // send data only when you receive data:
  if (Serial2.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial2.read();
    
    // say what you got:
    Serial.print("I received: ");
    Serial.println(String(incomingByte));
  }
}
