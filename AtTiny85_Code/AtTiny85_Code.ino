#include <CapacitiveSensor.h>
#include <SoftwareSerial.h>

#define SERIAL_BUFFER 10
char input[SERIAL_BUFFER + 1];

int ff = 1;
int ledStatus = 0;
int ligado = LOW;

int calib[2];//calib: pos0 = Sensibilidade; pos1 = Delay;
unsigned long csSum;

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
  //delay(1);
  //Lê os possíveis comandos de serial;
  if (Serial1.available()) {
    char *teste;
    byte tam = Serial1.readBytes(input, SERIAL_BUFFER);
    if (input[0] == 'H') {
      //Luz ligada;
      ligado = HIGH;
    } else if (input[0] == 'L') {
      //Luz desligada;
      ligado = LOW;
    } else {
      //Lê as mudanças de sensibilidade do touch;
      teste = strtok(input, "&");
      int count = 0;
      while (teste != 0) {
        calib[count] = atoi(teste);
        Serial1.println(calib[count]);
        teste = strtok(NULL, "&");
        count++;
      }
    }
  }
  //Manda comando reset para ESP quando o pino 0 for HIGH;
  if (digitalRead(0) == HIGH) {
    Serial1.println("Z");
  }
}
void CSread() {
  long cs;
  if (calib[0] != 0) {
    cs = c.capacitiveSensor(calib[0]);
  } else { 
    cs = c.capacitiveSensor(30); //a: Sensor resolution is set to 80
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
