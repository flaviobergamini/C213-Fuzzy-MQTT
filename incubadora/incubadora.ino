#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Fuzzy.h>    // instalar a biblioteca eFLL
#include <math.h>

Fuzzy *fuzzy = new Fuzzy();
float PV=30; // inicializando o nível com zero
float Erro;
float DErro; 
float PVanterior;
float Saida=0;
int setpoint=30;
int i=0;

int sp = 0;

// SSID e Password para o modo AP
const char *ssid_ap = "Incubadora";
const char *password_ap = "12345678";

// SSID e Password para o modo Station
const char *ssid_new;
const char *pass_new;

String ssid_s, password_s;

// Endereço do broker MQTT
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
ESP8266WebServer server(80);
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void WriteEEPROM(String ssid, String password);
void ReadEEPROM();
void eraseEEPROM();
void reconnect();

void setup_wifi(String ssid, String password, bool eeprom) {
  
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  digitalWrite(D0, HIGH);
  digitalWrite(D1, LOW);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  if(WiFi.status() == WL_CONNECTED){
    digitalWrite(D0, LOW);
    digitalWrite(D1, HIGH);
    digitalWrite(D3,LOW);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Gravando dados na eeprom
    if(eeprom)
      WriteEEPROM(ssid, password);
      
    delay(10);
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    // Iniciando broker MQTT
    while(true){
      if (!client.connected()) {
        reconnect();
      }
      client.loop();
      
      Erro=PV-setpoint;
      fuzzy->setInput(1, Erro);
      fuzzy->setInput(2, DErro);
      fuzzy->fuzzify();
      Saida = fuzzy->defuzzify(1);
      Serial.println("Temperatura atual: "+String(PV)+"*C");
      Serial.println("Erro: "+String(Erro));
      Serial.println("SetPoint: "+String(setpoint));
      Serial.println("------------------------------");
      //Serial.println (String(PV)+";"+String(Erro)+";"+String(Saida)+";"+String(setpoint)+";"+String(DErro));
      client.publish("incubadora/PV", String(PV).c_str());
      client.publish("incubadora/Erro", String(Erro).c_str());
      client.publish("incubadora/Temp", String(Saida).c_str());
      PVanterior=PV;
      PV=0.9954*PV+0.002763*Saida;
      DErro=PV-PVanterior;
      i=i+1;
      delay (250);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String number = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    number += (char)payload[i];
    //Serial.println(number);
  }
  setpoint = number.toInt();
  
  String topicStr(topic);
  int bottleCount = 0;                // assume no bottles unless we correctly parse a value from the topic
  if (topicStr.indexOf('/') >= 0) {
    // The topic includes a '/', we'll try to read the number of bottles from just after that
    topicStr.remove(0, topicStr.indexOf('/')+1);
    
    // Now see if there's a number of bottles after the '/'
    bottleCount = topicStr.toInt();
  }

  if (bottleCount > 0) {
    // Work out how big our resulting message will be
    int msgLen = 0;
    for (int i = bottleCount; i > 0; i--) {
      String numBottles(i);
      msgLen += 2*numBottles.length();
      if (i == 1) {
        msgLen += 2*String(" green bottle, standing on the wall\n").length();
      } else {
        msgLen += 2*String(" green bottles, standing on the wall\n").length();
      }
      msgLen += String("And if one green bottle should accidentally fall\nThere'll be ").length();
      switch (i) {
      case 1:
        msgLen += String("no green bottles, standing on the wall\n\n").length();
        break;
      case 2:
        msgLen += String("1 green bottle, standing on the wall\n\n").length();
        break;
      default:
        numBottles = i-1;
        msgLen += numBottles.length();
        msgLen += String(" green bottles, standing on the wall\n\n").length();
        break;
      };
    }
  
    // Now we can start to publish the message
    client.beginPublish("greenBottles/lyrics", msgLen, false);
    for (int i = bottleCount; i > 0; i--) {
      for (int j = 0; j < 2; j++) {
        client.print(i);
        if (i == 1) {
          client.print(" green bottle, standing on the wall\n");
        } else {
          client.print(" green bottles, standing on the wall\n");
        }
      }
      client.print("And if one green bottle should accidentally fall\nThere'll be ");
      switch (i) {
      case 1:
        client.print("no green bottles, standing on the wall\n\n");
        break;
      case 2:
        client.print("1 green bottle, standing on the wall\n\n");
        break;
      default:
        client.print(i-1);
        client.print(" green bottles, standing on the wall\n\n");
        break;
      };
    }
    // Now we're done!
    client.endPublish();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
     
      client.publish("outTopic", "hello world");
      
      client.subscribe("greenBottles/#");
      client.subscribe("incubadora/sp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
     
      delay(5000);
    }
  }
}


// ---------------------------- Trabalhando com a lógica Fuzzy ----------------------------  
 
// CONJUNTOS DA VARIÁVEL ERRO
FuzzySet *MN = new FuzzySet(-28, -28, -10, -5);
FuzzySet *PN = new FuzzySet(-10, -5, -5, 0);
FuzzySet *ZE = new FuzzySet(-5, 0,0, 5);
FuzzySet *PP = new FuzzySet(0, 5,5, 10);
FuzzySet *MP = new FuzzySet(5, 10, 13, 13);

// CONJUNTOS DA VARIÁVEL DELTA ERRO
FuzzySet *MNd = new FuzzySet(-28, -28, -10, -5);
FuzzySet *PNd = new FuzzySet(-10, -5,-5, 0);
FuzzySet *ZEd = new FuzzySet(-5, 0,0, 5);
FuzzySet *PPd = new FuzzySet(0, 5, 5,10);
FuzzySet *MPd = new FuzzySet(5, 10, 13, 13);

// CONJUNTOS DA VARIÁVEL RESISTENCIA
FuzzySet *MB = new FuzzySet(0,0,0,25);
FuzzySet *B = new FuzzySet(0, 25, 25, 50);
FuzzySet *N = new FuzzySet(25, 50, 50, 75);
FuzzySet *A = new FuzzySet(50, 75, 75, 100);
FuzzySet *MA = new FuzzySet(75, 100, 100, 100);


void setup()
{
    // ------------FUZZY------------ 
  //  VARIAVEL erro
        FuzzyInput *Erro = new FuzzyInput(1);
        Erro->addFuzzySet(MN);
        Erro->addFuzzySet(PN);
        Erro->addFuzzySet(ZE);
        Erro->addFuzzySet(PP);
        Erro->addFuzzySet(MP);  
        fuzzy->addFuzzyInput(Erro);

  //  VARIAVEL delta erro
        FuzzyInput *DErro = new FuzzyInput(2);
        DErro->addFuzzySet(MNd);
        DErro->addFuzzySet(PNd);
        DErro->addFuzzySet(ZEd);
        DErro->addFuzzySet(PPd);
        DErro->addFuzzySet(MPd);  
        fuzzy->addFuzzyInput(DErro);
  
  //  VARIAVEL saida
        FuzzyOutput *Saida = new FuzzyOutput(1);
        Saida->addFuzzySet(MB);
        Saida->addFuzzySet(B);
        Saida->addFuzzySet(N);
        Saida->addFuzzySet(A);
        Saida->addFuzzySet(MA);  
        fuzzy->addFuzzyOutput(Saida);

  FuzzyRuleConsequent* thenSaidaMB = new FuzzyRuleConsequent();
  FuzzyRuleConsequent* thenSaidaB = new FuzzyRuleConsequent();
  FuzzyRuleConsequent* thenSaidaN = new FuzzyRuleConsequent();
  FuzzyRuleConsequent* thenSaidaA = new FuzzyRuleConsequent();
  FuzzyRuleConsequent* thenSaidaMA = new FuzzyRuleConsequent();

 
  // -------------------------------MA-------------------------------------
  // Building FuzzyRule "IF Erro=MN and DEerro = MN THEN saida = MA"
  FuzzyRuleAntecedent *ifErroMNAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroMNd->joinWithAND(MN, MNd);
  thenSaidaMA->addOutput(MA);
  FuzzyRule *fuzzyRule01 = new FuzzyRule(1, ifErroMNAndDErroMNd, thenSaidaMA);
  fuzzy->addFuzzyRule(fuzzyRule01);

  // Building FuzzyRule "IF Erro=PN and DEerro = MN THEN saida = MA"
  FuzzyRuleAntecedent *ifErroPNAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroMNd->joinWithAND(PN, MNd);
  thenSaidaMA->addOutput(MA);
  FuzzyRule *fuzzyRule02 = new FuzzyRule(2, ifErroPNAndDErroMNd, thenSaidaMA);
  fuzzy->addFuzzyRule(fuzzyRule02);

  // Building FuzzyRule "IF Erro=MN and DEerro = PN THEN saida = MA"
  FuzzyRuleAntecedent *ifErroMNAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroPNd->joinWithAND(MN, PNd);
  thenSaidaMA->addOutput(MA);
  FuzzyRule *fuzzyRule03 = new FuzzyRule(3, ifErroMNAndDErroPNd, thenSaidaMA);
  fuzzy->addFuzzyRule(fuzzyRule03);

  // Building FuzzyRule "IF Erro=MN and DEerro = ZE THEN saida = MA"
  FuzzyRuleAntecedent *ifErroMNAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroZEd->joinWithAND(MN, ZEd);
  thenSaidaMA->addOutput(MA);
  FuzzyRule *fuzzyRule04 = new FuzzyRule(4, ifErroMNAndDErroZEd, thenSaidaMA);
  fuzzy->addFuzzyRule(fuzzyRule04);

  // ----------------------------------------------------------------------

  // --------------------------------A-------------------------------------
  // Building FuzzyRule "IF Erro=ZE and DEerro = MN THEN saida = A"
  FuzzyRuleAntecedent *ifErroZEAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroMNd->joinWithAND(ZE, MNd);
  thenSaidaA->addOutput(A);
  FuzzyRule *fuzzyRule05 = new FuzzyRule(5, ifErroZEAndDErroMNd, thenSaidaA);
  fuzzy->addFuzzyRule(fuzzyRule05);

  // Building FuzzyRule "IF Erro=PN and DEerro = PN THEN saida = A"
  FuzzyRuleAntecedent *ifErroPNAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroPNd->joinWithAND(PN, PNd);
  thenSaidaA->addOutput(A);
  FuzzyRule *fuzzyRule06 = new FuzzyRule(6, ifErroPNAndDErroPNd, thenSaidaA);
  fuzzy->addFuzzyRule(fuzzyRule06);

  // Building FuzzyRule "IF Erro=ZE and DEerro = PN THEN saida = A"
  FuzzyRuleAntecedent *ifErroZEAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroPNd->joinWithAND(ZE, PNd);
  thenSaidaA->addOutput(A);
  FuzzyRule *fuzzyRule07 = new FuzzyRule(7, ifErroZEAndDErroPNd, thenSaidaA);
  fuzzy->addFuzzyRule(fuzzyRule07);

  // Building FuzzyRule "IF Erro=PN and DEerro = ZE THEN saida = A"
  FuzzyRuleAntecedent *ifErroPNAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroZEd->joinWithAND(PN, ZEd);
  thenSaidaA->addOutput(A);
  FuzzyRule *fuzzyRule08 = new FuzzyRule(8, ifErroPNAndDErroZEd, thenSaidaA);
  fuzzy->addFuzzyRule(fuzzyRule08);

  // Building FuzzyRule "IF Erro=MN and DEerro = PP THEN saida = A"
  FuzzyRuleAntecedent *ifErroMNAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroPPd->joinWithAND(MN, PP);
  thenSaidaA->addOutput(A);
  FuzzyRule *fuzzyRule09 = new FuzzyRule(9, ifErroMNAndDErroPPd, thenSaidaA);
  fuzzy->addFuzzyRule(fuzzyRule09);

  // Building FuzzyRule "IF Erro=PN and DEerro = PP THEN saida = A"
  FuzzyRuleAntecedent *ifErroPNAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroPPd->joinWithAND(PN, PPd);
  thenSaidaA->addOutput(A);
  FuzzyRule *fuzzyRule10 = new FuzzyRule(10, ifErroPNAndDErroPPd, thenSaidaA);
  fuzzy->addFuzzyRule(fuzzyRule10);
  // ----------------------------------------------------------------------

  // --------------------------------N-------------------------------------
  // Building FuzzyRule "IF Erro=PP and DEerro = MN THEN saida = N"
  FuzzyRuleAntecedent *ifErroPPAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroMNd->joinWithAND(ZE, MNd);
  thenSaidaN->addOutput(N);
  FuzzyRule *fuzzyRule11 = new FuzzyRule(11, ifErroPPAndDErroMNd, thenSaidaN);
  fuzzy->addFuzzyRule(fuzzyRule11);

  // Building FuzzyRule "IF Erro=MP and DEerro = MN THEN saida = N"
  FuzzyRuleAntecedent *ifErroMPAndDErroMNd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroMNd->joinWithAND(MP, MNd);
  thenSaidaN->addOutput(N);
  FuzzyRule *fuzzyRule12 = new FuzzyRule(12, ifErroMPAndDErroMNd, thenSaidaN);
  fuzzy->addFuzzyRule(fuzzyRule12);

  // Building FuzzyRule "IF Erro=ZE and DEerro = ZE THEN saida = N"
  FuzzyRuleAntecedent *ifErroZEAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroZEd->joinWithAND(ZE, ZEd);
  thenSaidaN->addOutput(N);
  FuzzyRule *fuzzyRule13 = new FuzzyRule(13, ifErroZEAndDErroZEd, thenSaidaN);
  fuzzy->addFuzzyRule(fuzzyRule13);

  // Building FuzzyRule "IF Erro=ZE and DEerro = PP THEN saida = N"
  FuzzyRuleAntecedent *ifErroZEAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroPPd->joinWithAND(ZE, PP);
  thenSaidaN->addOutput(N);
  FuzzyRule *fuzzyRule14 = new FuzzyRule(14, ifErroZEAndDErroPPd, thenSaidaN);
  fuzzy->addFuzzyRule(fuzzyRule14);

  // Building FuzzyRule "IF Erro=MN and DEerro = MP THEN saida = N"
  FuzzyRuleAntecedent *ifErroMNAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroMNAndDErroMPd->joinWithAND(MN, MPd);
  thenSaidaN->addOutput(N);
  FuzzyRule *fuzzyRule15 = new FuzzyRule(15, ifErroMNAndDErroMPd, thenSaidaN);
  fuzzy->addFuzzyRule(fuzzyRule15);

  // Building FuzzyRule "IF Erro=PN and DEerro = MP THEN saida = N"
  FuzzyRuleAntecedent *ifErroPNAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroPNAndDErroMPd->joinWithAND(PN, MPd);
  thenSaidaN->addOutput(N);
  FuzzyRule *fuzzyRule16 = new FuzzyRule(16, ifErroPNAndDErroMPd, thenSaidaN);
  fuzzy->addFuzzyRule(fuzzyRule16);
  // ----------------------------------------------------------------------

  // --------------------------------B-------------------------------------
  // Building FuzzyRule "IF Erro=PP and DEerro = PN THEN saida = B"
  FuzzyRuleAntecedent *ifErroPPAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroPNd->joinWithAND(PP, PNd);
  thenSaidaB->addOutput(B);
  FuzzyRule *fuzzyRule17 = new FuzzyRule(17, ifErroPPAndDErroPNd, thenSaidaB);
  fuzzy->addFuzzyRule(fuzzyRule17);

  // Building FuzzyRule "IF Erro=MP and DEerro = PN THEN saida = B"
  FuzzyRuleAntecedent *ifErroMPAndDErroPNd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroPNd->joinWithAND(MP, PNd);
  thenSaidaB->addOutput(B);
  FuzzyRule *fuzzyRule18 = new FuzzyRule(18, ifErroMPAndDErroPNd, thenSaidaB);
  fuzzy->addFuzzyRule(fuzzyRule18);

  // Building FuzzyRule "IF Erro=PP and DEerro = ZE THEN saida = B"
  FuzzyRuleAntecedent *ifErroPPAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroZEd->joinWithAND(PP, ZEd);
  thenSaidaB->addOutput(B);
  FuzzyRule *fuzzyRule19 = new FuzzyRule(19, ifErroPPAndDErroZEd, thenSaidaB);
  fuzzy->addFuzzyRule(fuzzyRule19);

  // Building FuzzyRule "IF Erro=MP and DEerro = ZE THEN saida = B"
  FuzzyRuleAntecedent *ifErroMPAndDErroZEd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroZEd->joinWithAND(MP, ZEd);
  thenSaidaB->addOutput(B);
  FuzzyRule *fuzzyRule20 = new FuzzyRule(20, ifErroMPAndDErroZEd, thenSaidaB);
  fuzzy->addFuzzyRule(fuzzyRule20);

  // Building FuzzyRule "IF Erro=PP and DEerro = PP THEN saida = B"
  FuzzyRuleAntecedent *ifErroPPAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroPPd->joinWithAND(PP, PPd);
  thenSaidaB->addOutput(B);
  FuzzyRule *fuzzyRule21 = new FuzzyRule(21, ifErroPPAndDErroPPd, thenSaidaB);
  fuzzy->addFuzzyRule(fuzzyRule21);

  // Building FuzzyRule "IF Erro=ZE and DEerro = MP THEN saida = B"
  FuzzyRuleAntecedent *ifErroZEAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroZEAndDErroMPd->joinWithAND(ZE, MPd);
  thenSaidaB->addOutput(B);
  FuzzyRule *fuzzyRule22 = new FuzzyRule(22, ifErroZEAndDErroMPd, thenSaidaB);
  fuzzy->addFuzzyRule(fuzzyRule22);

  // ----------------------------------------------------------------------

  // -------------------------------MB-------------------------------------
  // Building FuzzyRule "IF Erro=MP and DEerro = PP THEN saida = MB"
  FuzzyRuleAntecedent *ifErroMPAndDErroPPd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroPPd->joinWithAND(MP, PPd);
  thenSaidaMB->addOutput(MB);
  FuzzyRule *fuzzyRule23 = new FuzzyRule(23, ifErroMPAndDErroPPd, thenSaidaMB);
  fuzzy->addFuzzyRule(fuzzyRule23);

  // Building FuzzyRule "IF Erro=PP and DEerro = MP THEN saida = MB"
  FuzzyRuleAntecedent *ifErroPPAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroPPAndDErroMPd->joinWithAND(PP, MPd);
  thenSaidaMB->addOutput(MB);
  FuzzyRule *fuzzyRule24 = new FuzzyRule(24, ifErroPPAndDErroMPd, thenSaidaMB);
  fuzzy->addFuzzyRule(fuzzyRule24);

  // Building FuzzyRule "IF Erro=MP and DEerro = MP THEN saida = MB"
  FuzzyRuleAntecedent *ifErroMPAndDErroMPd = new FuzzyRuleAntecedent();
  ifErroMPAndDErroMPd->joinWithAND(MP, MPd);
  thenSaidaMB->addOutput(MB);
  FuzzyRule *fuzzyRule25 = new FuzzyRule(25, ifErroMPAndDErroMPd, thenSaidaMB);
  fuzzy->addFuzzyRule(fuzzyRule25);
  // ----------------------------------------------------------------------

  // ------------- Demais configurações ---------------
  pinMode(D0,OUTPUT);               // WI-FI Station Desconectado
  pinMode(D1,OUTPUT);               // WI-FI Station Conectado
  pinMode(D2,OUTPUT);               // Tomada
  pinMode(D3,OUTPUT);               // WI-FI Access Point
  pinMode(D7, INPUT);               // Botão de reset da memória EEPROM

  digitalWrite(D0,LOW);
  digitalWrite(D1,LOW);
  digitalWrite(D2,LOW);
  digitalWrite(D3,LOW);

  Serial.begin(115200);
  EEPROM.begin(1024);
  
  byte value = EEPROM.read(0);
  EEPROM.end();
  Serial.println("");
  if(digitalRead(D7) == LOW){
    eraseEEPROM();
    Serial.println("Limpando memória EEPROM");
  }
  if(value == 0)
  {
    Serial.println("EEPROM Vazia, entrando no modo AP");
    delay(500);
    modeAP();
   }
   else
   {
   ReadEEPROM(); 
   Serial.println("*********** Tentando Conexao ***********");
   Serial.println("------------------------");
   Serial.print("SSID: ");
   Serial.println(ssid_s);
   Serial.print("Password: ");
   Serial.println(password_s);
   Serial.println("------------------------");
   setup_wifi(ssid_s, password_s, false);
   }
}


void loop()
{    
  server.handleClient();
}

// ----------------- Criando rede Access Point (AP) para configuração do Wi-Fi do modo station -----------------

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>

<body bgcolor="#8080ff">
  
  <h2>Incubadora<h2>
  <h3> C2135 - 2022</h3>

  <form action="/action_page">
    SSID:<br>
    <input type="text" name="SSID" value="">
    <br>
    Password:<br>
    <input type="password" name="Password" value="">
    <br><br>
    <input type="submit" value="Submit">
  </form> 

</body>

</html>
)=====";

