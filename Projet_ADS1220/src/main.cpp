#include <Arduino.h>
#include <WiFi.h>
#include <Protocentral_ADS1220.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==== CONFIGURATION WIFI ====

const char* ssid = "Livebox";
const char* password = "Password";

// === CONFIGURATION THINGSBOARD ===

const char* mqtt_server = "192.168.1.30";
const int mqtt_port = 1883;
const char* access_token = "zlH9J2gbmHYNKHLy5mhL";

WiFiClient espClient;
PubSubClient client(espClient);

// === CONFIGURATION ADS1220 ===
#define PGA 1
#define VREF 2.048
#define FULL_SCALE (((long int)1<<23)-1)
#define ADS1220_CS_PIN 5
#define ADS1220_DRDY_PIN 4

Protocentral_ADS1220 pc_ads1220;
int32_t adc_data;

//Prototype
float convertToVolt(int32_t i32data);
void wifi();
void reconnect();
float readADS1220Temperature(Protocentral_ADS1220& adc);
void rpcCallback(char* topic, byte* payload, unsigned int length);
void publishSettings();
int decodePGA(uint8_t reg0);
int decodeDR(uint8_t reg1);

void setup(){

  Serial.begin(115200);
  delay(1000);

  wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(rpcCallback);


  //Initialisation du capteur ADS1220
  pc_ads1220.begin(ADS1220_CS_PIN, ADS1220_DRDY_PIN);
  pc_ads1220.set_data_rate(DR_20SPS);
  pc_ads1220.set_pga_gain(PGA_GAIN_1);
  pc_ads1220.PGA_ON();
  pc_ads1220.set_conv_mode_single_shot();

  Serial.println("Configuration terminée, démarrage...");
  delay(50);
  publishSettings();
}

void loop(){

  if(!client.connected()){
    reconnect();
  }
  client.loop();

  // Lecture donnée
  adc_data = pc_ads1220.Read_SingleShot_SingleEnded_WaitForData(MUX_AIN0_AIN1);

  float ch1 =convertToVolt(adc_data);
  float temperature =readADS1220Temperature(pc_ads1220);
  
  //Affichage
  Serial.print("Tension mesurée :");
  Serial.printf("Ch1: %.3f V ", ch1);

  Serial.print("Temperature :");
  Serial.printf("Temp: %.3f °C ", temperature);

  //Construction du message à envoyer
  StaticJsonDocument<128> doc;
  doc["ch1"] = ch1;
  doc["temperature"]= temperature;
  String pload;
  serializeJson(doc, pload);

  //Envoi vers Thingsboard
  client.publish("v1/devices/me/telemetry" , pload.c_str());
  delay(2000);

}

float convertToVolt(int32_t i32data){
  uint8_t reg0 = pc_ads1220.readRegister(CONFIG_REG0_ADDRESS);
  int pga = decodePGA(reg0);

  if (pga <= 0)
  {
    pga=1;
  }

  float VFSR  =VREF / (float)pga;
  
  return (float) ((i32data * VFSR ) / FULL_SCALE);
}

