#include <PubSubClient.h>
#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

const char *ssid = "SOMEWIFI";			// cannot be longer than 32 characters!
const char *pass = "HIDDEN";		//

//flag for saving data
bool shouldSaveConfig = false;

WiFiClient client;

PubSubClient mqttclient(client, "broker.hivemq.com");

// Input / Output stuff
// Witty Cloud Board specifc pins 
const int LDR = A0;
const int BUTTON = 4;
const int RED = 15;
const int GREEN = 12;
const int BLUE = 13;

//Values
String	LDRvalue;
String	OldLDRvalue;

int		ButtonState;
int		OldButtonState;

String	ButtonFeedName;
String	LdrFeedName;
String	RedFeedName;
String	GreenFeedName;
String	BlueFeedName;

//callback notifying us of the need to save config
void saveConfigCallback() {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}

void callback(const MQTT::Publish& pub) {
	// handle message arrived

	Serial.print("Value (");
	Serial.print(pub.topic());
	Serial.print(") :");
	Serial.println(pub.payload_string());

	

	if (pub.topic() == RedFeedName)
	{
		analogWrite(RED, pub.payload_string().toInt());
	}

	if (pub.topic() == GreenFeedName)
	{
		analogWrite(GREEN, pub.payload_string().toInt());
	}

	if (pub.topic() == BlueFeedName)
	{
		analogWrite(BLUE, pub.payload_string().toInt());
	}
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	Serial.println();

	// Initialize LDR, Button and RGB LED 
	pinMode(LDR, INPUT);
	pinMode(BUTTON, INPUT);
	pinMode(RED, OUTPUT);
	pinMode(GREEN, OUTPUT);
	pinMode(BLUE, OUTPUT);

	ButtonFeedName =	"tecmarina/" + String(ESP.getChipId()) + "/feeds/button";
	LdrFeedName =		"tecmarina/" + String(ESP.getChipId()) + "/feeds/ldr";

	RedFeedName =		"tecmarina/" + String(ESP.getChipId()) + "/input/rgb/r";
	GreenFeedName =		"tecmarina/" + String(ESP.getChipId()) + "/input/rgb/g";
	BlueFeedName =		"tecmarina/" + String(ESP.getChipId()) + "/input/rgb/b";

}

void loop() {

	if (WiFi.status() != WL_CONNECTED) {
		Serial.print("Connecting to ");
		Serial.print(ssid);
		Serial.println("...");
		
		WiFi.begin(ssid, pass);

		while (WiFi.status() != WL_CONNECTED) {
			delay(500);
			Serial.print(".");
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

				mqttclient.subscribe(RedFeedName);
				mqttclient.subscribe(GreenFeedName);
				mqttclient.subscribe(BlueFeedName);

			}
			else
			{
				Serial.println("Could not connect to MQTT server");
			}
		}
		else
		{
			ButtonState = digitalRead(BUTTON);
			if (OldButtonState != ButtonState)
			{
				Serial.print("Sending button state (");
				Serial.print(ButtonFeedName);
				Serial.print(") : ");

				if (ButtonState == 1)
				{
					mqttclient.publish(ButtonFeedName.c_str(), "0");
					Serial.println("0");
				}
				else
				{
					mqttclient.publish(ButtonFeedName.c_str(), "1");
					Serial.println("1");
				}

				OldButtonState = ButtonState;
			}

			LDRvalue = map(analogRead(LDR), 100, 1024, 0, 255);
			if (OldLDRvalue != LDRvalue)
			{
				Serial.print("Sending ldr value (");
				Serial.print(LdrFeedName);
				Serial.print(") : ");
				Serial.println(LDRvalue);

				mqttclient.publish(LdrFeedName.c_str(), LDRvalue.c_str());

				OldLDRvalue = LDRvalue;
			}
		}

		if (mqttclient.connected())
			mqttclient.loop();

		delay(250);
	}
}

