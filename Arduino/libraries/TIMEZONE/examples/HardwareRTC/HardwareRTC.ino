/*----------------------------------------------------------------------*
 * Timezone library example sketch.                                     *
 * Self-adjusting clock for one time zone using an external real-time   *
 * clock, either a DS1307 or DS3231 (e.g. Chronodot).                   *
 * Assumes the RTC is set to UTC.                                       *
 * TimeChangeRules can be hard-coded or read from EEPROM, see comments. *
 * Check out the Chronodot at http://www.macetech.com/store/            *
 *                                                                      *
 * Jack Christensen Aug 2012                                            *
 *                                                                      *
 * CC BY-SA 4.0: This work is licensed under the Creative Commons       *
 * Attribution-ShareAlike 4.0 International License,                    *
 * https://creativecommons.org/licenses/by-sa/4.0/                      *
 *----------------------------------------------------------------------*/

#include <DS3232RTC.h>   // https://github.com/PaulStoffregen/DS1307RTC
#include <Timezone.h>    // https://github.com/JChristensen/Timezone

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone myTZ(CEST, CET);

// If TimeChangeRules are already stored in EEPROM, comment out the three
// lines above and uncomment the line below.
// Timezone myTZ(100);       //assumes rules stored at EEPROM address 100

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;

void setup(void)
{
    Serial.begin(115200);
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    if(timeStatus()!= timeSet)
        Serial.println("Unable to sync with the RTC");
    else
        Serial.println("RTC has set the system time");
}

void loop(void)
{
    Serial.println();
    utc = now();
    printTime(utc, "UTC");
    local = myTZ.toLocal(utc, &tcr);
    printTime(local, tcr -> abbrev);
    delay(10000);
}

// Function to print time with time zone
void printTime(time_t t, const char *tz)
{
    sPrintI00(hour(t));
    sPrintDigits(minute(t));
    sPrintDigits(second(t));
    Serial.print(' ');
    Serial.print(dayShortStr(weekday(t)));
    Serial.print(' ');
    sPrintI00(day(t));
    Serial.print(' ');
    Serial.print(monthShortStr(month(t)));
    Serial.print(' ');
    Serial.print(year(t));
    Serial.print(' ');
    Serial.print(tz);
    Serial.println();
}

// Print an integer in "00" format (with leading zero).
// Input value assumed to be between 0 and 99.
void sPrintI00(int val)
{
    if (val < 10) Serial.print('0');
    Serial.print(val, DEC);
    return;
}

// Print an integer in ":00" format (with leading zero).
// Input value assumed to be between 0 and 99.
void sPrintDigits(int val)
{
    Serial.print(':');
    if(val < 10) Serial.print('0');
    Serial.print(val, DEC);
}