void handleRoot() {
 String s = MAIN_page;                    //lendo HTML
 server.send(200, "text/html", s);        //enviando HTML para o client
}

void handleForm() {
   digitalWrite(D0, HIGH);
   digitalWrite(D1, LOW);
   String ssid_Station = server.arg("SSID"); 
   String pass_Station = server.arg("Password"); 
   
   ssid_new = ssid_Station.c_str();
   pass_new = pass_Station.c_str();
   
   Serial.print("SSID: ");
   Serial.println(ssid_Station);
   
   Serial.print("Password: ");
   Serial.println(pass_Station);
   Serial.print("------------- \n");
   Serial.println(ssid_new);
   
   String s = "<a href='/'> Go Back </a>";
   server.send(200, "text/html", s); // Enviando página Web
   
   for (uint8_t t = 4; t > 0; t--) {
      Serial.printf("[SETUP] WAIT %d...\n", t);
      Serial.flush();
      delay(800);
    }
    WiFi.mode(WIFI_STA);
    setup_wifi(ssid_new, pass_new, true);
}

void modeAP()
{
  digitalWrite(D0, HIGH);
  digitalWrite(D1, LOW);
  digitalWrite(D3,HIGH); 
  delay(1000);
  Serial.println();
  Serial.print("Configurando access point...");
  
  WiFi.softAP(ssid_ap, password_ap);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/action_page", handleForm); //form action is handled here
  server.begin();
  Serial.println("Servidor HTTP Iniciado");
}

