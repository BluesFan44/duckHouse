// Includes
#include <Arduino.h>
#include <TimerOne.h>
#include <Multiplexer.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// Pin Assignments
#define DOOR_DN_PIN 12
#define DOOR_UP_PIN 13
#define DOOR_BUTTON_PIN 9
#define PHOTO_PIN A1
#define THERMISTOR_INTERIOR_PIN A2
#define THERMISTOR_EXTERIOR_PIN A3
#define FAN_PIN 11

// Button Value Assignments
#define DOOR_UP_BUTTON 4
#define DOOR_DN_BUTTON 3
#define BACKLIGHT_BUTTON 2
#define FAN_BUTTON 1
#define INTERIOR_LIGHT_BUTTON 5

//// Constants
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
#define DEBUG_TIMER_MICROSECONDS 10000000
#define BYTES_VAL_T unsigned int
 //LCD Column -- must be 12 or 13
#define LCD_INT_COLUMN 7
#define LCD_EXT_COLUMN 19
#define I2C_ADDR    0x27  // Define I2C Address where the PCF8574A is
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
#define BACKLIGHT_PIN 3
#define BACKLIGHT_ON_TIME 2

//// Global Variables
boolean doorDirection,
isFanOn = false,
isBacklightOn = false,
isDoorUp = false,
isDaylight = true,
doorLock = false,
fanLock=false,
timerInterrupt=false;

int reading = LOW,
buttonPressed,
oldPinValues = 0,
lastButtonState = LOW,
buttonState,
samples[NUMSAMPLES],
interiorTemperature=0,
exteriorTemperature=0,
backlightCounter,
debugCounter=0;

float steinhart,
resistance,
average,
temperature,
light;
//Servo myServo;
long lastDebounceTime = 0;
TimerOne debugTimer; 
LiquidCrystal_I2C  lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);
Multiplexer multi(11, 11, 12, 13);

void setup() {
	Serial.begin(9600);
	debugTimer.initialize(DEBUG_TIMER_MICROSECONDS);
	//debugTimer.disablePwm(9);
	//debugTimer.disablePwm(10);
	debugTimer.attachInterrupt(onTimer);

	// set up the LCD
	lcd.begin(20, 4);
	lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
	// start the Thermister
	analogReference(EXTERNAL); 
	backlightOn();

	fanOff();
}

void loop() {
	if (timerInterrupt) {
		runTimed();
		timerInterrupt = false;
	}
	buttonPressed = multi.pressedButton();
	if (buttonPressed != 0) {
		backlightOn();
		processButton(buttonPressed);
	}
	isDaylight = getDaylight();
	if (!isDaylight) {
		doorLock = false;
		fanLock = false;
	}
	if ((isDaylight) && !isDoorUp && !doorLock)
		doorMove(UP);
	if ((interiorTemperature > TEMP_THRESHHOLD) && !isFanOn && !fanLock)
		fanOn();
	else if ((interiorTemperature < TEMP_THRESHHOLD) && isFanOn && !fanLock)
		fanOff();
}

void onTimer() { // just set the flag for the next loop() iteration
	timerInterrupt = true;
}

void processButton(int button) {
	switch (button) {
		case DOOR_UP_BUTTON:
			Serial.println("Door Up Button");
			isDoorUp = true;
			doorLock = false;
			break;
		case DOOR_DN_BUTTON:
			Serial.println("Door Down Button");
			isDoorUp = false;
			doorLock = true;
			break;
		case BACKLIGHT_BUTTON:
			Serial.println("Backlight Button");
			if (isBacklightOn)
				backlightOff();
			else
				backlightOn();
			break;
		case FAN_BUTTON:
			Serial.println("Fan Button");
			if (isFanOn)
				fanOff();
			else
				fanOn();
			fanLock = true;
			break;
		default: 
		  // I got nothing.  Carry on.
		break;
	  }
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

void runTimed() {
	interiorTemperature = readTemperature(THERMISTOR_INTERIOR_PIN);
	exteriorTemperature = readTemperature(THERMISTOR_EXTERIOR_PIN);
	displayLCD(interiorTemperature, exteriorTemperature);
	debugPrint();
	//Serial.println("");
	if (isBacklightOn && backlightCounter > BACKLIGHT_ON_TIME) {
		backlightOff();
		backlightCounter = 0;
	}
	else if (isBacklightOn)
		backlightCounter++;
}

void debugPrint() {
	Serial.println("Interior=" + (String)interiorTemperature);
	Serial.println("Exterior=" + (String)exteriorTemperature);
	//Serial.println("Door direction=" + trueFalse(doorDirection));
	Serial.println("Light Value=" + (String)light);
	//Serial.println("Daylight=" + trueFalse(isDaylight));
	//Serial.println("doorIsUp=" + trueFalse(isDoorUp));
	Serial.println("isBacklightOn=" + trueFalse(isBacklightOn));
	Serial.println("Backlight Counter=" + (String)backlightCounter);
	Serial.println("Debug Counter=" + (String)debugCounter++);
	Serial.println(" ");
}

void backlightOn() {
	Serial.println("Backlight On");
	lcd.setBacklight(HIGH);
	isBacklightOn = true;
	backlightCounter = 0;
}

void backlightOff() {
	if (isBacklightOn) {
		Serial.println("Backlight Off");
		lcd.setBacklight(LOW);
		isBacklightOn = false;
		backlightCounter = 0;
	}
}

void doorStop() {
	//digitalWrite(DOOR_UP_PIN, HIGH);
	//digitalWrite(DOOR_DN_PIN, HIGH);
	Serial.println("Door Stop");
	onTimer();

	doorDirection = !doorDirection;
}

void doorMove() {
	//digitalWrite(DOOR_UP_PIN, doorDirection);
	//digitalWrite(DOOR_DN_PIN, !doorDirection);
	isDoorUp = doorDirection;
	doorLock = (!isDoorUp && isDaylight);
	Serial.println("Door Moving: " + (String)((doorDirection) ? "Up" : "Down"));
}

void doorMove(boolean direction) {
	doorDirection = direction;
	doorMove();
	delay(500);
	doorStop();
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
	//digitalWrite(FAN_PIN, false);
	isFanOn = true;
	fanLock = true;
	onTimer();
	Serial.println("Fan On");
}

void fanOff() {
	//digitalWrite(FAN_PIN, true);
	isFanOn = false;
	fanLock = true;
	onTimer();
	Serial.println("Fan Off");
}

String trueFalse(boolean i) {
	return (i == 0) ? "False" : "True";
}