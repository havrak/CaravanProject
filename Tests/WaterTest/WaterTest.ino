// class for finding volume of tank, and counting speed of wiper
// will fill the tank, than you will drain it liter after liter

  

#define ANALOG_PIN 14 // potentiometer wiper (middle terminal) connected to analog pin 3

                    // outside leads to ground and +5V

long pulseCount = 0;
long pulseCountAfterLowHitted = 0;
bool printed = false;

 

void setup() {

  Serial.begin(115200);           //  setup serial
  pinMode(4, OUTPUT);             // rele (HIGH vypnuto, LOW zapnuto
  pinMode(15, INPUT);             // snímač hladiny Horni, LOW sepnuto
  pinMode(13, INPUT);             // snímač hladiny spodní, LOW sepnuto
  pinMode(5, INPUT);              // Prutokomer impulsy
  pinMode(14, INPUT);
  attachInterrupt(5, AddPulse, FALLING);
 
  digitalWrite(4, LOW);
}

 

void loop() {
  if(digitalRead(15) == HIGH){
    pulseCount = 0;
    pulseCountAfterLowHitted = 0;
    digitalWrite(4,HIGH);
  }
  Serial.print("Preasure        :"); Serial.println(analogRead(ANALOG_PIN));
  Serial.print("Level H         :"); Serial.println(digitalRead(15));
  Serial.print("Level L         :"); Serial.println(digitalRead(13));
  Serial.print("Pulses before L :"); Serial.println(pulseCount);
  Serial.print("Pulses after L  :"); Serial.println(pulseCountAfterLowHitted);
  delay(1000);
}

 

 

void AddPulse() {
  if(digitalRead(15) == LOW && digitalRead(13) == HIGH){
    pulseCount++; 
  }else if(digitalRead(15) == LOW && digitalRead(13) == LOW){
    if(!printed){
      Serial.println("---------------||---------------");
      Serial.print("Pulzů finální  :"); Serial.println(pulseCount);
      Serial.println("---------------||---------------");
      printed = true;
    }
    pulseCountAfterLowHitted++;
  }
} 
