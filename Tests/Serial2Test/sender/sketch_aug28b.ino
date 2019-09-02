void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1,16,17);
  //Serial2.print("baud=115200");
  //Serial2.write(0xff);  // We always have to send this three lines after each command sent to nextion.
  //Serial2.write(0xff);
  //Serial2.write(0xff);
  
  //Serial2.end();  // End the serial comunication of baud=9600
  //Serial2.begin(115200, SERIAL_8N1,16,17);  // Start serial comunication at baud=115200
  
  //Serial2.write(0xff);
  //Serial2.write(0xff);
  //Serial2.write(0xff);
}

void loop() {
  // send data only when you receive data:
  Serial.print("ttt");
  String command;
  command= "t0.txt=\"odpojena\"";
  Serial2.print(command);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  delay(1000);
  command= "t0.txt=\"odpossjena\"";
  Serial2.print(command);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  delay(1000);
}
