

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>


const char* ssid = "wifi_ssid";
const char* password = "sup3rp4ss";
const char* mqtt_server = "127.0.0.1";

WiFiClient espClient;
PubSubClient client(espClient);
const char* outTopic = "Zombies/ESP8266";

volatile float tiempo0 = 0;
volatile float tiempo1 = 0;

int RPM = 0;
float VEL = 0.00;
float VELMAX = 0.00;
float omega = 0;
const float pi = 3.14159;
const float dosPi = 6.28318;
const float radioIman = 0.165;    //En principio no hace falta para calcular la velocidad angular
const float radioRueda = 0.165;   //hace falta para calcular la velocidad lineal del coche
const int hallPin = 5;

boolean detectado = false; //usar la flag, si se hace un publish mqtt dentro de pulsoRueda() rompe, es un bug conocido

void setup_wifi() {
  delay(10);
  // Conectamos con la red wifi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    for(int i = 0; i<500; i++){
      delay(1);
    }
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop hasta que conecte
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Intenta conectar
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Una vez conectado publica...
      client.publish(outTopic, "ESP8266 arrancado");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Espera 5 segundos antes de reintentarlo
      for(int i = 0; i<5000; i++){
        delay(1);
      }
    }
  }
}

void pulsoRueda(){
  tiempo1 = micros();
  omega = (dosPi*1000000 / (tiempo1 - tiempo0)); //Omega en radianes/segundo
  RPM = ((omega * 60) / dosPi);
  VEL = omega * radioRueda *3.6;   //Velocidad en Km/hora
  tiempo0 = tiempo1;

  //Publicando datos en el MQTT
  //client.publish(outTopic, "{'rpm':'', 'vel':'', 'velmax':''}");   //ROMPE
  detectado = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(hallPin, INPUT_PULLUP);
  setup_wifi();                   // Connect to wifi
  client.setServer(mqtt_server, 1883);

  //Usamos interrupciones para evitar que el arduino se pase todo el tiempo leyendo del pin
  attachInterrupt(hallPin, pulsoRueda, FALLING);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (VEL >= VELMAX){
    VELMAX = VEL;
  }

  //Publicar por MQTT datos del sensor por el puerto serie.
  // RPM, VEL, VELMAX
  if(detectado){
    detectado = false;

    String rpm = String(RPM);
    String vel = String(VEL, 2);
    String velmax = String(VELMAX, 2);
    String msgstr = "{'rpm':'" + rpm + "', 'vel':'" + vel + "', 'velmax':'" + velmax + "'}";

    char msg[128];
    msgstr.toCharArray(msg, 128);

    Serial.println(msg);
    client.publish(outTopic, msg);
  }

}
