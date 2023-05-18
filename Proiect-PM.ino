#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Wire.h>
#include <string.h>

// https://github.com/NorthernWidget/DS3231


// Init the DS3231 using the hardware interface
// DS3231  myRTC;
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display

// defines pins numbers
#define TRIGPIN 13
#define ECHOPIN 12
#define BUZZERPIN 0
#define BUTTON1_PIN 3
#define BUTTON2_PIN 2
#define BUTTON3_PIN 4
// #define BUTTON4_PIN 4

#define NUMBER_MEASUREMENTS 5

#define NUMBER_SECONDS_ALARM1 20
// defines variables
int sensorPin = A0; // select the input pin for the potentiometer
int sensorValue = 0; // variable to store the value coming from the sensor
long averageDistance = 20; // in cm

int hasWarnedTooClose = 0;
int tooCloseWarnings = 0;

int notAtDesk = 0;
int numberWorkBreaks = 0;
DateTime startBreak;
DateTime endBreak;
TimeSpan totalBreakTime;

char buffer[17];

long microsecondsToInches(long microseconds) {
   return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
   return microseconds / 29 / 2;
}

void measureDistance() {
	long duration, distance;
	digitalWrite(TRIGPIN, LOW);
	delayMicroseconds(2);
	digitalWrite(TRIGPIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGPIN, LOW);
	duration = pulseIn(ECHOPIN, HIGH);
	distance = (duration/2) / 29.1;

	
	if (distance >= 400 || distance <= 0) {
		// Serial.println("Out of range");
	}
	else {
    averageDistance = averageDistance * (NUMBER_MEASUREMENTS - 1) + distance;
    averageDistance = averageDistance / NUMBER_MEASUREMENTS;
		// Serial.print(averageDistance);
		// Serial.println(" cm");
	}
  printToLCD(averageDistance);
}

void printToLCD(int value) {
  
    // // Print a message on both lines of the LCD.
    lcd.clear();         
    lcd.setCursor(2,0);   //Set cursor to character 2 on line 0
    itoa(value, buffer, 10);
    lcd.print(buffer);
}

void alarmBuzzer() {
  digitalWrite (BUZZERPIN, HIGH); // Buzzer is switched on
  delay (200); // wait mode for 4 seconds
  digitalWrite (BUZZERPIN, LOW); // Buzzer is switched off
  delay (2000); // Wait mode for another two seconds in which the LED is then turned off
}

void getAndPrintTime() {
  char t[32];
  DateTime now = rtc.now();
  sprintf(t, "%02d:%02d:%02d %02d/%02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());  
  Serial.print(F("Date/Time: "));
  Serial.println(t);
  delay(1000);
}

void setAlarm() {
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  DateTime now = rtc.now(); // Get current time
  rtc.setAlarm1(now + TimeSpan(0, 0, 0, NUMBER_SECONDS_ALARM1), DS3231_A1_Second); // In 10 seconds time
}

void setup() {
  // pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  // pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  
  pinMode(BUZZERPIN, OUTPUT); // Initialize output pin for the buzzer
	pinMode(TRIGPIN, OUTPUT);
	pinMode(ECHOPIN, INPUT);

  lcd.init();
  lcd.clear();         
  lcd.backlight();      // Make sure backlight is on

  setAlarm();

  
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);

  // initialize break time:
  DateTime nowNow = rtc.now();
  totalBreakTime = nowNow - nowNow;
}


void loop() {
  // unsigned int currentSeconds = myRTC.getSecond();

  if (!digitalRead(BUTTON1_PIN)) {
    Serial.println("Button 1 pressed");
  }
  if (!digitalRead(BUTTON2_PIN)) {
    Serial.println("Button 2 pressed");
  }
  if (!digitalRead(BUTTON3_PIN)) {
    Serial.println("Button 3 pressed");
  }

  // getAndPrintTime();
  measureDistance();
  if (averageDistance < 10) {
    if (!hasWarnedTooClose) {
      hasWarnedTooClose = 1;
      tooCloseWarnings++;
      Serial.println("WARNING: Too close, minimum distance is 10cm");
    }
  } else {
    if (hasWarnedTooClose) {
      hasWarnedTooClose = 0;
      Serial.println("Thanks for reajusting your distance");
    }
  }

  

  int deskMinDistance = 60;
  if (notAtDesk) {
    if (averageDistance < deskMinDistance) {
      notAtDesk = 0;
      Serial.println("Worker came back to desk");
      endBreak = rtc.now();

      TimeSpan breakTime = endBreak - startBreak;
      totalBreakTime = totalBreakTime + breakTime;
      Serial.println("Break time in seconds:");
      Serial.println(breakTime.totalseconds());
    }
  } else {
    if (averageDistance > deskMinDistance) {
      notAtDesk = 1;
      numberWorkBreaks++;
      Serial.println("Worker left desk");
      startBreak = rtc.now();
    }
  }

  delay(50);
  if (rtc.alarmFired(1)) {
    // The alarm has just fired
    
    DateTime now = rtc.now(); // Get the current time
    char buff[] = "Alarm triggered at hh:mm:ss DDD, DD MMM YYYY";
    Serial.println(now.toString(buff));
    Serial.println("tooCloseWarnings:");
    Serial.println(tooCloseWarnings);
    Serial.println("number breaks:");
    Serial.println(numberWorkBreaks);
    Serial.println("Total break time in seconds:");
    Serial.println(totalBreakTime.totalseconds());
    // Disable and clear alarm
    rtc.disableAlarm(1);
    rtc.clearAlarm(1);

    // Perhaps reset to new time if required
    rtc.setAlarm1(now + TimeSpan(0, 0, 0, NUMBER_SECONDS_ALARM1), DS3231_A1_Second);
  }
}
