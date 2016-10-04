#include <EEPROM.h>
#include <ESP8266WiFi.h>
String ip = "";
String resp = "";
String teste = "";

int val = 1;

const char* ssid = "InteliHome";
const char* password = "12345678";

int randNumber = 0;

boolean conn = false;

String nssid = "";
String npassword = "";

int modo = 0;

const char* host = "192.168.25.18";

unsigned long prev = 0; // last time update
long intervalo = 120000;

void checkCon() {
  unsigned long cur = millis();
  char stats = '0';
  if (cur - prev > intervalo) {
    prev = cur;
    sendCom();
    if (WiFi.status() != WL_CONNECTED)ESP.restart();
    else Serial.println("Ainda conectado..");
  }
}

void sendCom() {
  WiFiClient sender;
  const int httpPort = 80;
  if (!sender.connect(host, httpPort)) {
    Serial.println("connection failed");
    //return;
  }
  String url = String(randNumber);
  url += " - ";
  url += (val) ? "0" : "1";
  sender.print(url);
  sender.stop();
}

void EEPROMWritelong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
  EEPROM.commit();
}

long EEPROMReadlong(long address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void putEEPROMWiFi(String _ssid, String _password) {
  for (int i = 0; i < _ssid.length(); i++) {
    EEPROM.write(i + 10, _ssid.charAt(i));
  }
  EEPROM.write(_ssid.length() + 10, '&');
  for (int i = 0; i < _password.length(); i++) {
    EEPROM.write(i + 100, _password.charAt(i));
  }
  EEPROM.write(_password.length() + 100, '&');
  EEPROM.commit();
}
void readEEPROMWiFi() {
  for (int i = 0; EEPROM.read(i + 10) != '&'; i++) {
    nssid += (char)EEPROM.read(i + 10);
  }
  for (int i = 0; EEPROM.read(i + 100) != '&'; i++) {
    npassword += (char)EEPROM.read(i + 100);
  }
}

String ipToString(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    delay(1);
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(5566);

void setup() {
  WiFi.disconnect();
  Serial.begin(9600);
  EEPROM.begin(512);
  //EEPROM.write(0,'0');
  //EEPROM.commit();
  delay(10);

  if (EEPROM.read(0) == (char)'0')modo = 0;
  if (EEPROM.read(0) == (char)'1')modo = 1;

  // prepare GPIO2
  pinMode(2, OUTPUT);
  val = 1;
  digitalWrite(2, val);

  // Connect to WiFi network
  if (modo == 0) {
    WiFi.mode(WIFI_AP_STA);
    Serial.println();
    Serial.println();
    Serial.print("AP: ");
    Serial.println(ssid);

    WiFi.softAP(ssid, password);
    Serial.println("");
    Serial.println("AP Criado !");
  } else if (modo == 1) {
    randNumber = EEPROMReadlong(1);
    readEEPROMWiFi();
    delay(10);
    WiFi.begin(nssid.c_str(), npassword.c_str());
    int times = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(1);
      if (millis() - times > 500) {
        times = millis();
        //ESP.restart();
        Serial.print(".");
      }
    }
    Serial.println("Conectado ao roteador !");
  }
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  //Serial.println(WiFi.localIP());
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    checkCon();
    if (Serial.available()) {
      teste = "";
      while (Serial.available()) {
        teste += char(Serial.read());
      }
    }
    //Serial.println(teste);
    if (teste == "H") {
      val = 0;
      digitalWrite(2, val);
      teste = "";
    }
    if (teste == "L") {
      val = 1;
      digitalWrite(2, val);
      teste = "";
    }
    if (teste == "Z") {
      EEPROM.write(0, '0');
      EEPROM.commit();
      ESP.restart();
      teste = "";
    }
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request

  if (modo == 0) {
    if (req.indexOf("&") != -1) {
      for (int i = 0; req[i] != '&'; i++) {
        nssid += req[i];
      }
      nssid.trim();
      Serial.println(nssid);
      int pos = req.indexOf("&") + 1;
      for (int i = pos; i < req.length(); i++) {
        npassword += req[i];
      }
      npassword.trim();
      Serial.println(npassword);
      WiFi.begin(nssid.c_str(), npassword.c_str());
      int times = millis();
      while (WiFi.status() != WL_CONNECTED) {
        delay(1);
        //delay(500);
        //Serial.print(".");
        if (millis() - times > 500) {
          times = millis();
          //ESP.restart();
          Serial.print(".");
        }
      }
      putEEPROMWiFi(nssid, npassword);
      //conn = true;
    }
    else if (req.indexOf("/GETIP") != -1) {
      conn = true;
      ip = "";
      ip = ipToString(WiFi.localIP());
      ip += "+";
      randomSeed(millis());
      randNumber = random(100, 999);
      ip += String(randNumber);
      ip += "+";
      ip += String(1);
      ip += "\n";
      Serial.println(ip);
      EEPROMWritelong(1, randNumber);
    }
    else {
      Serial.println("invalid request");
      client.stop();
      return;
    }
  } else {
    if (req.indexOf("/H" + String(randNumber)) != -1) {
      resp = "";
      resp += "1";
      val = 0;
      digitalWrite(2, val);
      Serial.println("H\n");
    }
    else if (req.indexOf("/L" + String(randNumber)) != -1) {
      resp = "";
      resp += "0";
      val = 1;
      digitalWrite(2, val);
      Serial.println("L\n");
    }
    else if (req.indexOf("/c" + String(randNumber)) != -1) {
      int index = req.indexOf("/c" + String(randNumber));
      int pos = 0;
      String newSense = "";
      String newDelay = "";
      long safety = millis();
      for (int i = index + 5; req[i] != '&'; i++) {
        if (millis() - safety > 500)break;
        newSense += req[i];
        pos = i;
      }
      safety = millis();
      for (int i = pos + 2; req[i] != '&'; i++) {
        if (millis() - safety > 500)break;
        newDelay += req[i];
        //pos=i;
      }
      Serial.println(newSense + "&" + newDelay);
    }
    else if (req.indexOf("/o") != -1) {
      resp = "";
      resp += (val) ? "0" : "1";
      Serial.println("L\n");
    }
    else {
      Serial.println("invalid request");
      client.stop();
      return;
    }
  }

  client.flush();

  // Prepare the response
  String s = "";
  if (conn) {
    EEPROM.write(0, '1');
    EEPROM.commit();
    s = ip;
    client.print(s);
    delay(1);
    Serial.println("Disc");
    WiFi.mode(WIFI_STA);
    modo = 1;
    conn = false;
  } else {
    s = resp;
    client.print(s);
    delay(1);
    Serial.println("No client");
  }
  // Send the response to the client


  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed
}

