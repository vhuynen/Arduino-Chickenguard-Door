#include <DS3232RTC.h>   // https://github.com/PaulStoffregen/DS1307RTC
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
#include <LiquidCrystal.h>
#include <avr/sleep.h>

//###################### Vincent HUYNEN ########################/
//################## vincent.huynen@gmail.com ##################/
//######################### March 2018 #########################/
//######################## Version 1.0.0 #######################/

//##################### Start RTC Module #######################/
// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone myTZ(CEST, CET);

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, t, openingDateTime, closingDateTime;
//##################### End RTC Module #######################/

//##################### Start LCD screen #######################/
// Screen LCD
const int scrollingButton = 39;
const int backLight = 36;
const int powerLCD = 52;
const int contrastLCD = 53;
const int rs = 50, en = 48, d4 = 46, d5 = 44, d6 = 42, d7 = 40;
int scrolling = 0;
unsigned long timeoutLcd = 20000;
unsigned long lastActivateLcdMillis = 0;
LiquidCrystal LCD(rs, en, d4, d5, d6, d7); // We create screen LCD Object
//##################### End LCD screen #########################/

//##################### Start ChickenGuard ######################/
boolean modeAuto = false;
boolean error = false;
String  errorMsg = "";
// Log last time opening and closing
time_t lastOpening = 0, lastClosing = 0;
int openDoorButton = 23; // pin Button Open Door
int closeDoorButton = 30; // pin Button Close Door
int automaticModeButton = 26; // pin Button automatic mode activation
// LED Door Closed
int pinLedDoorClosed = 22; // pin Blink Led when Door is close
unsigned long previousMillis = 0; // will store last time LED was updated
const long interval = 500; // Interval blink Led Door Closed
boolean blinkLedDoorClosed = false;
unsigned long blinkLedDoorClosedPeriod = 0; // Use to perform blink process
long blinkLedDoorClosedDuration = 20000;// Time during Led blinks after closing process, adjust it if need it !
// LED Error
int pinErrorLed = 25; // pin Blink LED when Error have been thrown
unsigned long previousMillisError = 0; // will store last time LED was updated
const long intervalError = 300; // Interval blink Error LED
// Photoresistor
int photoResistor = A2; // pin Photo Resistor value
int daylightThreshold = 700; // Adjust Threshold DayLight
int nightLightThreshold = 500; // Adjust Threshold NightLight
int nightMonths[12][2]; // Array of opening hour hh.mm
int dayMonths[12][2]; // Array of closing hour hh.mm
int pinMicroSwitchTop = 28; // pin Micro Switch Top
int pinMicroSwitchBottom = 29; // pin Micro Switch Bottom
//##################### End ChickenGuard ########################/

//#################### Start Motor ##############################/
int pin1Motor = 32; // pin de command motor
int pin2Motor = 35; // pin de command motot
int pinPWMMotor = 5; // pin PWM motor
int pinPowerMotor = A0; // pin potentiometer for scale power motor
//#################### End Motor ##############################/

//#################### Start Power Saving ############################/
const uint8_t SQW_PIN(2);   // connect this pin to DS3231 INT/SQW pin
int pinWakeUp = 3; // Used to wake up the process
volatile boolean wakeUpManuallyWasCalled = false;
volatile boolean wakeUpByAlarmWasCalled = false;
boolean powerSavingMode = true; // Allow to desactivate Power Saving Mode
time_t lastAlarmTime; // Last triggering Alarm date
//#################### End Power Saving ##############################/

