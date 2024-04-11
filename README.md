## DS3231

An Arduino Library for DS3231-based RTC modules. A lot of the work of this library is based on [Adafruit RTClib](https://github.com/adafruit/RTClib). Many of the methods remain the same function prototype but many don't. Unlike other libraries that tries to support multiple chips, this library focus only on DS3231. The library should work with different Arduino Frameworks, however most of the tests was done on Modern AVR chips, such as ATtiny 0/1/2 series. Obviously the library requires Wire library for I2C communication. Unlike other popular DS3231 libraries that has dependencies of other libraries, this library has no dependency on other external library.

### Setting the date and time for the first time

When the DS3231 is power-up for the first time, without a back-up battery, the code will try to use one of the two options (depend on which one is not comment-out, see DS3231.ino example). 

* Option 1 - Using `__DATE__` and `__TIME__`
  The `__DATE__` and `__TIME__` is the date and time when the Arduino sketch is **compiled**, if the sketch is compiled yesterday, and each time when you power-up the DS3231 without the back-up battery, yesterday's date time will be used for setting up the DS3231. If you **made a change** of the code, and immediately upload the code, the DS3231 will likely show the time that was roughly a few seconds earlier than the current time (because from compilation of the code to upload take a few seconds), just be aware of this.

* Option 2 - Using a pre-defined string in the code
  You could use a pre-define time string and set it to a future time. See DS3231.ino example
  
  ```cpp
    char ts[] = "2024/04/03 10:20:30";
  ```
  
  Compile the code, and time your upload based on the date time you set in the string to upload the code. If your DS3231 is powered without the back-up battery, the value in the time string will be used for setting up the DS3231.

While with the DS3231 is being powered, connect a 3v back-up battery to VBAT pin or in case you are usinga DS3231 module that has a back-up battery holder, insert the back-up battery after the DS3231 is configured to the right time.

### DateTime

The DateTime object used in the library is actually the good old `struct tm` in `time.h`:

```cpp
#include <time.h>

typedef struct tm DateTime;
```

This is to avoid for re-inventing the wheel by creating a complete ground-up DateTime object and uses the existing available battle-proven `time.h` for handling the datetime information and conversion. However, it is used not exactly as the `struct tm` and there are sutle differences when using with DS3231, mainly in where the "year offset" and the "week of the day" is used in DS3231.

For `time.h`, the year offset `tm.tm_year` is the offset of the year starting from 1900, so year 2024 is stored in `tm.tm_year` as 2024 - 1900 = 124. The DS3231 also use the offset of a year in its register, it is however uses an offset value of 2000, so the same 2024 is stored as 2024 - 2000 = 24. We use the `DateTime` object (i.e. a `struct tm`) following DS3231's convention. Same for the week of year, where the `tm.tm_wday` in `time.h` has a value of 0 - 6, 0 being Sunday, and 6 as Saturday. While DS3231 internal day of week register stored value as 1 - 7, 1 as Sunday and 7 as Saturday. For easy of use, index starting with 0 is better, so we handle the day of the week as per `time.h`. Just be aware in case you need to use the `time.h` built-in function for handling the date time conversion, the value of the struct need to adjust accordingly in order to perfrom the correct conversion.

### Handling week of the days

There is a pre-defined array of c-string (char array) in the `DS3231.h`:
```cpp
const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
```
In case there is a need for converting the numeric value of `DateTime.tm_wday` to English description, it can easily be done with:
```cpp
Serial.println(daysOfTheWeek[dt.tm_wday]);
```

### Pre-defined data type and enum values

There are a few pre-defined data type and enum used both in the source code, mainly for avoid of using _magic number_ in the code. Those enum values will be used in application code, mainly for configuration as well as for determine the status of the DS3231. It listed here for easy reference so that you don't need to dig the soruce code to find out what to use. The actual usage can be found in examples.

```cpp
typedef enum {
    DS3231_SQW_1HZ    = (0 << DS3231_CONTROL_RS),
    DS3231_SQW_1024HZ = (1 << DS3231_CONTROL_RS),
    DS3231_SQW_4096HZ = (2 << DS3231_CONTROL_RS),
    DS3231_SQW_8192HZ = (3 << DS3231_CONTROL_RS),
    DS3231_SQW_OFF = 0x1c
} DS3231_SQW_RATE_t;

typedef enum {
  DS3231_ALARM2_EVERY_MINUTE = 0x07, /* Alarm once per minute(when seconds are 0) */
  DS3231_ALARM2_ON_MINUTE    = 0x06, /* Alarm when minutes match */
  DS3231_ALARM2_ON_HOUR      = 0x04, /* /* Alarm when hours, minutes match */
  DS3231_ALARM2_ON_DATE      = 0x00, /* Alarm when date (day of month), hours and minutes match */
  DS3231_ALARM2_ON_WEEKDAY   = 0x08  /* Alarm when day (day of week), hours and minutes match */
} DS3231_ALARM2_t;

typedef enum {
  DS3231_ALARM1_EVERY_SECOND = 0x0F,  /* Alarm once per second */
  DS3231_ALARM1_ON_SECOND    = 0x0E,  /* Alarm when seconds match */
  DS3231_ALARM1_ON_MINUTE    = 0x0C,  /* Alarm when minutes and seconds match */
  DS3231_ALARM1_ON_HOUR      = 0x08,  /* Alarm when hours, minutes and seconds match */
  DS3231_ALARM1_ON_DATE      = 0x00,  /* Alarm when date (day of month), hours, minutes and seconds match */
  DS3231_ALARM1_ON_WEEKDAY   = 0x10   /* Alarm when day (day of week), hours, minutes and seconds match */
} DS3231_ALARM1_t;
```

### 32kHz Output

By default, DS3231 generates a 32kHz clock on its 32kHz pin, and the 32kHz output is enabled by default when powering up. It requires either an external pull-up resistor (10k to 100k) or configure the GPIO pin with pullup enabled from the host MCU.

### Square Wave (SQW) Output

The INT/SQW pin is an open-drain output that can be programmed to generate a 1Hz, 1024Hz, 4096Hz ans 8192Hz output, it requires either an external pull-up resistor (10k to 100k) or configure the GPIO pin with pullup enabled from the host MCU. Refer to DS3231_SQW_interrupt.ino for the useage of the pin to generate an interrupt of 1Hz.

The INT/SQW is a share pin between alarm interrupt and square wave output, therefore when alarm is enabled, the output of square wave needs to be disabled first.

## Reference:

[DS3231 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
