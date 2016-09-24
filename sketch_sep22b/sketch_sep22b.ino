#include <CapacitiveSensor.h>
#include <SoftwareSerial.h>
int ff = 1;
int ledStatus = 0;
int ligado = LOW;
unsigned long csSum;
String teste = "";
int calibration = 0;
int ndelay = 0;
CapacitiveSensor c = CapacitiveSensor(1, 2);
SoftwareSerial Serial1(3,4);
void setup() {
  // put your setup code here, to run once:
  Serial1.begin(9600);
  pinMode(0,INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  CSread();
  delay(5);
  if(Serial1.available()){
    teste = "";
    while(Serial1.available()){
      char c = char(Serial1.read());
      teste += c;
      delay(1);
    }
    //Serial1.println(teste);
    int index = teste.indexOf("&");
    if(index != -1){
      int pos = 0;
      String newSense = "";
      String newDelay = "";
      long safety = millis();
      for (int i = index+1; teste[i] != '&'; i++) {
        if (millis() - safety > 500)break;
        newSense += teste[i];
        pos = i;
      }
      safety = millis();
      calibration = newSense.toInt();
      for (int i = pos + 2; teste[i] != '&'; i++) {
        if (millis() - safety > 500)break;
        newDelay += teste[i];
      }
      ndelay = newDelay.toInt();
       //Serial1.println(newSense+" - "+newDelay);
    }
    if(teste.indexOf("H") != -1){
      ligado = HIGH;
    }
    if(teste.indexOf("L") != -1){
      ligado = LOW;
    }
  
  }
  if (digitalRead(0) == HIGH) {
    //EEPROM.write(0, '0');
    //resetFunc();
    Serial1.println("Z");
  }
}
void CSread() {
  long cs;
  if (calibration != 0) {
    cs = c.capacitiveSensor(calibration);
  } else {
    cs = c.capacitiveSensor(30); //a: Sensor resolution is set to 80
  } //a: Sensor resolution is set to 80
  if (cs > 30) { //b: Arbitrary number
    csSum += cs;
    int dl;
    if (ndelay == 0) {
      dl = 120;
    } else {
      dl = ndelay;
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
          //digitalWrite(A1, ledStatus);
          Serial1.println("H");
        }
        if (ligado == HIGH && ff == 1) {
          ligado = LOW;
          ff = 0;
          ledStatus = LOW;
          //digitalWrite(A1, ledStatus);
          Serial1.println("L");
        }
      } //Reset
      c.reset_CS_AutoCal();
    }
  } else {
    csSum = 0;
  }
}
