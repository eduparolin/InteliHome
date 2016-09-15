#include <avr/pgmspace.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <CapacitiveSensor.h>
CapacitiveSensor c = CapacitiveSensor(9, 10);
SoftwareSerial Serial1(8, 7);
unsigned long prev = 0; // last time update
long intervalo = 30000;
int ff = 1;
int calibration = 0;
int ndelay = 0;
int ledStatus = 0;
int mode = 0;
String code = "";
int randNumber = 0;
int ligado = LOW;
unsigned long csSum;
String nome = "", password = "";

//#--------SETUP-------#//

void setup() {
  Serial.begin(19200);
  Serial1.begin(19200);
  pinMode(A1, OUTPUT);
  pinMode(12, INPUT);

  if (EEPROM.read(0) == '0')mode = 0;
  if (EEPROM.read(0) == '1')mode = 1;

  simpleCom(F("AT+RST\r\n"), 2000);
  simpleCom(F("AT+CIPMUX=1\r\n"), 500);
  if (mode == 0) {
    simpleCom(F("AT+CWMODE=3\r\n"), 500);
    simpleCom(F("AT+CWSAP_CUR=\"InteliHome\",\"12345678\",10,3\r\n"), 2000);
  } else if (mode == 1) {
    code = EEPROMReadlong(1);
    simpleCom(F("AT+CWMODE=1\r\n"), 7500);
  }
  delay(100);
  simpleCom(F("AT+CIPSERVER=1,5566\r\n"), 500);
}

//#--------LOOP-------#//

void loop() {
  void(* resetFunc) (void) = 0;
  if (mode == 0) {
    zeroCom();
  } else if (mode == 1) {
    //simpleCom("", 500);
    CSread();
    oneCom();
    if (checkCon() == '5') {
      resetFunc();
    }
  }
  if (digitalRead(12) == HIGH) {
    EEPROM.write(0, '0');
    resetFunc();
  }
}

//#--------Verifica Conexão-------#//

char checkCon() {
  unsigned long cur = millis();
  char stats = '0';
  if (cur - prev > intervalo) {
    prev = cur;
    String status = simpleCom(F("AT+CIPSTATUS\r\n"), 350);
    int index = status.indexOf("STATUS:");
    stats = status[index + 7];
  } return stats;

}

//#--------Espera por clique touch-------#//

void CSread() {
  long cs;
  if (calibration != 0) {
    cs = c.capacitiveSensor(calibration);
  } else {
    cs = c.capacitiveSensor(30); //a: Sensor resolution is set to 80
  }
  if (cs > 30) { //b: Arbitrary number
    csSum += cs;
    int dl;
    if (ndelay == 0) {
      dl = 120;
    } else {
      dl = ndelay;
    }
    //Serial.println(cs);
    if (csSum >= dl) //c: This value is the threshold, a High value means it takes longer to trigger
    {
      ff = 1;
      //Serial.print("Trigger: ");
      //Serial.println(csSum);
      if (csSum > 0) {
        csSum = 0;
        if (ligado == LOW && ff == 1) {
          ligado = HIGH;
          ledStatus = HIGH;
          ff = 0;
          digitalWrite(A1, ledStatus);
        }
        if (ligado == HIGH && ff == 1) {
          ligado = LOW;
          ff = 0;
          ledStatus = LOW;
          digitalWrite(A1, ledStatus);
        }
      } //Reset
      c.reset_CS_AutoCal();
    }
  } else {
    csSum = 0;
  }
}

//#--------Envia pedido simples-------#//

String simpleCom(const __FlashStringHelper* comm, int timeout) {
  Serial.print(comm);
  delay(7);
  String response = "";
  long int time = millis();
  while ( (time + timeout) > millis()) {
    while (Serial1.available()) {
      char r = Serial1.read();
      response += r;
    }
  }
  //Serial.println(response);
  return response;
}

//#--------Pedididos de modo 0-------#//