void setup() {

  // Uncomment this to Setup RTC time on first only, Be carrefuly you must set GMT Time
  //setTimeRTC(18, 3, 21, 20, 56, 0);   
  
  // Comment 
  Serial.begin(9600);
  
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.print("RTC has set the system time : ");printDateTime(now());

  // Setup Power Saving
  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);

  pinMode(SQW_PIN, INPUT_PULLUP);
  pinMode(pinWakeUp, INPUT_PULLUP);

  //attachInterrupt an manual wake Up when the process is in sleeping mode
  attachInterrupt(1, wakeUpManually, LOW); //use interrupt 1 (pin 3) and run function and wakeUp when pin 3 gets LOW

  //Setup LCD Screen
  pinMode(scrollingButton, INPUT_PULLUP);
  pinMode(backLight, OUTPUT);
  pinMode(powerLCD, OUTPUT);
  pinMode(contrastLCD, OUTPUT);
  //Activate LCD
  turnOnLcd();
  LCD.begin(16, 2);
  LCD.print("Hello");
  LCD.setCursor(0, 1);
  LCD.print("ChickenGuard");

  //Setup Button Open Door
  pinMode(openDoorButton, INPUT_PULLUP);
  //Setup Button Close Door
  pinMode(closeDoorButton, INPUT_PULLUP);

  //Setup Button Autmatic Mode
  pinMode(automaticModeButton, INPUT_PULLUP);

  //Setup Motor
  pinMode(pin1Motor, OUTPUT);
  pinMode(pin2Motor, OUTPUT);
  pinMode(pinPWMMotor, OUTPUT);

  //Setup Photoresistor
  pinMode(photoResistor, INPUT);

  //Setup Micro Switch Top
  pinMode(pinMicroSwitchTop, INPUT);
  //Setup Micor Switch Bottom
  pinMode(pinMicroSwitchBottom, INPUT);

  //Setup LED Door Closed
  pinMode(pinLedDoorClosed, OUTPUT);
  digitalWrite(pinLedDoorClosed, HIGH);

  //Setup LED Error
  pinMode(pinErrorLed, OUTPUT);
  digitalWrite(pinErrorLed, HIGH);

  //Setup Array of opening and closing time
  dayMonths[1][1] = 8;
  dayMonths[1][2] = 0;
  nightMonths[1][1] = 18;
  nightMonths[1][2] = 0;

  dayMonths[2][1] = 7;
  dayMonths[2][2] = 0;
  nightMonths[2][1] = 19;
  nightMonths[2][2] = 0;

  dayMonths[3][1] = 7;
  dayMonths[3][2] = 0;
  nightMonths[3][1] = 20;
  nightMonths[3][2] = 30;

  dayMonths[4][1] = 6;
  dayMonths[4][2] = 30;
  nightMonths[4][1] = 21;
  nightMonths[4][2] = 0;

  dayMonths[5][1] = 6;
  dayMonths[5][2] = 0;
  nightMonths[5][1] = 21;
  nightMonths[5][2] = 30;

  dayMonths[6][1] = 6;
  dayMonths[6][2] = 0;
  nightMonths[6][1] = 21;
  nightMonths[6][2] = 50;

  dayMonths[7][1] = 6;
  dayMonths[7][2] = 0;
  nightMonths[7][1] = 22;
  nightMonths[7][2] = 5;

  dayMonths[8][1] = 6;
  dayMonths[8][2] = 0;
  nightMonths[8][1] = 22;
  nightMonths[8][2] = 0;

  dayMonths[9][1] = 6;
  dayMonths[9][2] = 30;
  nightMonths[9][1] = 21;
  nightMonths[9][2] = 30;

  dayMonths[10][1] = 7;
  dayMonths[10][2] = 0;
  nightMonths[10][1] = 20;
  nightMonths[10][2] = 0;

  dayMonths[11][1] = 7;
  dayMonths[11][2] = 0;
  nightMonths[11][1] = 19;
  nightMonths[11][2] = 30;

  dayMonths[12][1] = 7;
  dayMonths[12][2] = 30;
  nightMonths[12][1] = 19;
  nightMonths[12][2] = 0;
}
void loop() {

  // Handle wake up Alarm process
  if (wakeUpByAlarmWasCalled)
  {
    delay(300);
    // Re-synchronize RTC with programm after wake up
    setSyncProvider(RTC.get);
    if (timeStatus() != timeSet)
      Serial.println("Unable to sync with the RTC");
    else
      Serial.println("RTC has set the system time");

    // Save the last Alarm triggering
    lastAlarmTime = myTZ.toLocal(now(), &tcr);
    Serial.print("! Wake Up at : "); printDateTime(lastAlarmTime);

    // Activate LCD
    turnOnLcd();
    LCD.begin(16, 2);
    LCD.clear(); // Clear screen
    LCD.print("Wake Up");
    LCD.setCursor(0, 1);
    LCD.print("By Alarm");
    // Sustain LCD Backlight On
    lastActivateLcdMillis = millis() - 10000;

    detachInterrupt(0); // Ensure that wakeUpNow code will not be executed during normal running time.
    // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
    RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
    RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_1, false);
    RTC.alarmInterrupt(ALARM_2, false);
    RTC.squareWave(SQWAVE_NONE);
    wakeUpByAlarmWasCalled = false;
  }

  // Handle manually wake up process
  if (wakeUpManuallyWasCalled)
  {
    delay(300);
    // Re-synchronize RTC with programm after wake up
    setSyncProvider(RTC.get);
    if (timeStatus() != timeSet)
      Serial.println("Unable to sync with the RTC");
    else
      Serial.println("RTC has set the system time");

    // Activate LCD
    turnOnLcd();
    LCD.begin(16, 2);
    LCD.clear(); // Clear screen
    LCD.print("Wake Up");
    LCD.setCursor(0, 1);
    LCD.print("Manually");
    // Sustain LCD Backlight On
    lastActivateLcdMillis = millis() - 10000;

    detachInterrupt(0); // Ensure that wakeUpNow code will not be executed during normal running time.
    // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
    RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
    RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_1, false);
    RTC.alarmInterrupt(ALARM_2, false);
    RTC.squareWave(SQWAVE_NONE);
    wakeUpManuallyWasCalled = false;
  }

  //Handle automatic process of the door
  t = myTZ.toLocal(now(), &tcr);

  tmElements_t tm;
  tm.Year = CalendarYrToTm(year(t));
  tm.Month = month(t);
  tm.Day = day(t);
  tm.Hour = dayMonths[month(t)][1];
  tm.Minute = dayMonths[month(t)][2];
  tm.Second = 0;
  openingDateTime = makeTime(tm); //Set opening datetime of the day
  //Serial.print("openingDateTime "); Serial.println(openingDateTime);
  tm.Hour = nightMonths[month(t)][1];
  tm.Minute = nightMonths[month(t)][2];
  closingDateTime = makeTime(tm); // Set closing datetime of the day

  // Only in automatic mode
  if (modeAuto) {
    //Closing Door
    if (t >= closingDateTime && analogRead(photoResistor) < nightLightThreshold && !error && !digitalRead(pinMicroSwitchBottom)) {
      resetLedDoorClosed(); // Reset if Led Door Closed is blinking
      closeDoor();
      if (error == false) {
        //Launch blink led process
        blinkLedDoorClosed = true;
        blinkLedDoorClosedPeriod = millis() + blinkLedDoorClosedDuration;
        lastClosing = t;
      }
      Serial.print("Closing Door at : "); printDateTime(lastClosing);
    }
    //Open door
    if
    ((t >= openingDateTime && t < closingDateTime) && analogRead(photoResistor) > daylightThreshold && !error && !digitalRead(pinMicroSwitchTop)) {
      openDoor();
      if (error == false) {
        lastOpening = t;
        if (powerSavingMode) {
          // Switch Off backLight LCD
          switchOffLcd();
          // set Alarm 1 for the next closing door
          t = myTZ.toLocal(now(), &tcr); // Get current time
          // Be careful if we are in switching day, it should not be happened
          int daysSavingTime = hour(t) - hour(now());
          RTC.setAlarm(ALM1_MATCH_HOURS, 0, nightMonths[month(t)][2], nightMonths[month(t)][1] - daysSavingTime, 0);
          RTC.alarm(ALARM_1);                   //ensure RTC interrupt flag is cleared
          RTC.alarmInterrupt(ALARM_1, true);
          Serial.print("Opening Door at : "); printDateTime(lastOpening);
          Serial.println("Enter in Power Saving Mode after Opening");
          delay(300);
          // Call sleepNow function
          sleepNow();
        }
      }
      return; // Return at the beginning of the loop
    }
  }

  //Handle LCD Screen
  if (digitalRead(scrollingButton) == LOW) {
    delay(300);  //Avoids button rebounce
    scrolling ++;
    if (scrolling > 7) {
      LCD.clear();
      scrolling = 1;
    }
    //Activate LCD
    turnOnLcd();
    LCD.begin(16, 2);
    switch (scrolling) {
      case 1: {
          t = myTZ.toLocal(now(), &tcr);
          LCD.clear(); // Clear screen
          printDateTimeLcd(t, 2);
          break;
        }
      case 2: {
          LCD.clear();
          LCD.print("MODE :");
          Serial.print("MODE : ");
          LCD.setCursor(0, 1);
          if (modeAuto) {
            LCD.print("AUTOMATIC");
            Serial.println("AUTOMATIC");
          } else {
            LCD.print("MANUAL");
            Serial.println("MANUAL");
          }

          break;
        }
      case 3: {
          LCD.clear();
          LCD.print("LAST OPENING :");
          if (lastOpening > 100) {
            printDateTimeLcd(lastOpening, 1);
            Serial.print("LAST OPENING : ");
            printDateTime(lastOpening);
          }
          break;
        }
      case 4: {
          LCD.clear();
          LCD.print("LAST CLOSING :");
          if (lastClosing > 100) {
            printDateTimeLcd(lastClosing, 1);
            Serial.print("LAST CLOSING : ");
            printDateTime(lastClosing);
          }
          break;
        }
      case 5: {
          LCD.clear();
          LCD.print("LAST ALARM :");
          if (lastAlarmTime > 100) {
            printDateTimeLcd(lastAlarmTime, 1);
            Serial.print("LAST ALARM : ");
            printDateTime(lastAlarmTime);
          }
          break;
        }
      case 6: {
          LCD.clear();
          LCD.print("ERROR CODE :");

          LCD.setCursor(0, 1);
          if (error) {
            LCD.print(errorMsg);
            Serial.print("ERROR CODE : ");
            Serial.println(errorMsg);
          }
          break;
        }
      case 7: {
          LCD.clear();
          LCD.print("PHOTORESISTOR :");
          Serial.print("PHOTORESISTOR :");
          LCD.setCursor(0, 1);
          LCD.print(analogRead(photoResistor));
          Serial.println(analogRead(photoResistor));
          break;
        }
    }
  }

  // Handle Backlight Timeout (20s)
  if (millis() > lastActivateLcdMillis + timeoutLcd) {
    switchOffLcd();
  }

  // Blink LED when closing door process have been performed
  if (blinkLedDoorClosed && millis() < blinkLedDoorClosedPeriod && !error && modeAuto) {
    //Handle Blink Led Door Closed
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      // if the LED is off turn it on and vice-versa:
      if (digitalRead(pinLedDoorClosed) == HIGH) {
        digitalWrite(pinLedDoorClosed, LOW);
      } else {
        digitalWrite(pinLedDoorClosed, HIGH);
      }
    } if (powerSavingMode && blinkLedDoorClosedPeriod - millis() < 5000 ) {
      // Enter in power saving mode 5s before ending of blinking closing door LED
      // Reset properties before entering in power saving mode
      resetLedDoorClosed();
      // Switch Off backLight LCD
      switchOffLcd();
      // set Alarm 1 for the next opening door
      t = myTZ.toLocal(now(), &tcr); // Get current time
      // Be careful if we are in switching day, it should not be happened
      int daysSavingTime = hour(t) - hour(now());
      RTC.setAlarm(ALM1_MATCH_HOURS, 0, dayMonths[month(t)][2], dayMonths[month(t)][1] - daysSavingTime, 0);
      RTC.alarm(ALARM_1);                   //ensure RTC interrupt flag is cleared
      RTC.alarmInterrupt(ALARM_1, true);
      Serial.println("Enter in Power Saving Mode after Closing");
      delay(300);
      // Call sleepNow function
      sleepNow();
      return; // Return at the beginning of the loop
    }
  } else {
    // Reset properties after process finished
    resetLedDoorClosed();
  }

  //Manual Opening Door
  if (digitalRead(openDoorButton) == LOW) {
    delay(300); //Avoids button rebounce
    modeAuto = false;

    // Activate LCD
    turnOnLcd();
    LCD.begin(16, 2);
    LCD.clear(); // Clear screen
    LCD.print("OPENING DOOR");
    LCD.setCursor(0, 1);
    LCD.print("IN PROGRESS...");

    // Perform Opening Door
    openDoor();

    // Switch off LCD
    switchOffLcd();
  }

  //Manual Closing Door
  if (digitalRead(closeDoorButton) == LOW) {
    delay(300);  //Avoids button rebounce
    modeAuto = false;

    // Activate LCD
    turnOnLcd();
    LCD.begin(16, 2);
    LCD.clear(); // Clear screen
    LCD.print("ClOSING DOOR");
    LCD.setCursor(0, 1);
    LCD.print("IN PROGRESS...");

    // Perform Closing Door
    closeDoor();

    // Switch off LCD
    switchOffLcd();
  }

  //Switch on Automatic Mode
  if (digitalRead(automaticModeButton) == LOW) {
    delay(300);  //Avoids button rebounce
    modeAuto = true;

    // Activate LCD
    turnOnLcd();
    LCD.begin(16, 2);
    LCD.clear(); // Clear screen
    LCD.print("AUTOMATIC MODE");
    LCD.setCursor(0, 1);
    LCD.print("ACTIVATE");
  }

  // Handle Error LED
  if (error) {
    //Handle Blink LED Error
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisError >= intervalError) {
      // save the last time you blinked the LED
      previousMillisError = currentMillis;
      // if the LED is off turn it on and vice-versa:
      if (digitalRead(pinErrorLed) == HIGH) {
        digitalWrite(pinErrorLed, LOW);
      } else {
        digitalWrite(pinErrorLed, HIGH);
      }
    }
  }
}

