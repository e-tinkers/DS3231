// This sketch demonstration the use of Alarm 1 with polling

#include <DS3231.h>
#include <time.h>

DS3231 rtc;

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
  rtc.disableAlarm(1);

  DateTime alarmTime{
    .tm_sec	  = 00,
    .tm_min	  = 10,
    .tm_hour  = 20,
    .tm_mday  = 9,
    .tm_wday  = 3, // 1 as Sunday, 7 as Saturday
    .tm_mon	  = 4,
    .tm_year  = 24
  };

  // rtc.setAlarm1(&alarmTime, DS3231_ALARM1_EVERY_SECOND);
  rtc.setAlarm1(&alarmTime, DS3231_ALARM1_ON_SECOND);
  // rtc.setAlarm1(&alarmTime, DS3231_ALARM1_ON_MINUTE);
  // rtc.setAlarm1(&alarmTime, DS3231_ALARM1_ON_HOUR);
  // rtc.setAlarm1(&alarmTime, DS3231_ALARM1_ON_DATE);
  // rtc.setAlarm1(&alarmTime, DS3231_ALARM1_ON_WEEKDAY);

  if (rtc.isAlarmArmed(1)) {
    DateTime dt{0};

    DS3231_ALARM1_t mode = rtc.getAlarm1Status(&dt);
    switch (mode) {
      case DS3231_ALARM1_EVERY_SECOND:
        Serial.printf("Alarm1: armed, Mode: Every Second\n"); 
        break;
      case DS3231_ALARM1_ON_SECOND:
        Serial.printf("Alarm1: armed, Mode: On Second, xx:xx:%02d\n", dt.tm_sec);
        break;
      case DS3231_ALARM1_ON_MINUTE:
        Serial.printf("Alarm1: armed, Mode: On Minute, xx:%02d:%02d\n", dt.tm_min, dt.tm_sec);
        break;
      case DS3231_ALARM1_ON_HOUR:
        Serial.printf("Alarm1: armed, Mode: On Hour, %02d:%02d:%02d\n", dt.tm_hour, dt.tm_min, dt.tm_sec);
        break;
      case DS3231_ALARM1_ON_DATE:
        Serial.printf("Alarm1: armed, Mode: On Date, xxxx:xx:%02d %02d:%02d:%02d\n", 
          dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
        break;
      case DS3231_ALARM1_ON_WEEKDAY:
        Serial.printf("Alarm1: armed, Mode: On WeekDay, %s %02d:%02d:%02d\n", 
          daysOfTheWeek[dt.tm_wday], dt.tm_hour, dt.tm_min, dt.tm_sec);
        break;
    }
  }
  else {
    Serial.println("Alarm 1: disarmed");
  }

}

void loop () {

  static uint8_t count = 0;

  DateTime now = rtc.now();
  Serial.printf("%04d/%02d/%02d (%s) %02d:%02d:%02d ",
    now.tm_year+2000, now.tm_mon, now.tm_mday, daysOfTheWeek[now.tm_wday],
    now.tm_hour, now.tm_min, now.tm_sec
  );
  
  if (rtc.alarmFired(1)) {
    Serial.println("*** Alarm 1 fired ***"); // this msg will print 10 times
    count++;
    if (count == 10) {                       // then counter is cleared
      rtc.clearAlarm(1);
      count = 0;
    }
  }
  else {
    Serial.println();
  }

  delay(1000);
}