void zeroCom() {
  int timeout = 500;
  int count = 0;
  String response = "";
  long int time = millis();
  while ( (time + timeout) > millis()) {
    while (Serial1.available()) {
      char r = Serial1.read();
      if (count < 64) {
        response += r;
        count ++;
      }
    }
  }
  char id = 'N';
  int index = response.indexOf("+IPD,");
  if (index != -1) {
    id = response[index + 5];
  }
  index = response.indexOf("/&");
  if (index != -1 && id != 'N') {
    nome = "";
    password = "";
    int pos = 0;
    long safety = millis();
    for (int i = index + 2; response[i] != '&'; i++) {
      if (millis() - safety > 500)break;
      nome += response[i];
      pos = i;
    }
    safety = millis();
    for (int i = pos + 2; response[i] != '&'; i++) {
      if (millis() - safety > 500)break;
      password += response[i];
    }
    //getAddr();
    closeRespCom(id, "CONF");
    createSTA(nome, password);
  }
  index = response.indexOf("GEIP");
  if (index != -1 && id != 'N') {
    mode = 2;
    getAddr(id);
    //closeCom(id);
  }
  index = response.indexOf("DONE");
  if (index != -1 && id != 'N') {
    //createSTA(nome, password);
    closeRespCom(id, "");
    simpleCom(F("AT+CWMODE=1\r\n"), 500);
    EEPROM.write(0, '1');
    mode = 1;
  }
  //Serial.println(response);
}

//#--------Obtem ip-------#//

void getAddr(char id) {
  String response = simpleCom(F("AT+CIFSR\r\n"), 1500);
  String ip = "";
  int index = response.indexOf("STAIP,\"");
  if (index != -1) {
    for (int i = index + 7; response[i] != '\"'; i++) {
      ip += response[i];
    }
  } while (randNumber < 100) {
    randomSeed(analogRead(0));
    randNumber = random(300);
  }
  EEPROMWritelong(1, randNumber);
  code = String(randNumber);
  //Serial.println(randNumber);
  closeRespCom(id, ip + "+" + String(randNumber));
  mode = 0;
}

//#--------Faz conexão com o roteador-------#//

void createSTA(String nome1, String pass) {
  simpleCom(F("AT+CWJAP_DEF=\""), 50);
  Serial.print(nome1);
  simpleCom(F("\",\""), 50);
  Serial.print(pass);
  simpleCom(F("\"\r\n"), 2000);
}

//#--------Pedididos de modo 1 (Depois de configurado)-------#//

void oneCom() {
  int timeout = 50;
  int count = 0;
  String response = "";
  long int time = millis();
  while ( (time + timeout) > millis()) {
    while (Serial1.available()) {
      char r = Serial1.read();
      //response += r;
      if (count < 64) {
        response += r;
        count ++;
      }
    }
  }
  String resp = "";
  char id = 'N';
  int index = response.indexOf("+IPD,");
  if (index != -1) {
    id = response[index + 5];
  }
  index = response.indexOf("/H" + code);
  if (index != -1) {
    ledStatus = 1;
    ff = 0;
    digitalWrite(A1, ledStatus);
  }
  index = response.indexOf("/L" + code);
  if (index != -1) {
    ff = 0;
    ledStatus = 0;
    digitalWrite(A1, ledStatus);
  }
  index = response.indexOf("/c" + code);
  if (index != -1) {
    int pos = 0;
    String newSense = "";
    String newDelay = "";
    long safety = millis();
    for (int i = index + 5; response[i] != '&'; i++) {
      if (millis() - safety > 500)break;
      newSense += response[i];
      pos = i;
    }
    safety = millis();
    calibration = newSense.toInt();
    for (int i = pos + 2; response[i] != '&'; i++) {
      if (millis() - safety > 500)break;
      newDelay += response[i];
      //pos=i;
    }
    ndelay = newDelay.toInt();
  }
  if (id != 'N') {
    resp += ledStatus;
    closeRespCom(id, resp);
  }
}

//#--------Fecha pedido com resposta-------#//

void closeRespCom(char id, String resp) {
  int tam = resp.length() + 2;
  delay(5);
  simpleCom(F("AT+CIPSEND="), 100);
  Serial.print(id);
  simpleCom(F(","), 25);
  Serial.print(tam);
  simpleCom(F("\r\n"), 50);
  delay(5);
  //simpleCom(F("HTTP/1.1 200 OK\r\n"), 100);
  //simpleCom(F("Content-type:text/html\r\n\r\n"), 100);
  Serial.print(resp);
  simpleCom(F("\r\n"), 500);
  delay(150);
  simpleCom(F("AT+CIPCLOSE="), 250);
  Serial.print(id);
  simpleCom(F("\r\n"), 250);
}

void EEPROMWritelong(int address, long value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

/*//#--------Verifica memória ram-------#//

  int freeRam ()
  {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  }*/

