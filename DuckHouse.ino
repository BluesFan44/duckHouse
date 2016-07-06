// which analog pin to connect
#define THERMISTOR_INTERIOR_PIN A0
#define THERMISTOR_EXTERIOR_PIN A1
// resistance at 25 degrees C
#define THERMISTORNOMINAL 42000
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
#define LCD_START_COLUMN 13
// Loop delay
#define LOOP_DELAY 10000

int samples[NUMSAMPLES];

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

void setup() {
	// set up the LCD
	lcd.begin(16, 2);
	lcd.print("Int Temp:"); 
	lcd.setCursor(LCD_START_COLUMN, 0);
	lcd.print((char)223);
	lcd.setCursor(0, 1);
	lcd.print("Ext Temp:");
	lcd.setCursor(LCD_START_COLUMN, 1);
	lcd.print((char)223);
	// start the Thermister
	Serial.begin(9600);
	analogReference(EXTERNAL);
}

void loop() {
	int interiorTemperature, exteriorTemperature;
	interiorTemperature = readTemperature(THERMISTOR_INTERIOR_PIN);
	exteriorTemperature = readTemperature(THERMISTOR_EXTERIOR_PIN);
	LCDTemperature(interiorTemperature, exteriorTemperature);
	Serial.println();
	delay(LOOP_DELAY);
}

float getTempFromResistance(float resistance) {
	float steinhart;
	steinhart = resistance / THERMISTORNOMINAL;     // (R/Ro)
	steinhart = log(steinhart);                  // ln(R/Ro)
	steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
	steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
	steinhart = 1.0 / steinhart;                 // Invert
	steinhart -= 273.15;                         // convert to C
	steinhart = steinhart * 9 / 5 + 32;			 // convert to F
	return steinhart;
}

int LCDTemperature(int intTemp, int extTemp) {
	lcd.setCursor(LCD_START_COLUMN - 3, 0);
	lcd.print("  ");
	lcd.setCursor(LCD_START_COLUMN - 3, 1);
	lcd.print("  ");
	lcd.setCursor(getTempLCDPosition(intTemp), 0);
	lcd.print((String)intTemp);
	lcd.setCursor(getTempLCDPosition(extTemp), 1);
	lcd.print((String)extTemp);
}

int readTemperature(uint8_t pin) {
	float resistance, average, temperature;
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

int getTempLCDPosition(int temp) {
	String strTemp;
	strTemp = (String)temp;

	return LCD_START_COLUMN-strTemp.length();
}