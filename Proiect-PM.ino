#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <Wire.h>
#include <string.h>

// https://github.com/NorthernWidget/DS3231


// Init the DS3231 using the hardware interface
DS3231  myRTC;
LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display

// defines pins numbers
#define TRIGPIN 13
#define ECHOPIN 12
// defines variables
long duration;
int distance;

int sensorPin = A0; // select the input pin for the potentiometer
int sensorValue = 0; // variable to store the value coming from the sensor
int Buzzer = 11;

void setup() {
  // pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  // pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(9600);
  
  pinMode(Buzzer, OUTPUT); // Initialize output pin for the buzzer
	pinMode(trigPin, OUTPUT);
	pinMode(echoPin, INPUT);

  lcd.init();
  lcd.clear();         
  lcd.backlight();      // Make sure backlight is on

  Wire.begin();
}

void measure_distance() {
	long duration, distance;
	digitalWrite(TRIGPIN, LOW);
	delayMicroseconds(2);
	digitalWrite(TRIGPIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGPIN, LOW);
	duration = pulseIn(ECHOPIN, HIGH);
	distance = (duration/2) / 29.1;
	
	if (distance >= 200 || distance <= 0) {
		Serial.println("Out of range");
	}
	else {
		Serial.print(distance);
		Serial.println(" cm");
	}
}

void print_to_lcd(int value) {
  
    // // Print a message on both lines of the LCD.
    lcd.clear();         
    lcd.setCursor(2,0);   //Set cursor to character 2 on line 0
    // unsigned int theDate = myRTC.getSecond();
    char buffer [16];
    itoa (cm,buffer,10);
    lcd.print(buffer);
}

void loop() {
  measure_distance();
  delay(500);
  // // Clears the trigPin
  // digitalWrite(trigPin, LOW);
  // delayMicroseconds(2);
  // // Sets the trigPin on HIGH state for 10 micro seconds
  // digitalWrite(trigPin, HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigPin, LOW);
  // // Reads the echoPin, returns the sound wave travel time in microseconds
  // duration = pulseIn(echoPin, HIGH);
  // // Calculating the distance
  // distance = duration * 0.034 / 2;
  // // Prints the distance on the Serial Monitor
  // Serial.print("Distance: ");
  // Serial.println(distance);



  //  long duration, inches, cm;
  //  pinMode(pingPin, OUTPUT);
  //  digitalWrite(pingPin, LOW);
  //  delayMicroseconds(2);
  //  digitalWrite(pingPin, HIGH);
  //  delayMicroseconds(10);
  //  digitalWrite(pingPin, LOW);
  //  pinMode(echoPin, INPUT);
  //  duration = pulseIn(echoPin, HIGH);
  //  inches = microsecondsToInches(duration);
  //  cm = microsecondsToCentimeters(duration);
  //  if (cm < 3000) {

  //  }
  //  delay(1000);



  // sensorValue = analogRead(sensorPin);
   
  // lcd.clear();         
  // lcd.setCursor(2,0);   //Set cursor to character 2 on line 0
  // char buffer [16];
  // itoa (sensorValue,buffer,10);
  // lcd.print(buffer);
  // delay(400);


  digitalWrite (Buzzer, HIGH); // Buzzer is switched on
  delay (200); // wait mode for 4 seconds
  digitalWrite (Buzzer, LOW); // Buzzer is switched off
  delay (2000); // Wait mode for another two seconds in which the LED is then turned off

  
  // lcd.setCursor(2,1);   //Move cursor to character 2 on line 1
  // lcd.print("HI");

  // delay (1000);
}

long microsecondsToInches(long microseconds) {
   return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
   return microseconds / 29 / 2;
}