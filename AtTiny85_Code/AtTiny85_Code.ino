#include <CapacitiveSensor.h>
#include <SoftwareSerial.h>

#define SERIAL_BUFFER 10
char input[SERIAL_BUFFER + 1];

int ff = 1;
int ledStatus = LOW;

String readString;

int calib[2];//calib: pos0 = Sensibilidade; pos1 = Delay;
unsigned long csSum;

unsigned long prev = 0; // last time update
long intervalo = 1500;

CapacitiveSensor c = CapacitiveSensor(1, 2);//(Antena, Referência);
SoftwareSerial Serial1(3, 4);//(RX,TX);

void setup() {
  Serial1.begin(9600);
  pinMode(0, INPUT);
  //Zerando as posições de calib;
  for (int i = 0; i < 2; i++) {
    calib[i] = 0;
  }
}

void loop() {
  CSread();
  while (Serial1.available()) {
    delay(3);
    char c = Serial1.read();
    readString += c;
  }
  readString.trim();
  if (readString.length() > 0) {
    if (readString == "H") {
      ledStatus = 1;
    }
    if (readString == "L")
    {
      ledStatus = 0;
    }
    readString = "";
  }
  //Manda comando reset para ESP quando o pino 0 for HIGH;
  if (digitalRead(0) == HIGH) {
    Serial1.println("Z");
  }
}
void CSread() {
  delay(3);
  long cs;
  if (calib[0] != 0) {
    cs = c.capacitiveSensor(calib[0]);
  } else {
    cs = c.capacitiveSensor(30); //a: Sensor resolution is set to 80
  } //a: Sensor resolution is set to 80
  if (cs > 90) { //b: Arbitrary number
    Serial1.println(String(cs));
    csSum += cs;
    int dl;
    if (calib[1] == 0) {  
      dl = 400; 
    } else {
      dl = calib[1];
    }
    if (csSum >= dl) //c: This value is the threshold, a High value means it takes longer to trigger
    {
      ff = 1;
      csSum = 0;
      if (ledStatus == LOW && ff == 1) {
        ledStatus = HIGH;
        ff = 0;
        Serial1.println("H");
      }
      if (ledStatus == HIGH && ff == 1) {
        ff = 0;
        ledStatus = LOW;
        Serial1.println("L");
      }
      //Reset
      c.reset_CS_AutoCal();
    }
  } else {
    csSum = 0;
  }
}
