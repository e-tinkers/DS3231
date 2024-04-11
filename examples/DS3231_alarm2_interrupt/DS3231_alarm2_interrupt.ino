// This sketch demonstration the use of Alarm alarm 2 with interrupt

#include <DS3231.h>
#include <time.h>

#define INT_PIN PIN_PA5;  // INT/SQW PIN on DS3231

volatile uint8_t alarmFlag{0};

DS3231 rtc;

#if (defined(MEGATINYCORE) || defined(DXCORE) || defined(ATTIYNCORE))
ISR(PORTA_PORT_vect) {
    alarmFlag = 1;
    PORTA.INTFLAGS = PIN5_bm;
}
#else
void interruptHandler() {
  alarmFlag = 1;
}
#endif

void setup () {

  Serial.begin(115200);
  while (!Serial);

  delay(3000);

  if (! rtc.begin(&Wire)) {
    Serial.println("Couldn't find RTC");
    while (1) delay(10);
  }

  // both SQW and Alarm interrupt use the same SQW_PIN, so disable SQW first
  rtc.setSquareWaveRate(DS3231_SQW_OFF);
  rtc.disableAlarm(2);

  DateTime alarmTime{
    .tm_sec	  = 00,
    .tm_min	  = 17,
    .tm_hour  = 12,
    .tm_mday  = 11,
    .tm_wday  = 4, // 0 as Sunday, 6 as Saturday
    .tm_mon	  = 4,
    .tm_year  = 24
  };

  // rtc.setAlarm2(&alarmTime, DS3231_ALARM2_EVERY_MINUTE);
  // rtc.setAlarm2(&alarmTime, DS3231_ALARM2_ON_MINUTE);
  // rtc.setAlarm2(&alarmTime, DS3231_ALARM2_ON_HOUR);
  rtc.setAlarm2(&alarmTime, DS3231_ALARM2_ON_DATE);
  // rtc.setAlarm2(&alarmTime, DS3231_ALARM2_ON_WEEKDAY);

  if (rtc.isAlarmArmed(2)) {
    DateTime dt{0};

    DS3231_ALARM2_t mode = rtc.getAlarm2Status(&dt);
    switch (mode) {
      case DS3231_ALARM2_EVERY_MINUTE:
        Serial.printf("Alarm2: armed, Mode: Every Minute\n"); 
        break;
      case DS3231_ALARM2_ON_MINUTE:
        Serial.printf("Alarm2: armed, Mode: On Minute, xx:%02d:00\n", dt.tm_min);
        break;
      case DS3231_ALARM2_ON_HOUR:
        Serial.printf("Alarm2: armed, Mode: On Hour, %02d:%02d:00\n", dt.tm_hour, dt.tm_min);
        break;
      case DS3231_ALARM2_ON_DATE:
        Serial.printf("Alarm2: armed, Mode: On Date, xxxx:xx:%02d %02d:%02d:00\n", 
          dt.tm_mday, dt.tm_hour, dt.tm_min);
        break;
      case DS3231_ALARM2_ON_WEEKDAY:
        Serial.printf("Alarm2: armed, Mode: On WeekDay, %s %02d:%02d:00\n", 
          daysOfTheWeek[dt.tm_wday], dt.tm_hour, dt.tm_min);
        break;
    }
  }
  else {
    Serial.println("Alarm 2: disarmed");
  }

#if (defined(MEGATINYCORE) || defined(DXCORE) || defined(ATTIYNCORE))
  // Spencer Konde of megaTinyCore called attachInterupt() as an "abdomination", so we used bare metal
  PORTA.DIRCLR = PIN5_bm;                                  // Set PA5 to input
  PORTA.PIN5CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc; // Enable PULLUP and Interrupt on PA5
#else
  // for the rest of Arduino framework, we use standard attachInterrupt()
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), interruptHandler, FALLING);
#endif

}

void loop () {

  static uint8_t count = 0;

  DateTime now = rtc.now();
  Serial.printf("%04d/%02d/%02d (%s) %02d:%02d:%02d ",
    now.tm_year+2000, now.tm_mon, now.tm_mday, daysOfTheWeek[now.tm_wday],
    now.tm_hour, now.tm_min, now.tm_sec
  );
  
  if(alarmFlag) {
    Serial.println("*** Alarm 2 fired ***"); // this msg will print 10 times
    count++;
    if (count == 10) {                       // then counter is cleared
      rtc.clearAlarm(2);
      count = 0;
      alarmFlag = 0;
    }
  }
  else {
    Serial.println();
  }

  delay(1000);
}

