
#include <avr/sleep.h> 
/*----------------------------------------------------------------------*
 * DS3232RTC library, example sketch to demonstrate usage of            *
 * the alarm interrupt for alarm 1 and alarm 2.                         *
 *                                                                      *
 * Notes:                                                               *
 * Using the INT/SQW pin for alarms is mutually exclusive with using    *
 * it to output a square wave. However, alarms can still be set when    *
 * a square wave is output, but then the alarm() function will need     *
 * to be used to determine if an alarm has triggered. Even though       *
 * the DS3231 power-on default for the INT/SQW pin is as an alarm       *
 * output, it's good practice to call RTC.squareWave(SQWAVE_NONE)       *
 * before setting alarms.                                               *
 *                                                                      *
 * I recommend calling RTC.alarm() before RTC.alarmInterrupt()          *
 * to ensure the RTC's alarm flag is cleared.                           *
 *                                                                      *
 * The RTC's time is updated on the falling edge of the 1Hz square      *
 * wave (whether it is output or not). However, the Arduino Time        *
 * library has no knowledge of this, as its time is set asynchronously  *
 * with the RTC via I2C. So on average, it will be 500ms slow           *
 * immediately after setting its time from the RTC. This is seen        *
 * in the sketch output as a one-second difference because the          *
 * time returned by now() has not yet rolled to the next second.        *
 *                                                                      *
 * Tested with Arduino 1.8.5, Arduino Uno, DS3231.                      *
 * Connect RTC SDA to Arduino pin A4.                                   *
 * Connect RTC SCL to Arduino pin A5.                                   *
 * Connect RTC INT/SQW to Arduino pin 2.                                *
 *                                                                      *
 * Jack Christensen 01Jan2018                                           *
 *----------------------------------------------------------------------*/ 

#include <DS3232RTC.h>      // http://github.com/JChristensen/DS3232RTC


const uint8_t SQW_PIN(2);   // connect this pin to DS3231 INT/SQW pin
int wakePin = 2;    
int count = 0;   
void setup()
{
    Serial.begin(115200);
    //Sleep Pin
    pinMode(wakePin, INPUT);

    // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
    RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
    RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_1, false);
    RTC.alarmInterrupt(ALARM_2, false);
    RTC.squareWave(SQWAVE_NONE);

    // setSyncProvider() causes the Time library to synchronize with the
    // external RTC by calling RTC.get() every five minutes by default.
    setSyncProvider(RTC.get);
    Serial.print("RTC Sync");
    if (timeStatus() != timeSet)
    {
        Serial.print(" FAIL!");
    }


    printDateTime(RTC.get());
    Serial.println(" --> Current RTC time\n");

    // configure an interrupt on the falling edge from the SQW pin
    pinMode(SQW_PIN, INPUT_PULLUP);
    
    attachInterrupt(INT0, alarmIsr, FALLING);

    // set alarm 1 for 20 seconds after every minute
    RTC.setAlarm(ALM1_MATCH_HOURS, 10, 55, 20, 0);  // daydate parameter should be between 1 and 7
    RTC.alarm(ALARM_1);                   // ensure RTC interrupt flag is cleared
    RTC.alarmInterrupt(ALARM_1, true);

    // set alarm 2 for every minute
    RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 1);    //daydate parameter should be between 1 and 7
    RTC.alarm(ALARM_2);                   //ensure RTC interrupt flag is cleared
    RTC.alarmInterrupt(ALARM_2, true);

}

volatile boolean alarmIsrWasCalled = false;

void alarmIsr()
{
    alarmIsrWasCalled = true;
}

void loop()
{
    if (alarmIsrWasCalled)
    {
        if (RTC.alarm(ALARM_1))
        {
            printDateTime( RTC.get() );
            Serial.println(" --> Alarm 1\n");
        }
        if (RTC.alarm(ALARM_2))
        {
            printDateTime( RTC.get() );
            Serial.println(" --> Alarm 2\n");
        }
        alarmIsrWasCalled = false;
    }

  // display information about the counter
  Serial.print("Awake for ");
  Serial.print(count);
  Serial.println("sec");
  count++;
  delay(1000);                           // waits for a second
 
  // compute the serial input
  if (Serial.available()) {
    int val = Serial.read();
    if (val == 'S') {
      Serial.println("Serial: Entering Sleep mode");
      delay(100);     // this delay is needed, the sleep
                      //function will provoke a Serial error otherwise!!
      count = 0;
      sleepNow();     // sleep function called here
    }
    if (val == 'A') {
      Serial.println("Hola Caracola"); // classic dummy message
    }
  }
 
  // check if it should go to sleep because of time
  if (count >= 10) {
      Serial.println("Timer: Entering Sleep mode");
      delay(100);     // this delay is needed, the sleep
                      //function will provoke a Serial error otherwise!!
      count = 0;
      sleepNow();     // sleep function called here
  }

    
}


void wakeUpNow()        // here the interrupt is handled after wakeup
{
  delay(3000);
  Serial.println("wakeUpNow");
         
}

void printDateTime(time_t t)
{
    Serial.println((day(t)<10) ? "0" : ""); Serial.print((day(t)));Serial.print(' ');
    Serial.print(monthShortStr(month(t))); Serial.print(' ');Serial.print(year(t));Serial.print(' ');
    Serial.print(((hour(t)<10) ? "0" : "")); Serial.print(hour(t));Serial.print(':');
    Serial.print(((minute(t)<10) ? "0" : "")); Serial.print(minute(t));Serial.print(':');
    Serial.print(((second(t)<10) ? "0" : "")); Serial.print(second(t));
}


void sleepNow()         // here we put the arduino to sleep
{
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and
     * wake up sources are available in which sleep mode.
     *
     * In the avr/sleep.h file, the call names of these sleep modes are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     * For now, we want as much power savings as possible, so we
     * choose the according
     * sleep mode: SLEEP_MODE_PWR_DOWN
     *
     */  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
 
    sleep_enable();          // enables the sleep bit in the mcucr register
                             // so sleep is possible. just a safety pin
 
    /* Now it is time to enable an interrupt. We do it here so an
     * accidentally pushed interrupt button doesn't interrupt
     * our running program. if you want to be able to run
     * interrupt code besides the sleep function, place it in
     * setup() for example.
     *
     * In the function call attachInterrupt(A, B, C)
     * A   can be either 0 or 1 for interrupts on pin 2 or 3.  
     *
     * B   Name of a function you want to execute at interrupt for A.
     *
     * C   Trigger mode of the interrupt pin. can be:
     *             LOW        a low level triggers
     *             CHANGE     a change in level triggers
     *             RISING     a rising edge of a level triggers
     *             FALLING    a falling edge of a level triggers
     *
     * In all but the IDLE sleep modes only LOW can be used.
     */
 
    attachInterrupt(0,wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function
                                       // wakeUpNow when pin 2 gets LOW
 printDateTime( RTC.get() );
    sleep_mode();            // here the device is actually put to sleep!!
                             // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
 
    sleep_disable();         // first thing after waking from sleep:
                             // disable sleep...
    detachInterrupt(0);      // disables interrupt 0 on pin 2 so the
                             // wakeUpNow code will not be executed
                             // during normal running time.
 
}