//Callback wakeUpManually function
void wakeUpManually()
{
  wakeUpManuallyWasCalled = true;
  modeAuto = true; // Stay in automatic mode
}

//Callback wakeUpByAlarm function
void wakeUpByAlarm()
{
  wakeUpByAlarmWasCalled = true;
  modeAuto = true;
}

void sleepNow()         // here we put the arduino to sleep
{
  // See https://playground.arduino.cc/Learning/ArduinoSleepCode for more
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here, SLEEP_MODE_PWR_DOWN is the most power savings
  sleep_enable();
  attachInterrupt(0, wakeUpByAlarm, LOW); // use interrupt 0 (pin 2) and run function wakeUpByAlarm when pin 2 gets LOW
  sleep_mode();            // here the device is actually put to sleep!!
  // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  sleep_disable();         // first thing after waking from sleep: disable sleep...
  detachInterrupt(0);      // disables interrupt 0 on pin 2 so the
  // wakeUpByAlarm code will not be executed
  // during normal running time.
}

// Handle Opening Door Process
void openDoor()
{
  long timeout = millis() + 15000;
  long timeoutMicroSwitchBottom = millis() + 2000;
  while ((millis() < timeout) && (digitalRead(pinMicroSwitchTop) == false))
  {
    if ((millis() > timeoutMicroSwitchBottom) && digitalRead(pinMicroSwitchBottom))
    {
      timeout = 1000;
    }
    // Start Motor
    handleMotor(1);
  }
  // Stop Motor
  handleMotor(3);
  Serial.println("Stop motor after opening door");
  // Somethings Wrong
  delay(300);
  if (digitalRead(pinMicroSwitchTop) == false) {
    error = true;
    errorMsg = "OVERTIME OPEN";
  }
}

