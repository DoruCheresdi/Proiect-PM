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

int hasChosenConfigMode = 0;
DateTime startConfigDate;
int isStartPhase = 0; // if currently configuring start phase
int startHour = 8;
int endHour = 20;
int currentConfigHour = 8;
int timer = 0;
#define SECONDS_FOR_CHOOSING 10

#define STATE_RUNNING_NORMAL 0
#define STATE_WARNING_DISTANCE_TOO_SMALL 1
#define STATE_WARNING_NOT_AT_DESK 2
#define STATE_CONFIG_CHOOSE_1 3
#define STATE_CONFIG_CHOOSE_2 4
#define STATE_CONFIG_POTENTIOMETER 5
#define STATE_CONFIG_DISTANCE 6
#define STATE_CONFIG_CHOOSE_POT 7
#define STATE_CONFIG_CHOOSE_DIST 8
int currentState = 0;

#define MODE_DEV 0
#define MODE_PROD 1
int runningMode = MODE_DEV;

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
      currentState = STATE_WARNING_DISTANCE_TOO_SMALL;
    }
  } else {
    if (hasWarnedTooClose) {
      currentState = STATE_RUNNING_NORMAL;
      hasWarnedTooClose = 0;
      Serial.println("Thanks for reajusting your distance");
    }
  }
}

void checkAtDesk() {
  // checking if at desk:
  int deskMinDistance = 60;
  if (notAtDesk) {
    if (averageDistance < deskMinDistance) {
      currentState = STATE_RUNNING_NORMAL;
      notAtDesk = 0;
      Serial.println("Worker came back to desk");
      endBreak = rtc.now();

      // calculate break time:
      TimeSpan breakTime = endBreak - startBreak;
      totalBreakTime = totalBreakTime + breakTime;
      // sprintf(buffer, "Break time in seconds: %d\n", breakTime.totalseconds());  
      // Serial.print(buffer);
      Serial.println("Break time in seconds:");
      Serial.println(breakTime.totalseconds());
    }
  } else {
    if (averageDistance > deskMinDistance) {
      currentState = STATE_WARNING_NOT_AT_DESK;
      notAtDesk = 1;
      numberWorkBreaks++;
      Serial.println("Worker left desk");
      startBreak = rtc.now();
    }
  }
}

ISR(TIMER0_COMPA_vect){    //This  is the interrupt request
  timer++;
}

void startTimer() {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR0A=(1<<WGM01);    //Set the CTC mode   
  OCR0A=0xF9; //Value for ORC0A for 1ms 
  TCCR0B |= (1<<CS01);    //Set the prescale 1/64 clock
  TCCR0B |= (1<<CS00);
  
  timer = 0;
  TIMSK0 |= (1<<OCIE0A);   //Set  the interrupt request
  interrupts();
  sei(); //Enable interrupt
  
}

void manageState() {
  
    // // Print a message on both lines of the LCD.
  measureDistance();

  lcd.clear();         
  switch(currentState) {
    case STATE_RUNNING_NORMAL: {
      if (!digitalRead(BUTTON1_PIN)) {
        currentState = STATE_CONFIG_CHOOSE_1;
        isStartPhase = 1;
        startConfigDate = rtc.now();
        hasChosenConfigMode = 0;
        startTimer();
      }
      if (!digitalRead(BUTTON3_PIN)) {
        // if B3 is pressed, show config:
        Serial.println("Start:");
        Serial.println(startHour);
        Serial.println("End:");
        Serial.println(endHour);
      }

      checkDistanceTooSmall();
      checkAtDesk();
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d cm", averageDistance);
      lcd.print(buffer);


      break;
    }
    case STATE_WARNING_DISTANCE_TOO_SMALL: {
      checkDistanceTooSmall();
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d cm", averageDistance);
      lcd.print(buffer);
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      lcd.print("min dist 10cm!!");

      break;
    }
    case STATE_WARNING_NOT_AT_DESK: {
      checkAtDesk();
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d cm", averageDistance);
      lcd.print(buffer);
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      lcd.print("Worker in break");
      
      break;
    }
    case STATE_CONFIG_CHOOSE_1: {
      if (!digitalRead(BUTTON3_PIN)) {
        currentState = STATE_CONFIG_CHOOSE_POT;
        hasChosenConfigMode = 1;
      }
      if (timer >= 1000 * SECONDS_FOR_CHOOSING) {
        // 10 seconds
        currentState = STATE_CONFIG_CHOOSE_DIST;
        hasChosenConfigMode = 1;
      }
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Ch strt date b3");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      TimeSpan timeSinceStartTimer = rtc.now() - startConfigDate;
      snprintf(buffer, BUFFER_SIZE, "%d s", SECONDS_FOR_CHOOSING - timeSinceStartTimer.totalseconds());
      lcd.print(buffer);
      
      break;
    }
    case STATE_CONFIG_CHOOSE_POT: {
      if (!digitalRead(BUTTON1_PIN)) {
        hasChosenConfigMode = 0;
        if (isStartPhase) {
          currentState = STATE_CONFIG_CHOOSE_2;
          startHour = currentConfigHour;
        } else {
          currentState = STATE_RUNNING_NORMAL;
          endHour = currentConfigHour;
        }
        isStartPhase = 0;
        startConfigDate = rtc.now();
        startTimer();
      }
      int analogValue = analogRead(A1);
      currentConfigHour = map(analogValue, 0, 1018, 8, 20);
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Press B1 pot");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d h", currentConfigHour);
      lcd.print(buffer);
      
      break;
    }
    case STATE_CONFIG_CHOOSE_DIST: {
      if (!digitalRead(BUTTON1_PIN)) {
        hasChosenConfigMode = 0;
        if (isStartPhase) {
          currentState = STATE_CONFIG_CHOOSE_2;
          startHour = currentConfigHour;
        } else {
          currentState = STATE_RUNNING_NORMAL;
          endHour = currentConfigHour;
        }
        isStartPhase = 0;
        startConfigDate = rtc.now();
        startTimer();
      }
      // use distance for config:
      currentConfigHour = map(averageDistance, 12, 30, 8, 20);
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Press B1 dist");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d h", currentConfigHour);
      lcd.print(buffer);
      
      break;
    }
    case STATE_CONFIG_CHOOSE_2: {
      if (!digitalRead(BUTTON3_PIN)) {
        currentState = STATE_CONFIG_CHOOSE_POT;
        hasChosenConfigMode = 1;
      }
      if (timer >= 1000 * SECONDS_FOR_CHOOSING) {
        // 10 seconds
        currentState = STATE_CONFIG_CHOOSE_DIST;
        hasChosenConfigMode = 1;
      }
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Ch strt end b3");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      TimeSpan timeSinceStartTimer = rtc.now() - startConfigDate;
      snprintf(buffer, BUFFER_SIZE, "%d s", SECONDS_FOR_CHOOSING - timeSinceStartTimer.totalseconds());
      lcd.print(buffer);
      
      break;
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
  Serial.println("tooCloseWarnings:");
  Serial.println(tooCloseWarnings);
  Serial.println("number breaks:");
  Serial.println(numberWorkBreaks);
  Serial.println("Total break time in seconds:");
  Serial.println(totalBreakTime.totalseconds());
  // check for break:
  TimeSpan timeSinceBreak = now - endBreak;
  if (timeSinceBreak.totalseconds() > MAX_SECONDS_SINCE_BREAK && !notAtDesk) {
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
  
  delay(50);
  if (rtc.alarmFired(1)) {
    sendStatisticsToPC();
  }

}
