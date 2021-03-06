//FUNCIONANDO 06/10/16 01:55

#include <CapacitiveSensor.h>
#include <SoftwareSerial.h>
CapacitiveSensor c = CapacitiveSensor(9, 10);
SoftwareSerial Serial1(8, 7);
#define SERIAL_SIZE 16
int ff = 1;
int ledStatus = 0;
unsigned long csSum;
String readString;
int calib[2];
char input[SERIAL_SIZE + 1];
unsigned long prev = 0; // last time update
long intervalo = 30000;
//String input = "";
void setup() {
  // put your setup code here, to run once:
  Serial1.begin(9600);
  Serial.begin(9600);
  Serial1.setTimeout(100);
  //Serial.println("Iniciando");
  pinMode(A1, OUTPUT);
  pinMode(12, INPUT);
  for (int i = 0; i < 2; i++) {
    calib[i] = 0;
  }
}

void checkCon() {
  unsigned long cur = millis();
  char stats = '0';
  if (cur - prev > intervalo) {
    prev = cur;
    String send = (ledStatus) ? "H" : "L";
    Serial.println(send);
  }
}

void loop() {
  checkCon();
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
      //Serial.println("H");
      digitalWrite(A1, HIGH);
    }
    if (readString == "L")
    {
      ledStatus = 0;
      //Serial.println("L");
      digitalWrite(A1, LOW);
    }

    readString = "";
  }

  /*if (Serial1.available()) {
    char *teste;
    byte tam = Serial1.readBytes(input, SERIAL_SIZE);
    Serial.println(input);
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
        teste = strtok(NULL, "&");
        count++;
      }
    }
    }
    if (digitalRead(12) == HIGH) {
    Serial.println('Z');
    }*/
  // put your main code here, to run repeatedly:
  if (digitalRead(12) == HIGH) {
    Serial.println('Z');
  }

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
      dl = 250;
    } else {
      dl = calib[1];
    }
    if (csSum >= dl) {
      ff = 1;
      csSum = 0;
      if (ledStatus == LOW && ff == 1) {
        ledStatus = HIGH;
        ff = 0;
        digitalWrite(A1, ledStatus);
        Serial.println("H");
      }
      if (ledStatus == HIGH && ff == 1) {
        ff = 0;
        ledStatus = LOW;
        digitalWrite(A1, ledStatus);
        Serial.println("L");
      }
      c.reset_CS_AutoCal();
    }
  } else {
    csSum = 0;
  }
}