//Handle Closing Door Process
void closeDoor()
{
  unsigned long timeout = millis() + 15000;
  unsigned long timeoutMicroSwitchTop = millis() + 2000;
  while ((millis() < timeout) && (digitalRead(pinMicroSwitchBottom) == false))
  {
    if ((millis() > timeoutMicroSwitchTop) && digitalRead(pinMicroSwitchTop))
    {
      timeout = 1000;
    }
    // Start Motor
    handleMotor(-1);
  }
  // Stop Motor
  handleMotor(3);
  Serial.println("Stop motor after closing door");
  // Somethings Wrong
  delay(300);
  if (digitalRead(pinMicroSwitchBottom) == false) {
    error = true;
    errorMsg = "OVERTIME CLOSE";
  }
}

// Handle Motor Direction and Power
void handleMotor(int direction) {
  //Read Power Motor
  //int powerMotor = map(analogRead(pinPowerMotor), 0, 1023, 0, 255);
  int powerMotor = 255; // Full Power
  analogWrite(pinPWMMotor, powerMotor);
  //Handle Motor Rotation
  if (direction == 1) {
    digitalWrite(pin1Motor, HIGH);
    digitalWrite(pin2Motor, LOW);
  } else if (direction == -1) {
    digitalWrite(pin1Motor, LOW);
    digitalWrite(pin2Motor, HIGH);
  } else {
    digitalWrite(pin1Motor, LOW);
    digitalWrite(pin2Motor, LOW);
  }
}

