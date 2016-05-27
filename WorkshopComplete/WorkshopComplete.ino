#include <PubSubClient.h>
#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

const char *ssid = "workshop";			// cannot be longer than 32 characters!
const char *pass = "workshop2016";		//

//flag for saving data
bool shouldSaveConfig = false;

WiFiClient client;

PubSubClient mqttclient(client, "40.127.166.94");

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
String	RGBFeedName;

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

	long unsigned int rgb = strtoul(pub.payload_string().c_str(), 0, 16);

	int red = rgb >> 16;

	int green = (rgb & 0x00ff00) >> 8;

	int blue = (rgb & 0x0000ff);

	Serial.write("RR : ");
	Serial.println(red);
	analogWrite(RED, red * 4);
	Serial.write("GG : ");
	Serial.println(green );
	analogWrite(GREEN, green * 4);
	Serial.write("BB : ");
	Serial.println(blue);
	analogWrite(BLUE, blue * 4);
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

	ButtonFeedName	=	"tecmarina/" + String(ESP.getChipId()) + "/feeds/button";
	LdrFeedName		=	"tecmarina/" + String(ESP.getChipId()) + "/feeds/ldr";
	RGBFeedName		=	"tecmarina/" + String(ESP.getChipId()) + "/input/rgb";
}

void loop() {

	if (WiFi.status() != WL_CONNECTED) {
		Serial.print("Connecting to ");
		Serial.print(ssid);
		Serial.println("...");
		
		WiFi.begin(ssid, pass);

		while (WiFi.status() != WL_CONNECTED) {
			analogWrite(RED, 1024);
			delay(500);
			Serial.print(".");
			analogWrite(RED, 0);
			delay(500);
		}
		Serial.println("");
		analogWrite(RED, 0);
		analogWrite(GREEN, 1024);
		delay(500);
		analogWrite(GREEN, 0);

		Serial.println("WiFi connected");
	}

	if (WiFi.status() == WL_CONNECTED) {
		if (!mqttclient.connected())
		{
			analogWrite(RED, 0);
			analogWrite(GREEN, 0);
			analogWrite(BLUE, 1024);

			Serial.println("Connecting to MQTT server");

			if (mqttclient.connect("witty" + String(ESP.getChipId()))) {
				delay(500);
				analogWrite(BLUE, 0);

				Serial.println("Connected to MQTT server");

				mqttclient.set_callback(callback);

				mqttclient.subscribe(RGBFeedName);

				analogWrite(GREEN, 1024);
				delay(500);
				analogWrite(GREEN, 0);

			}
			else
			{
				analogWrite(RED, 1024);
				delay(500);
				analogWrite(RED, 0);

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

