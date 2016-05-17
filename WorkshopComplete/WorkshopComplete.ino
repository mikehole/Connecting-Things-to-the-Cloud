#include <PubSubClient.h>
#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

char io_username[200] = "YOUR_IO_USERNAME";
char io_key[200] = "YOUR_IO_KEY";

//flag for saving data
bool shouldSaveConfig = false;

WiFiClientSecure client;
PubSubClient mqttclient("io.adafruit.com", 8883, subCallback, client);

// Input / Output stuff
// Witty Cloud Board specifc pins 
const int LDR = A0;
const int BUTTON = 4;
const int RED = 15;
const int GREEN = 12;
const int BLUE = 13;

//Values
String LDRvalue;
String OldLDRvalue;

String ButtonState;
String OldButtonState;

String ButtonFeedName;
String LdrFeedName;
String RgbFeedName;

//callback notifying us of the need to save config
void saveConfigCallback() {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}

void subCallback(char* topic, byte* payload, unsigned int length) {
	Serial.println(topic);
	Serial.write(payload, length);
	Serial.println("");

	String data;

	for (size_t i = 0; i < length; i++)
	{
		data += (char *)payload[i];
	}

	Serial.write("Data : ");
	Serial.println(data);

	Serial.write("RR : ");
	Serial.println(data.substring(1,2));

	Serial.write("GG : ");
	Serial.println(data.substring(3, 2));

	Serial.write("BB : ");
	Serial.println(data.substring(5, 2));
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	Serial.println();

	//clean FS, for testing
	//SPIFFS.format();

	//read configuration from FS json
	Serial.println("mounting FS...");

	if (SPIFFS.begin()) {
		Serial.println("mounted file system");
		if (SPIFFS.exists("/config.json")) {
			//file exists, reading and loading
			Serial.println("reading config file");
			File configFile = SPIFFS.open("/config.json", "r");
			if (configFile) {
				Serial.println("opened config file");
				size_t size = configFile.size();
				// Allocate a buffer to store contents of the file.
				std::unique_ptr<char[]> buf(new char[size]);

				configFile.readBytes(buf.get(), size);
				DynamicJsonBuffer jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(buf.get());
				json.printTo(Serial);
				if (json.success()) {
					Serial.println("\nparsed json");
					strcpy(io_username, json["io_username"]);
					strcpy(io_key, json["io_key"]);
				}
				else {
					Serial.println("failed to load json config");
				}
			}
		}
	}
	else {
		Serial.println("failed to mount FS");
	}
	//end read

	// The extra parameters to be configured (can be either global or just in the setup)
	// After connecting, parameter.getValue() will get you the configured value
	// id/name placeholder/prompt default length
	WiFiManagerParameter custom_io_username("username", "io user name", io_username, 200);
	WiFiManagerParameter custom_io_key("key", "io key", io_key, 200);

	//WiFiManager
	//Local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;

	//set config save notify callback
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	//add all your parameters here
	wifiManager.addParameter(&custom_io_username);
	wifiManager.addParameter(&custom_io_key);

	//reset settings - for testing
	//wifiManager.resetSettings();

	//set minimu quality of signal so it ignores AP's under that quality
	//defaults to 8%
	//wifiManager.setMinimumSignalQuality();

	//sets timeout until configuration portal gets turned off
	//useful to make it all retry or go to sleep
	//in seconds
	//wifiManager.setTimeout(120);

	//fetches ssid and pass and tries to connect
	//if it does not connect it starts an access point with the specified name
	//here  "AutoConnectAP"
	//and goes into a blocking loop awaiting configuration
	if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
		Serial.println("failed to connect and hit timeout");
		delay(3000);
		//reset and try again, or maybe put it to deep sleep
		ESP.reset();
		delay(5000);
	}

	//if you get here you have connected to the WiFi
	Serial.println("connected...yeey :)");

	//read updated parameters
	strcpy(io_username, custom_io_username.getValue());
	strcpy(io_key, custom_io_key.getValue());

	//save the custom parameters to FS
	if (shouldSaveConfig) {
		Serial.println("saving config");
		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.createObject();
		json["io_username"] = io_username;
		json["io_key"] = io_key;

		File configFile = SPIFFS.open("/config.json", "w");
		if (!configFile) {
			Serial.println("failed to open config file for writing");
		}

		json.printTo(Serial);
		json.printTo(configFile);
		configFile.close();
		//end save
	}

	Serial.println("");
	Serial.println("local ip");
	Serial.println(WiFi.localIP());

	ButtonFeedName = String(io_username) + "/feeds/button";

	LdrFeedName = String(io_username) + "/feeds/ldr";

	RgbFeedName = String(io_username) + "/feeds/rgb";

}

void loop() {

	if (!mqttclient.connected())
	{
		if (mqttclient.connect("widi", io_username, io_key)) {
			Serial.println(F("MQTT Connected"));
			
			mqttclient.subscribe(RgbFeedName.c_str());
		}
		else
		{
			Serial.println(F("MQTT Connection Failed"));
		}
	}
	else
	{
		ButtonState = digitalRead(BUTTON);
		if (OldButtonState != ButtonState)
		{
			mqttclient.publish(ButtonFeedName.c_str(), ButtonState.c_str());
			OldButtonState = ButtonState;
		}

		LDRvalue = map(analogRead(LDR), 100, 1024, 0, 255);
		if (OldLDRvalue != LDRvalue)
		{
			mqttclient.publish(LdrFeedName.c_str(), LDRvalue.c_str());
			OldLDRvalue = LDRvalue;
		}
	}

	mqttclient.loop();
	delay(250);
}

