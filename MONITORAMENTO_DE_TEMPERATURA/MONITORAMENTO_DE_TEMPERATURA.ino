#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define INTERVALO_ENVIO 1000

#define DEBUG

#define DS18B20 5

//Informações da Rede WiFi
const char* ssid = "Robson";
const char* password = "91637890";

//configurador do broker MQTT
const char* mqttServer = "tailor.cloudmqtt.com";
const char* mqttUser = "yzsnwxyy";
const char* mqttPassword = "JAcJ4HL7Xngy";
const uint16_t mqttPort = 10593;
const char* mqttTopicSub = "BeerCoolerMonitoringJM";

int UltimoEnvioMQTT = 0;

float temperatura;

WiFiClient espClient; 
PubSubClient client(espClient);

OneWire ourWire(DS18B20);
DallasTemperature sensors(&ourWire);

void setup() {
  
  Serial.begin(9600);

  
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    #ifdef DEBUG
    Serial.println("conectando ao WiFi...");
    #endif 
  }
  #ifdef DEBUG
  Serial.println("conectado na Rede WiFi");
  #endif 


  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while(!client.connected()){
    #ifdef DEBUG
    Serial.println("Conectando ao Boker MQTT...");
    #endif

    if (client.connect("ESP8266Client", mqttUser, mqttPassword)){
      #ifdef DEBUG
      Serial.println("Conectado");
      #endif
    }else{
      #ifdef DEBUG 
      Serial.print("falha estado ");
      Serial.print(client.state());
      #endif
      delay(2000);
    }
  }
  client.subscribe(mqttTopicSub);


  sensors.begin();
  delay(1000);

}
#ifdef DEBUG
void callback(char* topic, byte* payload, unsigned int length){

  //armazena msg recebida em uma string 
  payload[length] ='\0';
  String strMSG = String((char*)payload);

  #ifdef DEBUG 
  Serial.print("Mensagem chegou do topico: ");
  Serial.println(topic);
  Serial.print("Mensagem: ");
  Serial.print(strMSG);
  Serial.println();
  Serial.println("--------------------------");
  #endif
}
#endif


//função pra reconectar ao servido MQTT
void reconect() {
  //Enquanto estiver desconectado
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.print("Tentando conectar ao servidor MQTT");
    #endif
     
    bool conectado = strlen(mqttUser) > 0 ?
                     client.connect("ESP8266Client", mqttUser, mqttPassword) :
                     client.connect("ESP8266Client");
 
    if(conectado) {
      #ifdef DEBUG
      Serial.println("Conectado!");
      #endif
      //subscreve no tópico
      client.subscribe(mqttTopicSub, 1); //nivel de qualidade: QoS 1
    } else {
      #ifdef DEBUG
      Serial.println("Falha durante a conexão.Code: ");
      Serial.println( String(client.state()).c_str());
      Serial.println("Tentando novamente em 10 s");
      #endif
      //Aguarda 10 segundos 
      delay(10000);
    }
  }
}

 
void enviaTemperatura(){

  char MsgTemperaturaMQTT[10];
  char MsgAviso[10];
  
  #ifdef DEBUG
  sensors.requestTemperatures();
  temperatura = sensors.getTempCByIndex(0);
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println("C");
  delay(250);
  Serial.println("mensagem enviada para o Boker");
  #endif

  sprintf(MsgTemperaturaMQTT, "%f", temperatura);
  client.publish("BeerCoolerMonitoringJM", MsgTemperaturaMQTT);
  //client.publish("BeerCoolerMonitoringJM", String(temperatura).c_str(),TRUE);  

  if(temperatura < 0){
    sprintf(MsgAviso, "VAI CONGELAR!");
    client.publish("CadeMinhaGelada", MsgAviso);
  }
  if(temperatura >= 0 or temperatura <= 5){
    sprintf(MsgAviso, "MUITO GELADA!");
    client.publish("CadeMinhaGelada", MsgAviso);
  }
  if(temperatura > 5 or temperatura <= 7){
    sprintf(MsgAviso, "BEM GELADA!");
    client.publish("CadeMinhaGelada", MsgAviso);
  }
  if(temperatura >= 8 or temperatura <= 12){
    sprintf(MsgAviso, "GELADA!");
    client.publish("CadeMinhaGelada", MsgAviso);
  }
  if(temperatura > 12){
    sprintf(MsgAviso, "ta quente ainda!");
    client.publish("CadeMinhaGelada", MsgAviso);
  }
  
}


void loop() {

  if(!client.connected()){
    reconect();
  }

  if((millis() - UltimoEnvioMQTT) > INTERVALO_ENVIO){
     enviaTemperatura();
     UltimoEnvioMQTT = millis();
  }
 
  client.loop();
}
