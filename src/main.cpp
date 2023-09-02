/*
    MQTT sensor BME280
    20220628-20230728
    = Vlapa =  v.009
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;

const uint8_t pinBuiltinLed = D4;
const uint8_t pinBME280_SCL = 5;
const uint8_t pinBME280_SDA = 4;
const uint8_t pinBME280_gnd = 2;

const float corrTemper = 0.0; // коррекция температуры bme280_yama

const uint8_t pinPROGR = 13; // D7;   //  программрование (при запуске на землю)

// const char ssid[] = "MikroTik-2-ext";
// const char ssid[] = "MikroTik-2_gar";
// const char ssid[] = "link";
// const char pass[] = "dkfgf#*12091997";

const char ssid[] = "Elite_pers";
const char pass[] = "88888885";

const char *mqtt_server = "178.20.46.157";
const uint16_t mqtt_port = 1883;
const char *mqtt_client = "Home_bme280_002";
const char *mqtt_client2 = "Sensor_bme280_prog";
// const char *mqtt_client = "Villa_bme280_yama-2";
// const char *mqtt_client = "Villa_bme280_base";
const char *mqtt_user = "mqtt";
const char *mqtt_pass = "qwe#*1243";

const char *outTopicTemp = "/Temp";
const char *outTopicPres = "/Pres";
const char *outTopicHum = "/Hum";
const char *outTopicIP = "/IP";
const char *outTopicVcc = "/Vcc";

const uint32_t pauseSleep = 30 * 1000 * 1000; //  30 секунд спим
const uint16_t pauseOut = 200;                //  пауза после отправки, до засыпания

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
  Serial.print("Connection to:  ");
  Serial.println(ssid);
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
  Serial.print("\nConnected !\tIP: ");
  Serial.println(WiFi.localIP());
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

//**************************************************************
// медиана на 3 значения со своим буфером
// значение только uint32_t !
uint32_t median(uint32_t newVal, uint8_t offSet)
{
  uint32_t count;
  uint32_t buf[3];

  ESP.rtcUserMemoryRead(offSet, &count, sizeof(count));
  ESP.rtcUserMemoryWrite(count, &newVal, sizeof(newVal));

  uint8_t k = 0;
  for (uint8_t i = offSet - 3; i < offSet; ++i)
  {
    ESP.rtcUserMemoryRead(i, &buf[k], sizeof(buf[k]));
    Serial.print(buf[k]);
    Serial.print('\t');
    k++;
  }
  // Serial.println();

  if (++count >= offSet)
  {
    count = offSet - 3;
    ESP.rtcUserMemoryWrite(offSet, &count, sizeof(count));
  }
  else
  {
    ESP.rtcUserMemoryWrite(offSet, &count, sizeof(count));
  }

  // uint32_t a = (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
  // Serial.print("a = ");
  // Serial.println(a);
  return (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
}

//**************************************************************

void setup()
{
  Serial.begin(115200);
  Serial.print("\n\nSTART: ");
  Serial.println(millis());
  pinMode(pinBME280_gnd, OUTPUT);
  digitalWrite(pinBME280_gnd, LOW);
  pinMode(pinPROGR, INPUT_PULLUP);
  delay(1);

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  if (digitalRead(pinPROGR))
  {
    // pinMode(vccBme, OUTPUT);
    // digitalWrite(pinBuiltinLed, HIGH);

    // pinMode(analogInPin, INPUT);

    Wire.begin(pinBME280_SCL, pinBME280_SDA);
    Serial.print("bme280 test: ");
    if (bme.begin(0x76))
    {
      Serial.println("OK !");
    }
    else
    {
      Serial.println("FALSE !");
    }

    uint32_t d1 = median((uint32_t)((bme.readTemperature() + corrTemper) * 100), 3);
    uint32_t a1;
    ESP.rtcUserMemoryRead(4, &a1, sizeof(a1));

    if (a1 == d1)
    {
      pinMode(LED_BUILTIN, OUTPUT);
      digitalWrite(LED_BUILTIN, LOW);
      delay(1);
      ESP.deepSleep(pauseSleep);
    }
    else
    {
      ESP.rtcUserMemoryWrite(4, &d1, sizeof(d1));
    }

    delay(1);
    float u = (1.0 / 1023) * analogRead(A0) * 4.200;
    uint32_t u1 = u * 100;
    float u2 = median(u1, 16) / 100.0;
    Serial.print("U1 = ");
    Serial.println(u2);

    float t = d1 / 100.0;
    uint32_t p = (bme.readPressure() * 0.75 / 100);
    uint32_t h = bme.readHumidity();

    setupWiFi();
    reconnect();

    Serial.println();
    String topic = "/";
    topic += mqtt_client;
    topic += outTopicTemp;
    mqtt_publish(client, topic, (String)t);
    delay(10);

    topic = "/";
    topic += mqtt_client;
    topic += outTopicPres;
    mqtt_publish(client, topic, (String)p);
    delay(10);

    topic = "/";
    topic += mqtt_client;
    topic += outTopicHum;
    mqtt_publish(client, topic, (String)h);
    delay(10);

    topic = "/";
    topic += mqtt_client;
    topic += outTopicVcc;
    mqtt_publish(client, topic, (String)u2);

    delay(pauseOut);
    digitalWrite(pinBuiltinLed, HIGH);
    ESP.deepSleep(pauseSleep);
  }
  else
  {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    String topic = "/";
    topic += mqtt_client2;
    topic += outTopicIP;

    setupWiFi();
    reconnect();

    mqtt_publish(client, topic, WiFi.localIP().toString());

    ESP8266WebServer server(80);
    ESP8266HTTPUpdateServer httpUpdater;
    server.begin();
    httpUpdater.setup(&server);

    while (1)
    {
      server.handleClient();
      client.loop();
      delay(1);
    }
  }
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
