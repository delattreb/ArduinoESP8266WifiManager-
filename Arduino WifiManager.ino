#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "var.h"

char nodeServer[] = IP_SERVER;
WiFiClient wifiClient;
PubSubClient client(wifiClient);
String data, sensor[MAXSENSOR * 2];
const int CHAR = 48;
static unsigned long previousMillis = 0;
unsigned long currentMillis;

//
// callback
//
void callback(char *topic, byte *payload, unsigned int length)
{
}

//
// reconnect
//
void reconnect()
{
	// Connect to MQTT
	if (!client.connected())
	{
#ifdef INFO
		Serial.println("Attempting MQTT connection...");
#endif
		while (!client.connected())
		{
			client.connect(NETWORKNAME, MQTT_USER, MQTT_PWD);
			Serial.println("MK");
			delay(ATTENPTING);
			Serial.println("S" + String(abs(WiFi.RSSI())));
		}
#ifdef INFO
		Serial.println("");
		Serial.println("MQTT connected");
#endif
		Serial.println("MO");
	}
}

//
// setup
//
void setup()
{
	Serial.begin(SERIALBAUDS);
	while (!Serial) continue;
	Serial.println("WK");
#ifdef INFO
	delay(5000);
	Serial.print("Core version: ");
	Serial.println(ESP.getCoreVersion());
	Serial.print("Sdk version: ");
	Serial.println(ESP.getSdkVersion());
	Serial.print("MAC: ");
	Serial.println(WiFi.macAddress());
#endif
	WiFiManager wifiManager;
	//Reset setting
	//wifiManager.resetSettings();

	wifiManager.setAPStaticIPConfig(IPAddress(IPLOWA, IPLOWB, IPLOWC, IPLOWD), IPAddress(IPHIGHA, IPHIGHB, IPHIGHC, IPHIGHD), IPAddress(255, 255, 255, 0));
#ifdef WIFIDEBUG
	wifiManager.setDebugOutput(true);
#else
	wifiManager.setDebugOutput(false);
#endif

	if (!wifiManager.autoConnect(NETWORKNAME))
	{
#ifdef DEBUG
		Serial.println("Failed to connect");
#endif
		delay(1000);
		ESP.reset();
		delay(5000);
	}
	Serial.println("WO");
	client.setServer(nodeServer, MQTTPORT);
	client.setCallback(callback);
	reconnect();
	for (int i = 0; i < (MAXSENSOR * 2); i++)
		sensor[i] = "";
}

//
// loop
//
void loop()
{
	currentMillis = millis();
	reconnect();
	if (currentMillis - previousMillis >= DB_FREQUENCY)
	{
		Serial.println("S" + String(abs(WiFi.RSSI())));
		previousMillis = currentMillis;
	}
	if (client.connected())
		client.loop();
	// Check if data available
	if (Serial.available())
	{
		data = Serial.readString();
		// get data
		if (data.startsWith("T", 0))
		{
			sensor[int(data[1]) - CHAR - 1] = data.substring(3, data.length() - 2);
#ifdef DEBUG
			Serial.println(data);
#endif
		}
		if (data.startsWith("H", 0))
		{
			sensor[int(data[1]) - CHAR] = data.substring(3, data.length() - 2);
#ifdef DEBUG
			Serial.println(data);
#endif
		}
	}

	// Transmit Data
	if (sensor[int(data[1]) - CHAR - 1].length() > 0 && sensor[int(data[1]) - CHAR].length() > 0)
	{
		sendMQTT(data[1], sensor[int(data[1]) - CHAR - 1], sensor[int(data[1]) - CHAR]);
		sensor[int(data[1]) - CHAR - 1] = "";
		sensor[int(data[1]) - CHAR] = "";
	}
}

//
// sendMQTT
//
void sendMQTT(char sensor, String temp, String hum)
{
	// Send payload
	String topic = TOPIC_IOT;
	String str;
	const int capacity = JSON_OBJECT_SIZE(3);
	StaticJsonBuffer<capacity> payload;
	JsonObject& obj = payload.createObject();

	char attrh[100];
	char attrt[100];

	// Make payload
	str = "h";
	str.concat(sensor);
	obj.set("name", str);
	obj.set("value", hum.c_str());
	obj.printTo(attrh, sizeof(attrh));
	client.publish(topic.c_str(), attrh);

	// Make payload
	str = "t";
	str.concat(sensor);
	obj.set("name", str);
	obj.set("value", temp.c_str());
	obj.printTo(attrt, sizeof(attrt));
	client.publish(topic.c_str(), attrt);

	if (client.connected())
	{
#ifdef DEBUG
		Serial.println("Before send to MQTT broker:");
		Serial.println(temp);
		Serial.println(attrt);
		Serial.println(hum);
		Serial.println(attrh);
		Serial.println(sensor);
#endif
	}
}
