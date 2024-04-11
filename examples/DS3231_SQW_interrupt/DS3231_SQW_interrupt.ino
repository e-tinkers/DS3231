// This sketch demonstration using the SQW Pin as the interrupt pin to trigger the printout of time ecery 1Hz

#include <DS3231.h>
#include <time.h>

#define INT_PIN PIN_PA5

DS3231 rtc;

const char * rateStr[] = {"1Hz", "1024Hz", "4096Hz", "8192Hz"};

volatile bool interruptFlag{false};

#if (defined(MEGATINYCORE) || defined(DXCORE) || defined(ATTIYNCORE))
ISR(PORTA_PORT_vect) {
    interruptFlag = true;
    PORTA.INTFLAGS = PIN5_bm;
}
#else
void interruptHandler() {
  interruptFlag = true;
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

  if (rtc.lostPower()) {
    DateTime dt{0};
    char ts[] = "2024/04/03 10:20:30";
    rtc.getDateTime(ts, &dt);
    rtc.adjust(dt);
  }

  rtc.setSquareWaveRate(DS3231_SQW_1HZ);
  int sqw_rate = static_cast<int>(rtc.readSquareWaveRate())>> 3;
  Serial.print("Square Wave output enabled at ");
  Serial.println(rateStr[sqw_rate]);

#if (defined(MEGATINYCORE) || defined(DXCORE) || defined(ATTIYNCORE))
  // Spencer Konde of megaTinyCore called attachInterupt() as an "abdomination", so we use bare metal
  PORTA.DIRCLR = PIN5_bm;                                  // Set PA5 to input
  PORTA.PIN5CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc; // Enable PULLUP and Interrupt on PA5
#else
  // for the rest of Arduino framework, we use attachInterrupt()
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), interruptHandler, FALLING);
#endif

}

void loop () {
  if (interruptFlag) {
    interruptFlag = false;
    DateTime now = rtc.now();
    Serial.printf("%04d/%02d/%02d (%s) %02d:%02d:%02d\n",
      now.tm_year+2000, now.tm_mon, now.tm_mday, daysOfTheWeek[now.tm_wday],
      now.tm_hour, now.tm_min, now.tm_sec
    );
  }
}
