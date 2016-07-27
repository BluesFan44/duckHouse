
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
// Relay pins
// Photoresistor pin
// Button Pin
#define DEBOUNCE_DELAY 10
// Light level at which door goes up
#define LIGHT_THRESHHOLD 100
// Turn fan on at fahernheit temp
#define TEMP_THRESHHOLD 78
#define UP true
#define DOWN false
//#define SERVO_PIN 6
#define DEBUG_TIMER_MICROSECONDS 60000000

#define BYTES_VAL_T unsigned int

BYTES_VAL_T buttonPressed;
BYTES_VAL_T oldPinValues;