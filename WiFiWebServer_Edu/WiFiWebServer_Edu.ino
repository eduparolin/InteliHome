/*
    This sketch demonstrates how to set up a simple HTTP-like server.
    The server will set a GPIO pin depending on the request
      http://server_ip/gpio/0 will set the GPIO2 low,
      http://server_ip/gpio/1 will set the GPIO2 high
    server_ip is the IP address of the ESP8266 module, will be
    printed to Serial when the module is connected.
*/

#include <ESP8266WiFi.h>
#include <EEPROM.h>
const char* ssid = "InteliHome";
const char* password = "12345678";
String nome = "", pass = "";
int randNumber = 0;
int modo = 0;
int val = 0;
String teste = "";

String ipToString(IPAddress ip);
void EEPROMWritelong(int address, long value);
long EEPROMReadlong(long address);
void storeSSID(String ss, String pass);

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(5566);

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  //EEPROM.write(0,'0');
  //EEPROM.commit();
  delay(10);
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);
  if (EEPROM.read(0) == '0')modo = 0;
  if (EEPROM.read(0) == '1')modo = 1;
  randNumber = EEPROMReadlong(1);
  if (modo == 0) {
    WiFi.mode(WIFI_AP_STA);
    //Serial.println();
    //Serial.print("Configuring access point...");
    WiFi.softAP(ssid, password);

    IPAddress myIP = WiFi.softAPIP();
    //Serial.print("AP IP address: ");
    //Serial.println(myIP);
  } else {
    // prepare GPIO2
    // Connect to WiFi network
    String ss = "";
    String pass = "";
    for (int i = 5; i < 64; i++) {
      ss += char(EEPROM.read(i));
    }
    for (int i = 64; i < 96; i++) {
      pass += char(EEPROM.read(i));
    }

    /*Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ss.c_str());*/

    WiFi.begin(ss.c_str(), pass.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      //Serial.print(".");
    }
    //Serial.println("");
    //Serial.println("WiFi connected");

  }
  // Start the server
  server.begin();
  //Serial.println("Server started");

  // Print the IP address
  //Serial.println(WiFi.localIP());
}

void loop() {

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    if (Serial.available()) {
      teste = "";
      while (Serial.available()) {
        teste += char(Serial.read());
      }
      //Serial.println(teste);
    }
    if (teste == "H") {
      val = 1;
      digitalWrite(2, val);
      teste = "";
    }
    if (teste == "L") {
      val = 0;
      digitalWrite(2, val);
      teste = "";
    }
    if (teste == "Z") {
      EEPROM.write(0,'0');
      EEPROM.commit();
      ESP.restart();
      teste="";
    }
    return;
  }

  // Wait until the client sends some data
  //Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  //Serial.println(req);
  client.flush();
  if (modo == 0) {
    int index = req.indexOf("/&");
    if (index != -1) {
      ssid = "";
      password = "";
      int pos = 0;
      for (int i = index + 2; req[i] != '&'; i++) {
        ssid += req[i];
        pos = i;
      }
      for (int i = pos + 2; req[i] != '&'; i++) {
        password += req[i];
      }

      //server.stop();

      //WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Serial.print(".");
      }
      //Serial.println("");
      //Serial.println("WiFi connected");
      delay(10);
      client.flush();
      randomSeed(millis());
      randNumber = random(100, 999);
      String s = "";
      s += ipToString(WiFi.localIP());
      s += "+";
      s += String(randNumber);
      s += "+";
      s += String(1);
      s += "\n";
      client.print(s);
      delay(1);
      WiFi.mode(WIFI_STA);
      //Serial.println("");
      //Serial.println("Recriando servidor na porta 5566...");
      server.begin();
      //Serial.println("Servidor rodando!");
      modo = 1;
      EEPROM.write(0, '1');
      EEPROM.commit();
      EEPROMWritelong(1, randNumber);
      storeSSID(ssid, password);

    }
    else {
      //Serial.println("invalid request");
      client.stop();
      return;
    }
  }
  // Match the request

  if (modo == 1) {
    if (req.indexOf("/L" + String(randNumber)) != -1){
    Serial.println("L");
      val = 0;
    }
    else if (req.indexOf("/H" + String(randNumber)) != -1){
      Serial.println("H");
      val = 1;
    }
    else if (req.indexOf("/rst" + String(randNumber)) != -1) {
      EEPROM.write(0, '0');
      EEPROM.commit();
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
      //calibration = newSense.toInt();
      for (int i = pos + 2; req[i] != '&'; i++) {
        if (millis() - safety > 500)break;
        newDelay += req[i];
        //pos=i;
      }
      Serial.println("&" + newSense+"&"+newDelay+"&");
      //Serial.println("Tempo: " + newDelay);
      //ndelay = newDelay.toInt();
    }
    else if (req.indexOf("/o") != -1);
    else {
      //Serial.println("invalid request");
      client.stop();
      return;
    }

    // Set GPIO2 according to the request
    digitalWrite(2, val);

    client.flush();
    // Prepare the response
    //HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n
    String s = "";
    s += (val) ? "1" : "0";
    //s += "</html>\n";

    // Send the response to the client
    client.print(s);
    delay(1);
    //Serial.println("Client disonnected");

  }
}

void storeSSID(String ss, String pass) {
  //String last = ss+"&"+pass+"&";
  int tam_s = ss.length();
  int tam_p = pass.length();
  for (int i = 0; i < tam_s; i++) {
    EEPROM.write(i + 5, ss.charAt(i));
  }
  for (int i = 0; i < tam_p; i++) {
    EEPROM.write(i + 64, pass.charAt(i));
  }
  EEPROM.commit();
}

String ipToString(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
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
  EEPROM.commit();
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

