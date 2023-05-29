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

#define MAX_SECONDS_SINCE_BREAK 10
#define MAX_SECONDS_NOT_WORKING 10
// #define NUMBER_SECONDS_ALARM2 20
// defines variables
#define LIGHT_SENSOR_PIN A0 // select the input pin for the potentiometerensor
int averageDistance = 20; // in cm

int hasWarnedTooClose = 0;
int tooCloseWarnings = 0;

int lightAverageValue = 0; // variable to store the value coming from the s
int lightWarnings = 0;

int notAtDesk = 0;
int numberWorkBreaks = 0;
int snoozes = 0;
DateTime startBreak;
DateTime endBreak;
TimeSpan totalBreakTime;
int clearTimer = 0;
int hasMissedTooMuch = 0;

String pass = "pass";

int hasChosenConfigMode = 0;
DateTime startConfigDate;
int isStartPhase = 0; // if currently configuring start phase
int startHour = 8;
int endHour = 20;
int isDuringWorkSchedule = 0;
int currentConfigHour = 8;
#define SECONDS_FOR_CHOOSING 10

#define STATE_RUNNING_NORMAL 0
#define STATE_WARNING_DISTANCE_TOO_SMALL 1
#define STATE_WARNING_NOT_AT_DESK 2
#define STATE_CONFIG_CHOOSE_1 3
#define STATE_CONFIG_CHOOSE_2 4
#define STATE_CONFIG_DISTANCE 5
#define STATE_CONFIG_CHOOSE_POT 6
#define STATE_CONFIG_CHOOSE_DIST 7
#define STATE_WARNING_TURN_ON_LIGHT_MODE 8
#define STATE_WARNING_TAKE_A_BREAK 9
#define STATE_WAIT_PASS 10
#define STATE_WORKER_FAILED 11
int currentState = 0;

#define MODE_DEV 0
#define MODE_PROD 1
int runningMode = MODE_DEV;

#define START_HOUR 0
#define FINISH_HOUR 60

#define BUFFER_SIZE 60
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
  delay (20); // wait mode for 4 seconds
  digitalWrite (BUZZERPIN, LOW); // Buzzer is switched off
  delay (80); // Wait mode for another two seconds in which the LED is then turned off
}

void getAndPrintTime() {
  char t[32];
  DateTime now = rtc.now();
  sprintf(t, "%02d:%02d:%02d %02d/%02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());  
  Serial.print(F("Date/Time: "));
  Serial.println(t);
  delay(1000);
}

// void setAlarm2() {
//   rtc.disableAlarm(2);
//   rtc.clearAlarm(2);
//   DateTime now = rtc.now(); // Get current time
//   rtc.setAlarm2(now + TimeSpan(0, 0, 0, NUMBER_SECONDS_ALARM2), DS3231_A2_Minute); // In 10 seconds time
// }

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
      if (currentState == STATE_WARNING_NOT_AT_DESK) {
        currentState = STATE_RUNNING_NORMAL;
      }
      notAtDesk = 0;
      Serial.println("Worker came back to desk");
      endBreak = rtc.now();

      // calculate break time:
      TimeSpan breakTime = endBreak - startBreak;
      totalBreakTime = totalBreakTime + breakTime;
      snprintf(buffer, BUFFER_SIZE, "Break time in seconds: %d\n", breakTime.totalseconds());  
      Serial.print(buffer);
      // Serial.println("Break time in seconds:");
      // Serial.println(breakTime.totalseconds());
    }
  } else {
    if (averageDistance > deskMinDistance) {
      if (currentState == STATE_RUNNING_NORMAL || currentState == STATE_WARNING_TAKE_A_BREAK) {
        currentState = STATE_WARNING_NOT_AT_DESK;
      }
      notAtDesk = 1;
      numberWorkBreaks++;
      Serial.println("Worker left desk");
      startBreak = rtc.now();
    }
  }
}

void checkLight() {
  int lightValue = analogRead(A0);
  lightAverageValue = lightAverageValue * NUMBER_MEASUREMENTS + lightValue;
  lightAverageValue = lightAverageValue / (NUMBER_MEASUREMENTS + 1);

  if (lightAverageValue > 30) {
    if (currentState == STATE_RUNNING_NORMAL) {
      currentState = STATE_WARNING_TURN_ON_LIGHT_MODE;
      lightWarnings++;
    }
  }
  if (currentState == STATE_WARNING_TURN_ON_LIGHT_MODE) {
    if (lightAverageValue < 30) {
      currentState = STATE_RUNNING_NORMAL;
    }
  }
}

