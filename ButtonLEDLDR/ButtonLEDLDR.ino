// Witty Cloud Board specifc pins 
const int LDR = A0;
const int BUTTON = 4;
const int RED = 15;
const int GREEN = 12;
const int BLUE = 13;

String LDRvalue;
String ButtonState;

void setup()
{
	Serial.begin(115200);
	Serial.print("Hello From ESP8266! ");
	Serial.println("");

	// Initialize LDR, Button and RGB LED 
	pinMode(LDR, INPUT);
	pinMode(BUTTON, INPUT);
	pinMode(RED, OUTPUT);
	pinMode(GREEN, OUTPUT);
	pinMode(BLUE, OUTPUT);
}

void loop()
{

	analogWrite(RED, 255);
	analogWrite(GREEN, 0);
	analogWrite(BLUE, 1024);

	LDRvalue = analogRead(LDR);

	ButtonState = digitalRead(BUTTON);

	Serial.println("LDR: " + LDRvalue + "\t" + "Button: " + ButtonState);

	delay(200);
}
