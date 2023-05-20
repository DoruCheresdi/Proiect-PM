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
#define BUTTON1_PIN 3
#define BUTTON2_PIN 2
#define BUTTON3_PIN 4
#define BUZZERPIN 5
// #define BUTTON4_PIN 4

#define NUMBER_MEASUREMENTS 5

#define MAX_SECONDS_SINCE_BREAK 15
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

#define STATE_RUNNING_NORMAL 0
#define STATE_WARNING_DISTANCE_TOO_SMALL 1
#define STATE_WARNING_NOT_AT_DESK 2
#define STATE_CONFIG_POTENTIOMETER 5
#define STATE_CONFIG_DISTANCE 6
int currentState = 0;

#define START_HOUR 8
#define FINISH_HOUR 20

#define BUFFER_SIZE 36
char buffer[BUFFER_SIZE];

void measureDistance() {
	long duration, distance;
	digitalWrite(TRIGPIN, LOW);
	delayMicroseconds(2);
	digitalWrite(TRIGPIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGPIN, LOW);
	duration = pulseIn(ECHOPIN, HIGH);
	distance = (duration/2) / 29.1;

	
		// Serial.print(distance);
		// Serial.println(" cm");
	if (distance >= 700 || distance <= 0) {
		// Serial.println("Out of range");
	}
	else {
    averageDistance = averageDistance * (NUMBER_MEASUREMENTS - 1) + distance;
    averageDistance = averageDistance / NUMBER_MEASUREMENTS;
		// Serial.print(averageDistance);
		// Serial.println(" cm");
	}
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

void checkDistanceTooSmall() {
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
}

void manageState() {
  
    // // Print a message on both lines of the LCD.
  measureDistance();

  lcd.clear();         
  switch(currentState) {
    case STATE_RUNNING_NORMAL: {
      checkDistanceTooSmall();
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d cm", averageDistance);
      lcd.print(buffer);

      if (!digitalRead(BUTTON1_PIN)) {
        currentState = 
      }

      break;
    }
    case STATE_WARNING_NOT_AT_DESK: {
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d cm", averageDistance);
      lcd.print(buffer);
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      lcd.print("min dist 10cm!!");

      break;
    }
    case STATE_WARNING_NOT_AT_DESK: {
      
    }
    default:
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("ERROR");

  }
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
  endBreak = nowNow;
}

void sendStatisticsToPC() {
  // periodic check
  Serial.println("\n");
  
  DateTime now = rtc.now(); // Get the current time
  char buff[] = "Alarm triggered at hh:mm:ss DDD, DD MMM YYYY";
  Serial.println(now.toString(buff));
  // sprintf(buffer, "Too close warnings: %d\n", tooCloseWarnings);  
  // Serial.print(buffer);
  // sprintf(buffer, "number breaks: %d\n", numberWorkBreaks);  
  // Serial.print(buffer);
  // sprintf(buffer, "Total break time in seconds: %d\n", (int)totalBreakTime.totalseconds());  
  // Serial.print(buffer);
  Serial.println("tooCloseWarnings:");
  Serial.println(tooCloseWarnings);
  Serial.println("number breaks:");
  Serial.println(numberWorkBreaks);
  Serial.println("Total break time in seconds:");
  Serial.println(totalBreakTime.totalseconds());
  // check for break:
  TimeSpan timeSinceBreak = now - endBreak;
  if (timeSinceBreak.totalseconds() > MAX_SECONDS_SINCE_BREAK && !notAtDesk) {
    // sprintf(buffer, "WARNING: Take a break: %d s since last break\n", (int)timeSinceBreak.totalseconds());  
    // Serial.println(buffer);
    Serial.println("WARNING: Take a break:");  
    Serial.println(timeSinceBreak.totalseconds());
  }
  Serial.println("\n");
  // Disable and clear alarm
  rtc.disableAlarm(1);
  rtc.clearAlarm(1);

  // Perhaps reset to new time if required
  rtc.setAlarm1(now + TimeSpan(0, 0, 0, NUMBER_SECONDS_ALARM1), DS3231_A1_Second);
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

  manageState();
  // getAndPrintTime();

  // checking if distance is too small:

  
  // checking if at desk:
  int deskMinDistance = 60;
  if (notAtDesk) {
    if (averageDistance < deskMinDistance) {
      notAtDesk = 0;
      Serial.println("Worker came back to desk");
      endBreak = rtc.now();

      TimeSpan breakTime = endBreak - startBreak;
      totalBreakTime = totalBreakTime + breakTime;
      // sprintf(buffer, "Break time in seconds: %d\n", breakTime.totalseconds());  
      // Serial.print(buffer);
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

  int analogValue = analogRead(A1);
  int hour = map(analogValue, 0, 1018, 8, 20);
  Serial.println(hour);
  
  delay(50);
  if (rtc.alarmFired(1)) {
    sendStatisticsToPC();
  }

}
