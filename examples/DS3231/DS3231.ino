// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
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

  if (rtc.lostPower()) {
    DateTime dt{0};

    /* option 1 Get date/time from coode compile __DATE__ & __TIME__ */
    // Serial.println("Set date and time using code compiled time");
    // rtc.getDateTime(__DATE__, __TIME__, &dt);

    /* option 2 - Get date/time from a pre-defined ts string */
    char ts[] = "2024/04/03 10:20:30";
    Serial.print("Set date and time using timerString ");
    Serial.println(ts);
    rtc.getDateTime(ts, &dt);

    rtc.adjust(dt);
  }

  Serial.print("Internal Temperature: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C (+/- 3 degrees)");

  if (rtc.is32KEnabled()) 
    Serial.println("32kHz Pin output enabled"); 
  else
    Serial.println("32kHz Pin output disabled");

  DS3231_SQW_RATE_t sqw_rate = rtc.readSquareWaveRate();
  if (sqw_rate == DS3231_SQW_OFF) {
    Serial.println("Square Wave output disabled");
  }
  else {
    const char * rateStr[] = {"1Hz", "1024Hz", "4096Hz", "8192Hz"};
    sqw_rate = (DS3231_SQW_RATE_t) (sqw_rate >> 3);
    Serial.print("Square Wave output enabled at ");
    Serial.println(rateStr[sqw_rate]);
  }

  Serial.println();
}

void loop () {
    DateTime now = rtc.now();

    Serial.printf("%04d/%02d/%02d (%s) %02d:%02d:%02d\n",
      now.tm_year+2000, now.tm_mon, now.tm_mday, daysOfTheWeek[now.tm_wday],
      now.tm_hour, now.tm_min, now.tm_sec
    );

    delay(1000);
}
