
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <HCSR04.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>

//Configuração de conexão do NODEMCU com Servidor MQTT
const char* ssid = "AndroidAP";
const char* password = "snbj8460";
const char* mqttServer = "m15.cloudmqtt.com";
const int mqttPort = 15260;
const char* mqttUser = "hzwxnzwy";
const char* mqttPassword = "8ztJzTw0rMy_";

//Iniciação de variaveis para comunicacao MQTT
WiFiClient espClient;
PubSubClient client(espClient);
void mqtt_callback(char* topic, byte* dados_tcp, unsigned int length); 

//Iniciação de variaveis - MOTOR DE PASSO
const int stepsPerRevolution = 50;
Stepper myStepper(stepsPerRevolution, D5, D4, D3, D6);

//Iniciação de variaveis - SENSOR ULTRASÔNICO
UltraSonicDistanceSensor distanceSensor(13, 15);
LiquidCrystal_I2C lcd(0x3F, 16, 2);

//Variavel de controle
int TimeOut = 0;


void setup() {
  
  myStepper.setSpeed(150); //Velocidade do motor de passo

  // Iniciacao do Display
  lcd.begin(16,2); 
  lcd.init();
  lcd.backlight();
  
  TimeOut = 0;
  Serial.begin(115200);
  setup_wifi(); // Função para conexão com a Rede Wifi
  
  Serial.println("Conectado");

  //Inicia comunicação com Servidor MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  //Mantem a conexão conexão com Servidor MQTT.
  while (!client.connected()) {
    Serial.println("Conectando ao servidor MQTT...");
    if (client.connect("Projeto", mqttUser, mqttPassword ))
    { 
      Serial.println("Conectado ao servidor MQTT!");  
    } else { 
      Serial.print("Falha ao conectar ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  //Publica no Servido MQTT
  client.publish("Status ","Reiniciado!");
  client.publish("Placa","Em funcionamento!");

  //Se inscreve em Tópicos no Servidor MQTT
  client.subscribe("LIXEIRA"); 
  client.subscribe("CODIGO");
}

  //Metódo responsavel por ouvir Servidor MQTT  
  void callback(char* topic, byte* dados_tcp, unsigned int length) {
    
    String msg,topico;
    char c;
    for (int i = 0; i < length; i++) {
       c = (char)dados_tcp[i];
       msg += c;
    }
    topico = (String)topic;
    Serial.println("ANTES DE LER O TOPICO");
    Serial.println(topic);
    Serial.println(msg);
    if(topico.equals("LIXEIRA")){
      Serial.println(topico);
      Serial.println(msg);
      if (msg.equals("L1")) {
            Serial.println("PLASTICO - ABRIR COMPARTIMENTO");  
            toggleTrash('o'); // Método para Abrir compartimento
            TimeOut = 1; // Inicia contagem para TimeOut
          } else if(msg.equals("D1")){
            Serial.println("PLASTICO - FECHAR COMPARTIMENTO");
            toggleTrash('c'); // Método para fechar compartimento
          }
          if (msg.equals("L2")) { 
            Serial.println("METAL - ABRIR COMPARTIMENTO");  
            toggleTrash('o'); // Método para Abrir compartimento
            TimeOut = 1; // Inicia contagem para TimeOut  
          } else if(msg.equals("D2")){ 
            Serial.println("METAL - FECHAR COMPARTIMENTO");
            toggleTrash('c'); // Método para fechar compartimento
          }
    }
    if(topico.equals("CODIGO")){
      Serial.println(topico);
      Serial.println(msg);
      exibeCodigSegurancaDisplay(msg); // Metódo para exibir Código no Display
    }
    msg = "";
  } 

  //Método - Conexão Wifi
  void setup_wifi() {
    delay(10);
    // Inicia conexão com a rede Wifi
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

//
void reconnect() {
  // Loop para reconectar com servidor MQTT
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Tentativa de conectar
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("LIXEIRA"); 
      client.subscribe("CODIGO");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Espera 5 segundos antes de tentar novamente
      delay(5000);
    }
  }
}

//Método responsavel pela leitura do Sensor Ultrasônico
void leituraSensor(){
  delay(100);
  if(TimeOut > 0 & TimeOut < 200){
    Serial.print("TimeOut - ");
    Serial.println(TimeOut);
    TimeOut++;
  }else if(TimeOut == 200){
    client.publish("controle","timeOut");
    Serial.print("TimeOut ");
    Serial.println(TimeOut);
    toggleTrash('c');
    TimeOut = 0;
  }
  
  float ret = distanceSensor.measureDistanceCm();
  if (isnan(ret)) {
    Serial.println("Erro ao ler o sensor!");
    return;
  }else{
     //Serial.println(distanceSensor.measureDistanceCm());
     if(ret < 20){
      client.publish("Sensor","PLASTICO_ATIVADO");
      TimeOut = 0;
      Serial.println("FECHAR LIXEIRA");
      toggleTrash('c');
     }
     if(ret >= 20){
        client.publish("Sensor","DESATIVADO");
     }
  }
}

// Método responsavel por Exibir código no Display
void exibeCodigSegurancaDisplay(String codigo){
    lcd.setCursor(4, 0);
    lcd.print(codigo);
}

//Método responsavel por Abrir e fechar compartimento da Lixeira
void toggleTrash(char op){
  Serial.print("TOGGLE TRASH");
  //o = Open  / c = Close
  if (op == 'o' || op == 'O' || op == 'c' || op == 'C'){
      int dir = (op == 'c' || op == 'C') ? -1 : 1;
      for(int i=0; i<30; i++){
        myStepper.step(stepsPerRevolution*dir);
        delay(10);
      }
    }
}

void loop() {
  leituraSensor();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
