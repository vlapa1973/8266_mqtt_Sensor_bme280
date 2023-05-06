/*
    MQTT sensor BME280
    20220628-20230103
    = Vlapa =  v.008
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;

// extern "C"
// {
// #include "user_interface.h"
//   extern struct rst_info resetInfo;
// }

// const uint8_t pinBuiltinLed = D4;
const uint8_t pinBME280_SCL = 5;
const uint8_t pinBME280_SDA = 4;
const uint8_t pinBME280_gnd = 2;

// const char ssid[] = "MikroTik-2-ext";
// const char ssid[] = "MikroTik-2_gar";
const char ssid[] = "link";
const char pass[] = "dkfgf#*12091997";

// const char *mqtt_server = "94.103.87.97";
const char *mqtt_server = "178.20.46.157";
const uint16_t mqtt_port = 1883;
const char *mqtt_client = "Home_bme280";
// const char *mqtt_client = "Villa_bme280_yama";
// const char *mqtt_client = "Villa_bme280_base";
const char *mqtt_user = "mqtt";
const char *mqtt_pass = "qwe#*1243";

const char *outTopicTemp = "/Temp";
const char *outTopicPres = "/Pres";
const char *outTopicHum = "/Hum";
// const char *outTopicVcc = "/Vcc";

const uint32_t pauseSleep = 30 * 1000 * 1000; //  30 секунд спим

WiFiClient espClient;
PubSubClient client(espClient);

// ADC_MODE(ADC_VCC);

inline bool mqtt_subscribe(PubSubClient &client, const String &topic)
{
  // Serial.print("Subscribing to: ");
  // Serial.println(topic);
  return client.subscribe(topic.c_str());
}

inline bool mqtt_publish(PubSubClient &client, const String &topic, const String &value)
{
  // Serial.print("Publishing topic ");
  // Serial.print(topic);
  // Serial.print(" = ");
  // Serial.println(value);
  return client.publish(topic.c_str(), value.c_str());
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  // for (uint8_t i = 0; i < length; i++)
    // Serial.print((char)payload[i]);
  // Serial.print("Pres: ");
  // Serial.println();

  // char *topicBody = topic + strlen(mqtt_client) + 1;
  // if (!strncmp(topicBody, inTopicTemp, strlen(inTopicTemp)))
  // {
  //   for (uint8_t i = 0; i < length; i++)
  //     outTempData += (char)payload[i];
  //   Serial.print("Temp: ");
  //   Serial.println(outTempData);
  //   outTempData = "";
  // }
  // else if (!strncmp(topicBody, inTopicPres, strlen(inTopicTemp)))
  // {
  //   for (uint8_t i = 0; i < length; i++)
  //     outPresData += (char)payload[i];
  //   Serial.print("Pres: ");
  //   Serial.println(outPresData);
  //   outPresData = "";
  // }
  // else if (!strncmp(topicBody, inTopicHum, strlen(inTopicTemp)))
  // {
  //   for (uint8_t i = 0; i < length; i++)
  //     outHumData += (char)payload[i];
  //   Serial.print("Hum : ");
  //   Serial.println(outHumData);
  //   outHumData = "";
  // }
}

void setupWiFi()
{
  uint8_t wifiCount = 20;
  WiFi.begin(ssid, pass);
  // Serial.print("Connection to:  ");
  // Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print('.');
    if (wifiCount)
    {
      wifiCount--;
    }
    else
    {
      ESP.deepSleep(pauseSleep);
    }
  }
  // Serial.print("\nConnected !\tIP: ");
  // Serial.println(WiFi.localIP());
}

void reconnect()
{
  uint8_t mqttCount = 20;
  while (!client.connected())
  {
    if (!client.connect(mqtt_client, mqtt_user, mqtt_pass))
    {
      if (mqttCount)
      {
        mqttCount--;
      }
      else
      {
        ESP.deepSleep(pauseSleep);
      }
      delay(300);
    }
  }
}

// медианный фильтр из 3-ёх значений
// float middle_of_3(float a, float b, float c)
// {
//   if ((a <= b) && (a <= c))
//   {
//     return (b <= c) ? b : c;
//   }
//   else
//   {
//     if ((b <= a) && (b <= c))
//     {
//       return (a <= c) ? a : c;
//     }
//     else
//     {
//       return (a <= b) ? a : b;
//     }
//   }
//   // return middle;
// }

void setup()
{
  // Serial.begin(115200);
  // Serial.print("\n\nSTART: ");
  // Serial.println(millis());
  pinMode(pinBME280_gnd, OUTPUT);
  digitalWrite(pinBME280_gnd, LOW);

  // pinMode(vccBme, OUTPUT);
  // digitalWrite(pinBuiltinLed, HIGH);

  // pinMode(analogInPin, INPUT);

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  setupWiFi();
  reconnect();

  Wire.begin(pinBME280_SCL, pinBME280_SDA);
  // Serial.print("bme280 test: ");
  if (bme.begin(0x76))
  {
    // Serial.println("OK !");
  }
  else
  {
    // Serial.println("FALSE !");
  }

  // Serial.println();
  String topic = "/";
  topic += mqtt_client;
  topic += outTopicTemp;
  float a = bme.readTemperature() + 1;
  mqtt_publish(client, topic, (String)a);

  topic = "/";
  topic += mqtt_client;
  topic += outTopicPres;
  mqtt_publish(client, topic, (String)((uint32_t)(bme.readPressure() * 0.75 / 100)));

  topic = "/";
  topic += mqtt_client;
  topic += outTopicHum;
  mqtt_publish(client, topic, (String)(uint32_t)bme.readHumidity());

  // float dist_3[3];
  // float dist; //, dist_filtered, k;

  // dist_3[0] = analogRead(analogInPin) * 4.1 / 1000;
  // dist_3[1] = analogRead(analogInPin) * 4.1 / 1000;
  // dist_3[2] = analogRead(analogInPin) * 4.1 / 1000;

  // dist = middle_of_3(dist_3[0], dist_3[1], dist_3[2]);

  // uint32_t distU32_3[3];
  // uint32_t distU32;
  // distU32_3[0] = analogRead(analogInPin);
  // distU32_3[1] = analogRead(analogInPin);
  // distU32_3[2] = analogRead(analogInPin);
  // distU32 = middle_of_3(distU32_3[0], distU32_3[1], distU32_3[2]);

  // for (uint8_t j = 2; j < 3; --j) {
  //   ESP.rtcUserMemoryRead(1, ESP.rtcUserMemoryWrite(2, ))
  // }

  // ESP.rtcUserMemoryWrite(0, &distU32, sizeof(distU32));

  // (abs(dist_filtered - dist) > 1) ? k = 0.3 : k = 0.1;
  // dist_filtered = (dist * k + dist_filtered * (1 - k)) * 1.4208; // фильтр "бегущее среднее"

  // Serial.println(dist);

  // topic = "/";
  // topic += mqtt_client;
  // topic += outTopicVcc;
  // mqtt_publish(client, topic, (String)dist);

  // digitalWrite(pinBuiltinLed, HIGH);
  // Serial.println("\nESP sleep in 30s.....");

  // digitalWrite(4, LOW);
  // digitalWrite(5, LOW);
  // Serial.print("\nEND: ");
  // Serial.println(millis() + 20);
  delay(300);
  digitalWrite(D4, HIGH);
  ESP.deepSleep(pauseSleep);
}

void loop()
{
  // float voltage = (float)(analogRead(analogInPin) * 4.1 / 1000);
  // Serial.println(voltage);

  // String topic = "/";
  // topic += mqtt_client;
  // topic += outTopicVcc;
  // float voltage = (float)(analogRead(analogInPin));
  // mqtt_publish(client, topic, (String)voltage);
  // delay(1000);
}