void manageState() {
  
    // // Print a message on both lines of the LCD.
  measureDistance();

  clearTimer++;
  if (clearTimer > 5) {
    clearTimer = 0;
    lcd.clear();       
  }  
  switch(currentState) {
    case STATE_RUNNING_NORMAL: {
      if (!digitalRead(BUTTON1_PIN)) {
        currentState = STATE_WAIT_PASS;
        break;
      }
      if (!digitalRead(BUTTON3_PIN)) {
        // if B3 is pressed, show config:
        DateTime now = rtc.now();
        char buff[] = "Showing stats triggered at hh:mm:ss DDD, DD MMM YYYY";
        Serial.println(now.toString(buff));
        Serial.println("Start:");
        Serial.println(startHour);
        Serial.println("End:");
        Serial.println(endHour);
      }
      
      // check for break:
      TimeSpan timeSinceBreak = rtc.now() - endBreak;
      if (isDuringWorkSchedule && timeSinceBreak.totalseconds() > MAX_SECONDS_SINCE_BREAK && !notAtDesk) {
        currentState = STATE_WARNING_TAKE_A_BREAK;
        break;
      }
      // Too much break:
      int totalMissedTime = totalBreakTime.totalseconds();
      if (notAtDesk) {
        totalMissedTime += timeSinceBreak.totalseconds();
      }
      if (isDuringWorkSchedule && totalMissedTime > (endHour - startHour) * 3 / 4) {
        currentState = STATE_WORKER_FAILED;
        break;
      }

      checkLight();
      checkDistanceTooSmall();
      checkAtDesk();
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d cm %hhu s", averageDistance, rtc.now().second());
      lcd.print(buffer);

      if (isDuringWorkSchedule) {
        lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
        lcd.print("Working");
      }


      break;
    }
    case STATE_WARNING_DISTANCE_TOO_SMALL: {
      checkDistanceTooSmall();
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d cm", averageDistance);
      lcd.print(buffer);
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      lcd.print("min dist 10cm!!");

      alarmBuzzer();

      break;
    }
    case STATE_WARNING_NOT_AT_DESK: {
      checkAtDesk();
      // Too much break:
      TimeSpan timeSinceBreak = rtc.now() - endBreak;
      int totalMissedTime = totalBreakTime.totalseconds();
      if (notAtDesk) {
        totalMissedTime += timeSinceBreak.totalseconds();
      }
      if (isDuringWorkSchedule && totalMissedTime > (endHour - startHour) * 3 / 4) {
        currentState = STATE_WORKER_FAILED;
        break;
      }
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d cm", averageDistance);
      lcd.print(buffer);
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      lcd.print("Worker in break");
      
      break;
    }
    case STATE_WARNING_TURN_ON_LIGHT_MODE: {
      checkLight();
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "NO LIGHT %d", lightAverageValue);
      lcd.print(buffer);
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      lcd.print("TURN ON DARK MODE");

      Serial.println("TURN ON DARK MODE ON PC!");
      
      break;
    }
    case STATE_WARNING_TAKE_A_BREAK: {
      checkAtDesk();
      if (!digitalRead(BUTTON2_PIN)) {
        // snooze button:
        currentState = STATE_RUNNING_NORMAL;
        endBreak = rtc.now();
        snoozes++;
        break;
      }
      TimeSpan timeSinceBreak = rtc.now() - endBreak;
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("WRN:Take a brk");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "SNZ B2 %d %d", averageDistance, timeSinceBreak.totalseconds());
      lcd.print(buffer);

      alarmBuzzer();

      break;
    }


    // configuration states:
    case STATE_WAIT_PASS: {
      if (!digitalRead(BUTTON2_PIN)) {
        currentState = STATE_CONFIG_CHOOSE_1;
        isStartPhase = 1;
        startConfigDate = rtc.now();
        hasChosenConfigMode = 0;
        resetStats();
        break;
      }

      if(Serial.available()){
        String newPass = Serial.readStringUntil('\n');
        if (newPass.equals(pass)) {
          currentState = STATE_CONFIG_CHOOSE_1;
          isStartPhase = 1;
          startConfigDate = rtc.now();
          hasChosenConfigMode = 0;
          resetStats();

          break;
        }
      } 

      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Waiting for pass");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      lcd.print("B2");
      
      break;
    }
    case STATE_CONFIG_CHOOSE_1: {
      if (!digitalRead(BUTTON3_PIN)) {
        currentState = STATE_CONFIG_CHOOSE_POT;
        hasChosenConfigMode = 1;
      }
      TimeSpan timeSinceStartTimer = rtc.now() - startConfigDate;
      if (timeSinceStartTimer.totalseconds() > SECONDS_FOR_CHOOSING) {
        // 10 seconds
        currentState = STATE_CONFIG_CHOOSE_DIST;
        hasChosenConfigMode = 1;
      }
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Ch strt date b3");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d s", SECONDS_FOR_CHOOSING - timeSinceStartTimer.totalseconds());
      lcd.print(buffer);
      
      break;
    }
    case STATE_CONFIG_CHOOSE_POT: {
      if (!digitalRead(BUTTON2_PIN)) {
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
      }
      int analogValue = analogRead(A1);
      currentConfigHour = map(analogValue, 0, 1000, START_HOUR, FINISH_HOUR);
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Press B2 pot");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d h", currentConfigHour);
      lcd.print(buffer);
      
      break;
    }
    case STATE_CONFIG_CHOOSE_DIST: {
      if (!digitalRead(BUTTON2_PIN)) {
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
      }
      // use distance for config:
      currentConfigHour = map(averageDistance, 8, 50, START_HOUR, FINISH_HOUR);
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Press B2 dist");
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
      TimeSpan timeSinceStartTimer = rtc.now() - startConfigDate;
      if (timeSinceStartTimer.totalseconds() > SECONDS_FOR_CHOOSING) {
        // 10 seconds
        currentState = STATE_CONFIG_CHOOSE_DIST;
        hasChosenConfigMode = 1;
      }
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("Ch end date b3");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      snprintf(buffer, BUFFER_SIZE, "%d s", SECONDS_FOR_CHOOSING - timeSinceStartTimer.totalseconds());
      lcd.print(buffer);
      
      break;
    }
    case STATE_WORKER_FAILED: {
      hasMissedTooMuch = 1;
      if (!isDuringWorkSchedule) {
        // return to normal:
        currentState = STATE_RUNNING_NORMAL;
      }
      lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
      lcd.print("U missed");
      lcd.setCursor(0,1);   //Set cursor to character 2 on line 0
      lcd.print("too much!!");
      
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

  // setAlarm2();

  
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);

  // initialize break time:
  DateTime nowNow = rtc.now();
  totalBreakTime = nowNow - nowNow;
  endBreak = nowNow;
}