// ---------------------------- Trabalhando com a memória EEPROM ----------------------------
void eraseEEPROM() {
  EEPROM.begin(1024);
  for (int i = 0; i < 1024; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.end();
}

void WriteEEPROM(String ssid, String password)
{
  EEPROM.begin(1024);
  int ssidlen = ssid.length();
  int passlen = password.length();

  Serial.println("Escrevendo SSID na EEPROM:");
  for (int i = 0; i < ssidlen; ++i)
  {
    EEPROM.write(i, ssid[i]);
    Serial.print("Escrito: ");
    Serial.println(ssid[i]); 
  }
  EEPROM.write(ssidlen, '|');
  Serial.println("Escrevendo Password na EEPROM:");
  for (int i = 0; i < passlen; ++i)
  {
    EEPROM.write((i+ssidlen+1), password[i]);
    Serial.print("Escrito: ");
    Serial.println(password[i]); 
  }
  EEPROM.end();
}

void ReadEEPROM()
{
  EEPROM.begin(1024);
  Serial.println("Lendo EEPROM ssid");
  
  bool ssidBool = false;
  String esid = "";
  String passq = "";
  for(int i = 0; i < EEPROM.length(); i++){
    if(char(EEPROM.read(i)) == '|')
      ssidBool = true;
    if(char(EEPROM.read(i)) != '|' and ssidBool == false){
        esid += char(EEPROM.read(i));
    }
    else if(char(EEPROM.read(i)) != '|' and ssidBool == true){
        passq +=char(EEPROM.read(i));
   }
  }
  ssid_s = esid.c_str();
  password_s = passq.c_str();
  EEPROM.end();
}