// Reset LED Door Closed properties
void resetLedDoorClosed() {
  digitalWrite(pinLedDoorClosed, HIGH);
  blinkLedDoorClosed = false;
  blinkLedDoorClosedPeriod = 0;
}

// Handle switch off LCD
void switchOffLcd() {
  LCD.clear(); // Clear screen
  digitalWrite(powerLCD, LOW);
  digitalWrite(contrastLCD, LOW);
  digitalWrite(backLight, LOW);
  lastActivateLcdMillis = 0;
}

// Handle turn on LCD 
void turnOnLcd() {
    digitalWrite(powerLCD, HIGH);
    digitalWrite(contrastLCD, HIGH);
    digitalWrite(backLight, HIGH);
    LiquidCrystal LCD(rs, en, d4, d5, d6, d7); // We re-create screen LCD Object
    lastActivateLcdMillis = millis();
}

void printDateTimeLcd(time_t t, int nbrRow) {

  if (nbrRow == 2) {
    printI00ToLcd(hour(t));
    LCD.print(hour(t));
    LCD.print(':');
    printI00ToLcd(minute(t));
    LCD.print(minute(t));
    LCD.print(':');
    printI00ToLcd(second(t));
    LCD.print(second(t));
    LCD.setCursor(0, 1); //Go to next row
    LCD.print(day(t));
    LCD.print(' ');
    LCD.print(monthShortStr(month(t)));
    LCD.print(' ');
    LCD.print(year(t));
  } else {
    LCD.setCursor(0, 1); //Go to next row
    LCD.print(day(t));
    LCD.print(' ');
    LCD.print(monthShortStr(month(t)));
    LCD.print(' ');
    printI00ToLcd(hour(t));
    LCD.print(hour(t));
    LCD.print(':');
    printI00ToLcd(minute(t));
    LCD.print(minute(t));
    LCD.print(':');
    printI00ToLcd(second(t));
    LCD.print(second(t));
  }
}

//Print to LCD an integer in "00" format (with leading zero).
void printI00ToLcd(int val) {
  if (val < 10) LCD.print('0');
}

// Print Date Time
void printDateTime(time_t t)
{
  Serial.print((day(t) < 10) ? "0" : ""); Serial.print((day(t))); Serial.print(' ');
  Serial.print(monthShortStr(month(t))); Serial.print(' '); Serial.print(year(t)); Serial.print(' ');
  Serial.print(((hour(t) < 10) ? "0" : "")); Serial.print(hour(t)); Serial.print(':');
  Serial.print(((minute(t) < 10) ? "0" : "")); Serial.print(minute(t)); Serial.print(':');
  Serial.print(((second(t) < 10) ? "0" : "")); Serial.println(second(t));
}

// Set RTC Time
void setTimeRTC(int y, int m, int d, int h, int mm, int s) {
    tmElements_t tm;
    tm.Year = y2kYearToTm(y); // Format yy
    tm.Month = m;
    tm.Day = d;
    tm.Hour = h;
    tm.Minute = mm;
    tm.Second = s;
    t = makeTime(tm);
    RTC.set(t);        //use the time_t value to ensure correct weekday is set
    setTime(t);
}
