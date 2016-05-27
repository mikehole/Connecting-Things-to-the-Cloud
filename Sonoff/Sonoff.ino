#include <PubSubClient.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

const char *ssid = "workshop";			// cannot be longer than 32 characters!
const char *pass = "workshop2016";		//

									//flag for saving data
bool shouldSaveConfig = false;

WiFiClient client;

PubSubClient mqttclient(client, "40.127.166.94");

#define SONOFF_BUTTON    0 
#define SONOFF_RELAY    12
#define SONOFF_LED      13
#define SONOFF_INPUT    14

String	RelayFeedName;

void callback(const MQTT::Publish& pub) {
	// handle message arrived

	Serial.print("Value (");
	Serial.print(pub.topic());
	Serial.print(") :");
	Serial.println(pub.payload_string());

	if (pub.payload_string() == "1")
	{
		digitalWrite(SONOFF_RELAY, HIGH);
		digitalWrite(SONOFF_LED, LOW);
	}
	else
	{
		digitalWrite(SONOFF_RELAY, LOW);
		digitalWrite(SONOFF_LED, HIGH);
	}
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	Serial.println();

	RelayFeedName = "tecmarina/feeds/relay";

	pinMode(SONOFF_RELAY, OUTPUT);
	pinMode(SONOFF_LED, OUTPUT);


}

void loop() {

	if (WiFi.status() != WL_CONNECTED) {
		Serial.print("Connecting to ");
		Serial.print(ssid);
		Serial.println("...");

		WiFi.begin(ssid, pass);

		while (WiFi.status() != WL_CONNECTED) {
			digitalWrite(SONOFF_LED, HIGH);
			delay(500);
			digitalWrite(SONOFF_LED, LOW);
			Serial.print(".");
			delay(500);
		}
		Serial.println("");

		Serial.println("WiFi connected");
	}

	if (WiFi.status() == WL_CONNECTED) {
		if (!mqttclient.connected())
		{
			Serial.println("Connecting to MQTT server");

			if (mqttclient.connect("witty")) {

				Serial.println("Connected to MQTT server");

				mqttclient.set_callback(callback);

				mqttclient.subscribe(RelayFeedName);

			}
			else
			{
				Serial.println("Could not connect to MQTT server");
			}
		}
		else
		{
		}

		if (mqttclient.connected())
			mqttclient.loop();

		delay(250);
	}
}