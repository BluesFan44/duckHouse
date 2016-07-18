//#include <DuckHouseConstants.h>
#include <Multiplexer.h>
//#include <Servo.h>
#include <TimerOne.h>
#include <LiquidCrystal.h>

// Pin Assignments
#define PHOTO_PIN A1
#define THERMISTOR_INTERIOR_PIN A3
#define THERMISTOR_EXTERIOR_PIN A4
#define FAN_PIN 11
//#define DOOR_DN_PIN 12
//#define DOOR_UP_PIN 13
//#define DOOR_BUTTON_PIN 10
//#define LCD_BACKLIGHT_BUTTON_PIN 10
#define LCD_BACKLIGHT_PIN 7

// which analog pin to connect
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
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
#define LCD_INT_COLUMN 7
#define LCD_EXT_COLUMN 19
//#define VOLTAGE_POSITION 10
// Loop delay
#define LOOP_DELAY 500

// Light level at which door goes up
#define LIGHT_THRESHHOLD 100
// Turn fan on at fahernheit temp
#define TEMP_THRESHHOLD 78
#define UP true
#define DOWN false
//#define SERVO_PIN 6
#define TIMER_MICROSECONDS 60000000

#define BYTES_VAL_T unsigned int

BYTES_VAL_T pinValue;
BYTES_VAL_T oldPinValues;

boolean doorDirection; // true=down, false=up
boolean isFanOn = false, isDoorUp = false, isDaylight = true, doorlock = false;
int reading = LOW, lastButtonState = LOW, buttonState, samples[NUMSAMPLES], interiorTemperature, exteriorTemperature;
float steinhart, resistance, average, temperature, light;
//Servo myServo;
long lastDebounceTime = 0;
TimerOne timer1;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 6, 2, 3, 4, 5);

Multiplexer multi(10, 11, 12, 13);

void setup() {
	Serial.begin(9600);
	timer1.initialize(TIMER_MICROSECONDS);
	timer1.disablePwm(9);
	timer1.disablePwm(10);
	timer1.attachInterrupt(printDebug);

	// set up the LCD
	lcd.begin(20, 4);
	// start the Thermister
	analogReference(EXTERNAL);
	//pinMode(DOOR_UP_PIN, OUTPUT); 
	//pinMode(DOOR_DN_PIN, OUTPUT);
	pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
	//pinMode(FAN_PIN, OUTPUT);
	//pinMode(DOOR_BUTTON_PIN, INPUT);   
	//fanOff();
	printDebug();
}

void loop() {/* Read the state of all zones.
	*/
	pinValue = multi.read_shift_regs();

	/* If there was a chage in state, display which ones changed.
	*/
	if (pinValue != oldPinValues)
	{
		Serial.print("*Pin value change detected* \r\n");
		multi.display_pin_values();
		oldPinValues = pinValue;
	}

	interiorTemperature = readTemperature(THERMISTOR_INTERIOR_PIN);
	exteriorTemperature = readTemperature(THERMISTOR_EXTERIOR_PIN);
	isDaylight = getDaylight();
	if (!isDaylight)
		doorlock = false;
	if ((isDaylight) && !isDoorUp && !doorlock)
		doorMove(UP);
	if ((interiorTemperature > TEMP_THRESHHOLD) && !isFanOn)
		fanOn();
	else if ((interiorTemperature < TEMP_THRESHHOLD) && isFanOn)
		fanOff();
}

void printDebug() {
	Serial.println("Interior=" + (String)interiorTemperature);
	Serial.println("Exterior=" + (String)exteriorTemperature);
	//Serial.println("Door direction=" + trueFalse(doorDirection));
	Serial.println("Light Value=" + (String)light);
	//Serial.println("Daylight=" + trueFalse(isDaylight));
	Serial.println("doorIsUp=" + trueFalse(isDoorUp));
	//Serial.println("doorLock=" + trueFalse(doorlock));
	displayLCD(interiorTemperature, exteriorTemperature);
	Serial.println("");
}

