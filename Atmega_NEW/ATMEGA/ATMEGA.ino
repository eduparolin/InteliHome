#include <CapacitiveSensor.h>
#include <SoftwareSerial.h>
CapacitiveSensor c = CapacitiveSensor(9, 10);
SoftwareSerial Serial1(8, 7);
#define SERIAL_SIZE 16
int ff = 1;
int ledStatus = 0;
int ligado = LOW;
unsigned long csSum;
int calib[2];
char input[SERIAL_SIZE + 1];
void setup() {
  // put your setup code here, to run once:
  Serial1.begin(9600);
  Serial.begin(9600);
  pinMode(A1, OUTPUT);
  pinMode(12, INPUT);
  for (int i = 0; i < 2; i++) {
    calib[i] = 0;
  }
}

void loop() {
  CSread();
  if (Serial1.available()) {
    char *teste;
    byte tam = Serial1.readBytes(input, SERIAL_SIZE);
    if (input[0] == 'H') {
      //Serial.println("HIGH");
      digitalWrite(A1, HIGH);
      ledStatus = HIGH; 
    } else if (input[0] == 'L') {
      digitalWrite(A1, LOW);
      ledStatus = LOW;
    } else {
      teste = strtok(input, "&");
      int count = 0;
      while (teste != 0) {
        calib[count] = atoi(teste);
        //Serial.println(calib[count]);
        teste = strtok(NULL, "&");
        count++;
      }
    }
  }
  if (digitalRead(12) == HIGH) {
    Serial.println("Z");
  }
  // put your main code here, to run repeatedly:

}
void CSread() {
  long cs;
  if (calib[0] != 0) {
    cs = c.capacitiveSensor(calib[0]);
  } else {
    cs = c.capacitiveSensor(20); //a: Sensor resolution is set to 80
  } //a: Sensor resolution is set to 80
  if (cs > 30) { //b: Arbitrary number
    csSum += cs;
    int dl;
    if (calib[1] == 0) {
      dl = 150;
    } else {
      dl = calib[1];
    }
    if (csSum >= dl) //c: This value is the threshold, a High value means it takes longer to trigger
    {
      ff = 1;
      if (csSum > 0) {
        csSum = 0;
        if (ligado == LOW && ff == 1) {
          ligado = HIGH;
          ledStatus = HIGH;
          ff = 0;
          digitalWrite(A1, ledStatus);
          //Serial.println("H");
        }
        if (ligado == HIGH && ff == 1) {
          ligado = LOW;
          ff = 0;
          ledStatus = LOW;
          digitalWrite(A1, ledStatus);
          //Serial.println("L");
        }
      } //Reset
      c.reset_CS_AutoCal();
    }
  } else {
    csSum = 0;
  }
}
