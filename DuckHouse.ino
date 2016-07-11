// which analog pin to connect
#define THERMISTOR_INTERIOR_PIN A3
#define THERMISTOR_EXTERIOR_PIN A4
// resistance at 25 degrees C
#define THERMISTORNOMINAL 50000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000
// LCD Column -- must be 12 or 13
#define LCD_START_COLUMN 7
#define VOLTAGE_POSITION 10
// Loop delay
#define LOOP_DELAY 500
// Relay pins
#define DOOR_UP_PIN 13
#define DOOR_DN_PIN 10
#define FAN_PIN 8
// Photoresistor pin
#define PHOTO_PIN A1
// Button Pin
#define BUTTON_PIN 7
// Light level at which door goes up
#define LIGHT_THRESHHOLD 300
// Turn fan on at fahernheit temp
#define TEMP_THRESHHOLD 78
#define UP true
#define DOWN false



boolean doorDirection; // true=down, false=up
boolean isFanOn = false, isDoorUp = false, isDaylight=true, doorlock=false;
int buttonState, samples[NUMSAMPLES], interiorTemperature, exteriorTemperature;
float steinhart, resistance, average, temperature;

// include the library code:
#include <LiquidCrystal.h>
//#include <timer.h>
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
	// set up the LCD
	lcd.begin(16, 2);
	// start the Thermister
	Serial.begin(9600);
	analogReference(EXTERNAL);
	pinMode(DOOR_UP_PIN, OUTPUT); 
	pinMode(DOOR_DN_PIN, OUTPUT);
	pinMode(FAN_PIN, OUTPUT);
	pinMode(BUTTON_PIN, INPUT);
	fanOff();
}

void loop() {
	interiorTemperature = readTemperature(THERMISTOR_INTERIOR_PIN);
	exteriorTemperature = readTemperature(THERMISTOR_EXTERIOR_PIN);
	displayLCD(interiorTemperature, exteriorTemperature);
	isDaylight = getDaylight();
	if (!isDaylight) 
		doorlock = false;
	getButton(BUTTON_PIN);
	//voltage = getVoltage();
	printDebug();
	if ((isDaylight) && !isDoorUp && !doorlock)
		doorMove(UP);
	if ((interiorTemperature > TEMP_THRESHHOLD) && !isFanOn)
		fanOn();
	else if ((interiorTemperature < TEMP_THRESHHOLD) && isFanOn)
		fanOff();

	delay(LOOP_DELAY);
}

int displayLCD(int intTemp, int extTemp) {
	lcd.setCursor(0,0);
	lcd.print("Int:"); 
	lcd.setCursor(LCD_START_COLUMN, 0);
	lcd.print((char)223);
	lcd.setCursor(0, 1);
	lcd.print("Ext:");
	lcd.setCursor(LCD_START_COLUMN, 1);
	lcd.print((char)223);
	lcd.setCursor(LCD_START_COLUMN - 3, 0);
	lcd.print("  ");
	lcd.setCursor(LCD_START_COLUMN - 3, 1);
	lcd.print("  ");
	lcd.setCursor(getTempLCDPosition(intTemp), 0);
	lcd.print((String)intTemp);
	lcd.setCursor(getTempLCDPosition(extTemp), 1);
	lcd.print((String)extTemp);
	/*lcd.setCursor(VOLTAGE_POSITION, 0);
	lcd.print("Volts");
	lcd.setCursor(11, 1);
	lcd.print((String)voltage);*/
}

int getTempLCDPosition(int temp) {
	String strTemp;
	strTemp = (String)temp;

	return LCD_START_COLUMN-strTemp.length();
}

float getTempFromResistance(float resistance) {
	
	steinhart = resistance / THERMISTORNOMINAL;     // (R/Ro)
	steinhart = log(steinhart);                  // ln(R/Ro)
	steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
	steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
	steinhart = 1.0 / steinhart;                 // Invert
	steinhart -= 273.15;                         // convert to C
	steinhart = steinhart * 9 / 5 + 32;			 // convert to F
	return steinhart;
}

int readTemperature(uint8_t pin) {
	uint8_t i;

	// take N samples in a row, with a slight delay
	for (i = 0; i < NUMSAMPLES; i++) {
		samples[i] = analogRead(pin);
		delay(10);
	}

	// average all the samples out
	average = 0;
	for (i = 0; i < NUMSAMPLES; i++) 
		average += samples[i];
	average /= NUMSAMPLES;

	// convert the value to resistance
	resistance = SERIESRESISTOR / (1023 / average - 1);
	temperature = getTempFromResistance(resistance);

  return (int)temperature;
}

//float getVoltage()
//{	int sample_count = 0;
//	int sum = 0;
//	// take a number of analog samples and add them up
//	while (sample_count < NUMSAMPLES) {
//		sum += analogRead(A2);
//		sample_count++;
//		delay(10);
//	}
//	// calculate the voltage
//	// use 5.0 for a 5.0V ADC reference voltage
//	// 5.015V is the calibrated reference voltage
//	voltage = ((float)sum / (float)NUMSAMPLES * 5.015) / 1024.0;
//	// send voltage for display on Serial Monitor
//	// voltage multiplied by 11 when using voltage divider that
//	// divides by 11. 11.132 is the calibrated voltage divide
//	// value
//	Serial.print(voltage);
//	Serial.println(" V");
//
//}

void printDebug() {
	Serial.println("Interior=" + (String)interiorTemperature);
	Serial.println("Exterior=" + (String)exteriorTemperature);
	Serial.println("Door direction=" + trueFalse(doorDirection));
	Serial.println("Daylight=" + trueFalse(isDaylight));
	Serial.println("doorIsUp=" + trueFalse(isDoorUp));
	Serial.println("doorLock=" + trueFalse(doorlock));
	Serial.println("");
}

void getButton(uint8_t pin) {
	buttonState = digitalRead(pin);
	if (buttonState == HIGH) {
		Serial.println("Button Down");
		doorMove();
		delay(3000);
		doorStop();
	}
}

void doorStop() {
	digitalWrite(DOOR_UP_PIN, HIGH);
	digitalWrite(DOOR_DN_PIN, HIGH);
	Serial.println("Door Stop");

	doorDirection = !doorDirection;
}

void doorMove() {
	digitalWrite(DOOR_UP_PIN, doorDirection);
	digitalWrite(DOOR_DN_PIN, !doorDirection);
	isDoorUp = doorDirection;
	doorlock = (!isDoorUp && isDaylight);
	Serial.println("Door Moving: " + (String)((doorDirection) ? "Up" : "Down"));
}



void doorMove(boolean direction) {
	doorDirection = direction;
	doorMove();
		delay(3000);
		doorStop();
}

boolean getDaylight() {	
	uint8_t i;
	// take N samples in a row, with a slight delay
	for (i = 0; i < NUMSAMPLES; i++) {
		samples[i] = analogRead(PHOTO_PIN);
		delay(10);
	}

	// average all the samples out
	average = 0;
	for (i = 0; i < NUMSAMPLES; i++) 
		average += samples[i];
	average /= NUMSAMPLES; 
	Serial.println("Light Value=" + (String)average);
	return (average >= LIGHT_THRESHHOLD);
}

void fanOn() {
	digitalWrite(FAN_PIN, false);
	isFanOn = true;
	Serial.println("Fan On");
}

void fanOff() {
	digitalWrite(FAN_PIN, true);
	isFanOn = false;
	Serial.println("Fan Off");
}

String trueFalse(boolean i) {
	return (i == 0) ? "False" : "True";
}