void wifi(){
  Serial.print("Connexion au Wifi : ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWifi connecté !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

void reconnect(){
  while (!client.connected())
  {
    Serial.print("Connexion à Thingsboard...");

    if (client.connect("ESP32_Device", access_token, NULL))
    {
      Serial.println("connecté !");
      client.subscribe("v1/devices/me/rpc/request/+");
      publishSettings();
    }else{
      Serial.print("Echec, code erreur = ");
      Serial.println(client.state());
      Serial.println("Nouvelle tentative dans 5s...");
      delay(5000);
    }
    
  }
  
}


float readADS1220Temperature(Protocentral_ADS1220& adc){
  adc.TemperatureSensorMode_enable();
  delay(50);
  int32_t raw_temp = adc.Read_SingleShot_WaitForData();
  float temperature = (raw_temp /1000.0 * 0.03125);
  adc.TemperatureSensorMode_disable();
  delay(50);
  return temperature;

}

void rpcCallback(char* topic, byte* payload, unsigned int length){
  String message;

  for (unsigned int i=0; i<length; i++)
  {
    message +=(char)payload[i];
  }
  Serial.println("RPC reçu : " + message);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error)
  {
    Serial.print("Erreur JSON RPC " );
    Serial.println(error.c_str());
  }

  JsonVariant tab = doc["params"].isNull() ? doc : doc["params"];
  bool changed = false;

  //PGA
  if (tab["pga"].is<int>())
  {
    int new_pga =tab["pga"].as<int>();
    Serial.printf("Modification PGA demandée : %d\n", new_pga);

    switch(new_pga){
      case 1: pc_ads1220.set_pga_gain(PGA_GAIN_1); 
        changed = true; 
        break;
      case 2: pc_ads1220.set_pga_gain(PGA_GAIN_2); 
        changed = true; 
        break;
      case 4: pc_ads1220.set_pga_gain(PGA_GAIN_4); 
        changed = true; 
        break;
      case 8: pc_ads1220.set_pga_gain(PGA_GAIN_8); 
        changed = true; 
        break;
      case 16: pc_ads1220.set_pga_gain(PGA_GAIN_16); 
        changed = true; 
        break;
      case 32: pc_ads1220.set_pga_gain(PGA_GAIN_32); 
        changed = true; 
        break;
      case 64: pc_ads1220.set_pga_gain(PGA_GAIN_64); 
        changed = true; 
        break;
      case 128: pc_ads1220.set_pga_gain(PGA_GAIN_128); 
        changed = true; 
        break;
      
      default : Serial.println("PGA invalide (1,2,4,8,16,32,64,128)");
    }
  }

  //Data Rate
  if (tab["dr"].is<int>())
  {
    int new_dr = tab["dr"].as<int>();
    Serial.printf("Modification DataRate demandée : %d\n", new_dr);

    switch (new_dr)
    {
    case 20: pc_ads1220.set_data_rate(DR_20SPS);
      changed = true;
      break;
    case 45: pc_ads1220.set_data_rate(DR_45SPS);
      changed = true;
      break;
    case 90: pc_ads1220.set_data_rate(DR_90SPS);
      changed = true;
      break;
    case 175: pc_ads1220.set_data_rate(DR_175SPS);
      changed = true;
      break;
    case 330: pc_ads1220.set_data_rate(DR_330SPS);
      changed = true;
      break;
    case 600: pc_ads1220.set_data_rate(DR_600SPS);
      changed = true;
      break;
    case 1000: pc_ads1220.set_data_rate(DR_1000SPS);
      changed = true;
      break;
    
    default:Serial.println("DataRate invalide (20,45,90,175,330,600,1000)");
    }
  }

  if (changed)
  {
    delay(10);
    //Relancer une conversion 
    pc_ads1220.Start_Conv();
    publishSettings();
  }
  
}

void publishSettings(){
  uint8_t reg0 = pc_ads1220.readRegister(CONFIG_REG0_ADDRESS);
  uint8_t reg1 = pc_ads1220.readRegister(CONFIG_REG1_ADDRESS);

  int pga_val = decodePGA(reg0);
  int dr_val = decodeDR(reg1);

  StaticJsonDocument<128> doc;
  doc["pga"]= pga_val;
  doc["dr"]= dr_val;
  String payload;
  serializeJson(doc, payload);
  client.publish("v1/devices/me/attributes", payload.c_str());
  Serial.println("Attributs ADC envoyés : " +payload);
}

//Convertit le champ du registre en gain lisible
int decodePGA(uint8_t reg0){
  uint8_t pga_bits = reg0 & REG_CONFIG0_PGA_GAIN_MASK;
  switch (pga_bits)
  {
  case PGA_GAIN_1: return 1;
  case PGA_GAIN_2: return 2;
  case PGA_GAIN_4: return 4;
  case PGA_GAIN_8: return 8;
  case PGA_GAIN_16: return 16;
  case PGA_GAIN_32: return 32;
  case PGA_GAIN_64: return 64;
  case PGA_GAIN_128: return 128;
  default: return -1;
  }
}

//Convertit le champ DR du registre en SPS lisible

int decodeDR(uint8_t reg1){
  uint8_t dr_bits =reg1 & REG_CONFIG1_DR_MASK;
  switch (dr_bits)
  {
  case DR_20SPS: return 20;
  case DR_45SPS: return 45;
  case DR_90SPS: return 90;
  case DR_175SPS: return 175;
  case DR_330SPS: return 330;
  case DR_600SPS: return 600;
  case DR_1000SPS: return 1000;
  default: return -1;
  }
}
