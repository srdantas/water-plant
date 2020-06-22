#include <DHT.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <PubSubClient.h>


#define DHTPIN D3          
DHT dht(DHTPIN, DHT11);


char ssid[] = "*********";          //  Nome da rede Wifi que deseja conectar 
char pass[] = "**********";           // Senha da rede wifi 
char nomeUsuarioMqtt[] = "ProjetoIoT";  // Utilizar qualquer nome 
char senhaMqtt[] = "**********";      // Colocar a senha do MQTT API do ThingSpeak que pode ser localizado no seu perfil do site   
char writeAPIKey[] = "**********";    // Colocar a writeAPIKey do seu canal criado no site ThingSpeak
long channelID = 1085933;                    // Colocar o ID do seu canal no ThingSpeak
int fieldNumber = 1;                        // Colocar a quantidade de campos que vai utilizar no ThingSpeak 


WiFiClient client;

PubSubClient mqttClient(client); 
const char* server = "mqtt.thingspeak.com"; // conexao com o servidor

unsigned long ultimaConexao =0;
const unsigned long intervalooPostagem = 20L * 1000L;
const int sensorUmidade = A0;             
const int bombaAgua = D0;
unsigned long intervalo1 = 5000;
unsigned long previousMillis1 = 0;
float umidadePorcentagem;              
float h;                  
float t;                  

void setup()
{
  Serial.begin(9600);
  delay(10);
  pinMode(bombaAgua, OUTPUT);
  digitalWrite(bombaAgua, LOW); // mantem motor desligado no início 
  dht.begin();
  Serial.println("Conectando Ao: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");              // coloca pontos finais no serial ate conectar a internet 
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  mqttClient.setServer(server, 1883); // conecta ao broker 
}

void reconnect() // funcao para realizar a conexao ao Broker MQTT
{
  char clientID[9];

  
  while (!mqttClient.connected()) 
  {
    Serial.print("Tentando realizar conexao MQTT...");
    
    for (int i = 0; i < 8; i++) {
        clientID[i] = alphanum[random(51)];
    }
    clientID[8]='\0';

    
    if (mqttClient.connect(clientID,nomeUsuarioMqtt,senhaMqtt)) 
    {
      Serial.println("conectado");
    } else 
    {
      Serial.print("Falha na conexao, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}
// Funcao para publicar dados usando MQTT
void mqttPublishField(int fieldNum) {
  
  umidadePorcentagem = ( 100.00 - ( (analogRead(sensorUmidade) / 1023.00) * 100.00 ) ); 
 
  // Criando os dados para enviar ao ThingSpeak 
  String data = String(umidadePorcentagem, DEC);
  int length = data.length();
  const char *msgBuffer;
  msgBuffer = data.c_str();
  Serial.println(msgBuffer);
  
  // Criação de um topico e publicacao no feed do ThingSpeak  
  String topicString ="channels/" + String( channelID ) + "/publish/fields/field" + String(fieldNum) + "/" + String(writeAPIKey);
  length = topicString.length();
  const char *topicBuffer;
  topicBuffer = topicString.c_str();
  mqttClient.publish( topicBuffer, msgBuffer );

  ultimaConexao = millis();
}


void loop()
{
  unsigned long currentMillis = millis(); // Obtem o tempo atual

  h = dht.readHumidity();     // Leitura de umidade do DHT
  t = dht.readTemperature();     // Leitura da Temperatura do DHT

  if (isnan(h) || isnan(t))
  {
    Serial.println("Falha ao ler o sensor DHT");
    return;
  }

  umidadePorcentagem = ( 100.00 - ( (analogRead(sensorUmidade) / 1023.00) * 100.00 ) ); // formula para transformar a leitura do sensor em forma de porcentagem
  // mostra no monitor serial a variavel lida pelo sensor de umidade
  if ((unsigned long)(currentMillis - previousMillis1) >= intervalo1) {
    Serial.print("Umidade do solo  = ");
    Serial.print(umidadePorcentagem);
    Serial.println("%");
    Serial.print("Temperatura =");
    Serial.print(t);
    Serial.println(" graus Celsius");
    previousMillis1 = millis();
  }

// condição para ligar a bomba de água 
if (umidadePorcentagem < 55) {
  digitalWrite(bombaAgua, HIGH);         // Liga o bomba de água
}
if (umidadePorcentagem > 55 && umidadePorcentagem < 65) {
  digitalWrite(bombaAgua, HIGH);        // Mantem a bomba de água ligada
}
if (umidadePorcentagem > 66) {
  digitalWrite(bombaAgua, LOW);          // Desliga a bomba de água 
}

// Reconecta ao Broker MQTT caso ele não esteja conectado 
if (!mqttClient.connected()) 
{
  reconnect();
}

mqttClient.loop();   // Chama a funcao de loop para manter a conexao com o servidor 
// Se o intervalo passou desde a ultima conexao feita, publica o dado no ThingSpeak
if (millis() - ultimaConexao > intervalooPostagem) 
{
  Serial.print("Dado enviado:");
  mqttPublishField(fieldNumber); // Publica um valor ao campo escolhido
    
}
}
