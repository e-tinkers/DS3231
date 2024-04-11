#ifndef __DS3231_H__
#define __DS3231_H__
#include <Arduino.h>
#include <Wire.h>
#include <time.h>

#define DS3231_ADDRESS        0x68  // I2C address for DS3231

// DS3231 Registers
#define DS3231_TIME           0x00  // Register 0 - 6 for time information
#define DS3231_ALARM1         0x07
#define DS3231_ALARM2         0x0B
#define DS3231_CONTROL        0x0E
#define DS3231_STATUS         0x0F
#define DS3231_TEMPERATURE    0x11  // Temperature register (high byte - low byte is at 0x12), 10-bit temperature value

// DS3231 Register Bit Position
#define DS3231_CONTROL_ALARM1_INT_EN 0
#define DS3231_CONTROL_ALARM2_INT_EN 1
#define DS3231_CONTROL_INTCON        2
#define DS3231_CONTROL_RS            3
#define DS3231_STATUS_EN32KHZ        3
#define DS3231_STATUS_OSC_STOP       7

const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

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

typedef struct tm DateTime;

class DS3231
{
public:
  bool begin(TwoWire *wire, uint32_t speed=400000);
  bool lostPower(void);
  void adjust(const DateTime &dt);
  DateTime now();
  DS3231_SQW_RATE_t readSquareWaveRate();
  void setSquareWaveRate(DS3231_SQW_RATE_t rate);
  float getTemperature();
  void setAlarm1(const DateTime *dt, DS3231_ALARM1_t alarm_mode);
  void setAlarm2(const DateTime *dt, DS3231_ALARM2_t alarm_mode);
  DS3231_ALARM1_t getAlarm1Status(DateTime *dt);
  DS3231_ALARM2_t getAlarm2Status(DateTime *dt);
  bool isAlarmArmed(uint8_t alarm_num);
  void clearAlarm(uint8_t alarm_num);
  void disableAlarm(uint8_t alarm_num);
  bool alarmFired(uint8_t alarm_num);
  void enable32K(void);
  void disable32K(void);
  bool is32KEnabled(void);
  int8_t weekDay(int16_t yOff, int8_t m, int8_t d);
  void getDateTime(const char* d, const char* t, DateTime* dt);
  void getDateTime(char* ts, DateTime* dt);

private:
    TwoWire* _wire;

    void _write_register(uint8_t *buf, uint8_t len);
    void _write_register(uint8_t reg, uint8_t val);
    uint8_t _read_register(uint8_t reg);
    uint8_t _read_register(uint8_t reg, uint8_t *data, uint8_t len);
    uint8_t _bin2bcd(uint8_t val) { return val + 6 * (val / 10); }
    uint8_t _bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }

};
#endif
