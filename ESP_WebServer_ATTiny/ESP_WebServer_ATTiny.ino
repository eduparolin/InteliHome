#include <EEPROM.h>
#include <ESP8266WiFi.h>

//-----Funções:

//ip: ip da conexão local(roteador).
//resp: Resposta ao pedido.
String ip = "";
String resp = "";
String teste = "";

//val: 1=desligado, 0=ligado. Essa configuração é devido a
//forma que a placa foi montada(válido para attiny).
int val = 1;

//Lê a entrada serial.
String readString;

//ssid e senha do AP.
const char* ssid = "InteliHome";
const char* password = "12345678";

//Nûmero de identificação de cada módulo.
//É gerado durante o modo 1.
int randNumber = 0;

//Impede envio de pacotes de conexão.
boolean conn = false;

//ssid e senha da rede local(roteador).
String nssid = "";
String npassword = "";

//Modo de operação: 0=Configuração, 1=Ready_to_play !.
int modo = 0;

//Informações do servidor: apenas para log.
const char* host = "192.168.25.18";
const char* host_ext = "intelihome.redirectme.net";

//Usado para calcular intervalo de update de conexão.
unsigned long prev = 0;
long intervalo = 120000;

//checkCon(): Verifica conexão em cada intervalo de tempo.
void checkCon() {
  unsigned long cur = millis();
  char stats = '0';
  if (cur - prev > intervalo) {
    prev = cur;
    sendCom();
    if (WiFi.status() != WL_CONNECTED)ESP.restart();
    else Serial.println(".");
  }
}

//sendCom(): Envia informações para o servidor.
void sendCom() {
  WiFiClient sender;
  const int httpPort = 80;
  if (ext_url) {
    if (!sender.connect(host_ext, httpPort)) {
      //Serial.println("connection failed");
      return;
    }
  } else {
    if (!sender.connect(host, httpPort)) {
      //Serial.println("connection failed");
      return;
    }
  }
  String url = String(randNumber);
  url += " - ";
  url += (val) ? "0" : "1";
  sender.print(url);
  sender.stop();

}

//EEPROMWritelong(): Escreve long ou int na memória EEMPROM.
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

//EEPROMReadlong(): Lê long ou int da memória EEPROM.
long EEPROMReadlong(long address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

//putEEPROMWiFi(): Coloca as informações de WIFI na EEPROM.
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

//readEEPROMWiFi(): Lê as informações de WIFI da EEPROM.
void readEEPROMWiFi() {
  for (int i = 0; EEPROM.read(i + 10) != '&'; i++) {
    nssid += (char)EEPROM.read(i + 10);
  }
  for (int i = 0; EEPROM.read(i + 100) != '&'; i++) {
    npassword += (char)EEPROM.read(i + 100);
  }
}

//ipToString(): Converte im endereço de IP para String.
String ipToString(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    delay(1);
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}

//Aqui acaba as funções e começa o código em si.

//------The_Brain:

//Declarando servidor: porta 5566 padrão.
WiFiServer server(5566);

void setup() {
  //Desconecta de todas as redes(AP e STA).
  WiFi.disconnect();
  //Inicia EEPROM e Serial.
  Serial.begin(9600);
  EEPROM.begin(512);
  //EEPROM.write(0,'0');
  //EEPROM.commit();
  delay(10);

  if (EEPROM.read(0) == (char)'0')modo = 0;
  if (EEPROM.read(0) == (char)'1')modo = 1;

  // prepara GPIO2(controle da luz).
  pinMode(2, OUTPUT);
  val = 1;
  digitalWrite(2, val);

  //Aqui o modo define o que o módulo está fazendo:
  if (modo == 0) {
    //Modo de configuração.
    WiFi.mode(WIFI_AP_STA);
    //Prints só para verificação no console(não são necessaários).
    Serial.println();
    Serial.println();
    Serial.print("AP: ");
    Serial.println(ssid);
    //Inicia o AP(SSID: InteliHome, SENHA: 12345678).
    WiFi.softAP(ssid, password);
    Serial.println("");
    Serial.println("AP Criado !");
  } else if (modo == 1) {
    //Modo normal(operação).
    //Lê as informações de WIFI da EEPROM. Tambem o identificador (randNumber).
    randNumber = EEPROMReadlong(1);
    readEEPROMWiFi();
    delay(10);
    //Inicia STA(conezão com roteador).
    WiFi.begin(nssid.c_str(), npassword.c_str());
    int times = millis();
    while (WiFi.status() != WL_CONNECTED) {
      //Espera até que a conexão seja estabelecida.
      delay(1);
      if (millis() - times > 500) {
        times = millis();
        //ESP.restart();
        Serial.print(".");
      }
    }
    //Serial.println("\nConectado ao roteador !");
    // Print the IP address
    //Serial.println(WiFi.localIP());
  }
  //Inicia o servidor.
  server.begin();
  //Serial.println("Server started");


}

void loop() {
  //Verifica se existe conexão com algum cliente.
  WiFiClient client = server.available();
  if (!client) {
    //Isso acontece enquanto não existe nenhuma conexão..
    //Aqui nós checamos a conexão com o roteador e ficamos ouvido por
    //atualizações no Serial.
    checkCon();
    while (Serial.available()) {
      delay(3);
      char c = Serial.read();
      readString += c;
    }
    readString.trim();
    if (readString.length() > 0) {
      if (readString == "H") {
        //Ativa luz pelo serial.
        val = 0;
        digitalWrite(2, val);
      }
      if (readString == "L")
      {
        //Desativa luz pelo serial.
        val = 1;
        digitalWrite(2, val);
      }
      if (readString == "Z")
      {
        //Apaga dados e reseta;
        EEPROM.write(0, '0');
        EEPROM.commit();
        ESP.restart();
      }
      if (readString == "X")
      {
        ext_url = true;
      }
      if (readString == "Y")
      {
        ext_url = false;
      }

      readString = "";
    }
    return;
  }

  //A partir daqui já existe uma conexão, mas nenhum dado foi recebido ainda.
  
  //Serial.println("..");
  while (!client.available()) {
    delay(1);
  }

  //Dados recebidos..
  //Vamos ler eles e interpretar.

  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  //Outra separação e configuração e operação.

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
      //Serial.println("invalid request");
      client.stop();
      return;
    }
  } else {
    if (req.indexOf("/H" + String(randNumber)) != -1) {
      resp = "";
      resp += "1";
      val = 0;
      digitalWrite(2, val);
      Serial.println("H");
    }
    else if (req.indexOf("/L" + String(randNumber)) != -1) {
      resp = "";
      resp += "0";
      val = 1;
      digitalWrite(2, val);
      Serial.println("L");
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
    //Serial.println(".");
  }
  // Send the response to the client


  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed
}