// void resetAlarm2() {
//   // periodic check
//   Serial.println("\n");
  
//   DateTime now = rtc.now(); // Get the current time
//   char buff[] = "Alarm2 triggered at hh:mm:ss DDD, DD MMM YYYY";
//   Serial.println(now.toString(buff));
//   Serial.println("\n");
//   // Disable and clear alarm
//   setAlarm2();
// }

void resetStats() {
  Serial.println("reset");
  tooCloseWarnings = 0;
  lightWarnings = 0;
  numberWorkBreaks = 0;
  DateTime now = rtc.now();
  snoozes = 0;
  hasMissedTooMuch = 0;
  endBreak = now;
  totalBreakTime = now - now;
}

void sendStatsToPC() {
  Serial.println("\n");
  
  DateTime now = rtc.now(); // Get the current time
  Serial.println("\n/*********************/");
  char buff[] = "End work triggered at hh:mm:ss DDD, DD MMM YYYY";
  Serial.println(now.toString(buff));
  snprintf(buffer, BUFFER_SIZE, "tooCloseWarnings: %d", tooCloseWarnings);
  Serial.println(buffer);
  // Serial.println("tooCloseWarnings:");
  // Serial.println(tooCloseWarnings);
  snprintf(buffer, BUFFER_SIZE, "lightWarnings: %d", lightWarnings);
  Serial.println(buffer);
  // Serial.println("lightWarnings:");
  // Serial.println(lightWarnings);
  snprintf(buffer, BUFFER_SIZE, "number breaks: %d", numberWorkBreaks);
  Serial.println(buffer);
  // Serial.println("number breaks:");
  // Serial.println(numberWorkBreaks);
  snprintf(buffer, BUFFER_SIZE, "Total break time in seconds: %d", totalBreakTime.totalseconds());
  Serial.println(buffer);
  snprintf(buffer, BUFFER_SIZE, "Total snoozes: %d", snoozes);
  Serial.println(buffer);
  // Serial.println("Total break time in seconds:");
  // Serial.println(totalBreakTime.totalseconds());
  if (hasMissedTooMuch) {
    Serial.println("!!!!Employee was absent too much!!!");
  }
  // check for break:
  TimeSpan timeSinceBreak = now - endBreak;
  snprintf(buffer, BUFFER_SIZE, "Time since break: %d", timeSinceBreak.totalseconds());
  Serial.println(buffer);
  // Serial.println("Time since break:");  
  // Serial.println(timeSinceBreak.totalseconds());
  Serial.println("/*********************/\n");
}

void manageWorkSchedule() {
  DateTime now = rtc.now();
  if (now.second() > startHour && now.second() < endHour) {
    if (isDuringWorkSchedule == 0) {
      isDuringWorkSchedule = 1;
      // reset stats:
      resetStats();
      Serial.println("\n\n/*********************/");
      char buff[] = "Start work triggered at hh:mm:ss DDD, DD MMM YYYY";
      Serial.println(now.toString(buff));
      Serial.println("/*********************/\n");
    }    
  } else {
    if (isDuringWorkSchedule == 1) {
      isDuringWorkSchedule = 0;
      // send schedule to pc
      sendStatsToPC();
    }
  }
}

void loop() {
  // unsigned int currentSeconds = myRTC.getSecond();

  manageState();
  // getAndPrintTime();
  
  delay(50);
  // if (rtc.alarmFired(2)) {
  //   resetAlarm2();
  // }

  manageWorkSchedule();
}