int displayLCD(int intTemp, int extTemp) {
	lcd.setCursor(0, 0);
	lcd.print("Int:   ");
	lcd.print((char)223);
	lcd.print("    Ext:   ");
	lcd.print((char)223);
	lcd.setCursor(LCD_INT_COLUMN - 3, 0);
	lcd.print("   ");
	lcd.setCursor(LCD_EXT_COLUMN - 3, 3);
	lcd.print("   ");
	lcd.setCursor(0, 2);
	if (isDoorUp)
		lcd.print("Door is up  ");
	else
		lcd.print("Door is down");
	lcd.setCursor(0, 3);
	if (isFanOn)
		lcd.print("Fan is on ");
	else
		lcd.print("Fan is off");
	lcd.setCursor(getTempLCDPosition(intTemp, LCD_INT_COLUMN), 0);
	lcd.print((String)intTemp);
	lcd.setCursor(getTempLCDPosition(extTemp, LCD_EXT_COLUMN), 0);
	lcd.print((String)extTemp);
}

int getTempLCDPosition(int temp, int offset) {
	String strTemp;
	strTemp = (String)temp;

	return offset - strTemp.length();
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

//void getDoorButton() {
//
//	reading = digitalRead(DOOR_BUTTON_PIN);
//	if (reading == HIGH)	
//	if (reading != lastButtonState) {
//		// reset the debouncing timer
//		lastDebounceTime = millis();
//	}
//
//	//filter out any noise by setting a time buffer
//	if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
//		//if the button has been pressed, lets toggle the LED from "off to on" or "on to off"
//		if (reading != buttonState) {
//			buttonState = reading;
//
//			// only toggle the LED if the new button state is HIGH
//			if (reading == HIGH) {
//				//	backlightOn();
//				//	delay(3000);
//				//	backlightOff();
//			}
//		}
//	}
//}

//void getBacklightButton() {
//	// code from https://www.arduino.cc/en/Tutorial/Debounce
//	int reading = digitalRead(LCD_BACKLIGHT_BUTTON_PIN);
//	if (reading != lastButtonState) {
//		lastDebounceTime = millis();
//	}
//	if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
//		if (reading != buttonState) {
//			buttonState = reading;
//			if (buttonState == HIGH) { // our action goes here
//				Serial.println("Backlight Button Pressed");
//			}
//		}
//	}
//	lastButtonState = reading;
//}

void doorStop() {
	//int i;if (doorDirection)
	//	i = 160;
	//else
	//	i = 20;
	//myServo.write(i);
	//digitalWrite(DOOR_UP_PIN, HIGH);
	//digitalWrite(DOOR_DN_PIN, HIGH);
	Serial.println("Door Stop");
	printDebug();

	doorDirection = !doorDirection;
}

void doorMove() {
	//digitalWrite(DOOR_UP_PIN, doorDirection);
	//digitalWrite(DOOR_DN_PIN, !doorDirection);
	isDoorUp = doorDirection;
	doorlock = (!isDoorUp && isDaylight);
	Serial.println("Door Moving: " + (String)((doorDirection) ? "Up" : "Down"));

}

void doorMove(boolean direction) {
	doorDirection = direction;
	doorMove();
	delay(500);
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
	light = average;
	return (average >= LIGHT_THRESHHOLD);
}

void fanOn() {
	digitalWrite(FAN_PIN, false);
	isFanOn = true;
	printDebug();
	Serial.println("Fan On");
}

void fanOff() {
	digitalWrite(FAN_PIN, true);
	isFanOn = false;
	printDebug();
	Serial.println("Fan Off");
}

void backlightOn() {
	digitalWrite(LCD_BACKLIGHT_PIN, true);
	isFanOn = true;
	printDebug();
	Serial.println("Backlight On");
}

void backlightOff() {
	digitalWrite(LCD_BACKLIGHT_PIN, true);
	isFanOn = false;
	printDebug();
	Serial.println("Backlight Off");
}

String trueFalse(boolean i) {
	return (i == 0) ? "False" : "True";
}