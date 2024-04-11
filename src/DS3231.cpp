#include "DS3231.h"


/**************************************************************************/
/*!
  @brief  Start I2C for the DS3231 and test succesful connection
  @param  wire pointer to the Wire object
  @param  speed SCL clock speed, default to 400000L if not procided
  @return True if Wire can find DS3231 or false otherwise.
*/
/**************************************************************************/
bool DS3231::begin(TwoWire *wire, uint32_t speed = 400000L) {
  _wire = wire;
  _wire->begin();
  if (speed != 400000L) {
    _wire->setClock(speed);
  }
  // 32kHz Enable bit is always 1 when power up, read indicated I2C is working
  return _read_register(DS3231_STATUS) & 0x08;
}

/**************************************************************************/
/*!
  @brief  Check the status register Oscillator Stop Flag to see if the DS3231
   stopped due to power loss
  @return True if the bit is set (oscillator stopped) or false if it is running
*/
/**************************************************************************/
bool DS3231::lostPower(void) {
  return _read_register(DS3231_STATUS) >> DS3231_STATUS_OSC_STOP;
}

/**************************************************************************/
/*!
  @brief  Set the date and flip the Oscillator Stop Flag
  @param dt DateTime object containing the date/time to set
*/
/**************************************************************************/
void DS3231::adjust(const DateTime &dt) {
  uint8_t buffer[] = {
    DS3231_TIME,
    _bin2bcd(dt.tm_sec),
    _bin2bcd(dt.tm_min),
    _bin2bcd(dt.tm_hour),
    _bin2bcd(dt.tm_mday),
    _bin2bcd(dt.tm_wday + 1),
    _bin2bcd(dt.tm_mon),
    _bin2bcd(dt.tm_year)
  };
  _write_register(buffer, sizeof(buffer));

  uint8_t statreg = _read_register(DS3231_STATUS);
  _write_register(DS3231_STATUS, statreg & ~(1 << DS3231_STATUS_OSC_STOP)); // clear OSC STOP flag
}

/**************************************************************************/
/*!
  @brief  Get the current date/time
  @return DateTime object with the current date/time
*/
/**************************************************************************/
DateTime DS3231::now() {
  uint8_t buffer[7]{0};
  _read_register(DS3231_TIME, buffer, sizeof(buffer));

  DateTime dt {
    .tm_sec	  = _bcd2bin(buffer[0]),
    .tm_min	  = _bcd2bin(buffer[1]),
    .tm_hour  = _bcd2bin(buffer[2] & 0x7F), // msb = 0/1 for 12/24 hours
    .tm_mday  = _bcd2bin(buffer[4]),
    .tm_wday  = _bcd2bin(buffer[3]-1),
    .tm_mon	  = _bcd2bin(buffer[5]) & 0x7F, // msb = Century
    .tm_year  = _bcd2bin(buffer[6])
  };
  return dt;
}

/**************************************************************************/
/*!
  @brief  Read the SQW pin mode
  @return DS3232_SQW_RATE_t enum. If INTCON=1, it means SQW output is off
*/
/**************************************************************************/
DS3231_SQW_RATE_t DS3231::readSquareWaveRate() {
  int rate = _read_register(DS3231_CONTROL) & ( 3 << DS3231_CONTROL_RS | (1 << DS3231_CONTROL_INTCON));
  return static_cast<DS3231_SQW_RATE_t>(rate);
}

/**************************************************************************/
/*!
  @brief   Set the SQW wave output frequency
  @param   rate Square Wave rate to one of the DS3231_SQW_RATE_t enum value
  @details The INT/SQW pin requires a pull up resistor to VCC
*/
/**************************************************************************/
void DS3231::setSquareWaveRate(DS3231_SQW_RATE_t rate) {
  uint8_t ctrl = _read_register(DS3231_CONTROL);
  ctrl &= ~DS3231_SQW_OFF; // reset bits
  _write_register(DS3231_CONTROL, ctrl | rate);
}

/**************************************************************************/
/*!
  @brief  Get the current temperature from the DS3231's temperature sensor
  @return Current temperature (float)
*/
/**************************************************************************/
float DS3231::getTemperature() {
  uint8_t buffer[2]{0};
  _read_register(DS3231_TEMPERATURE, buffer, sizeof(buffer));
  return static_cast<float>(buffer[0]) + (buffer[1] >> 6) * 0.25f;
}

/**************************************************************************/
/*!
  @brief  Set alarm 1 and armed/enabled the alarm
  @param 	dt DateTime object
  @param 	alarm_mode Desired mode, see DS3231_ALARM1_t enum
  @details Set the alarm will automatically armed the alarm with both A1IE 
  and INTCN bits in CONTROL register set. If interrupt is setup, the ISR 
  will be called when alarm is fired, alternatively by polling the 
  DS3231::isAlarmFired() method to check alarm status.
*/
/**************************************************************************/
void DS3231::setAlarm1(const DateTime *dt, DS3231_ALARM1_t alarm_mode) {

  uint8_t A1M1 = (alarm_mode & 0x01) << 7;  // Seconds bit 7.
  uint8_t A1M2 = (alarm_mode & 0x02) << 6;  // Minutes bit 7.
  uint8_t A1M3 = (alarm_mode & 0x04) << 5;  // Hour bit 7.
  uint8_t A1M4 = (alarm_mode & 0x08) << 4;  // Day/Date bit 7.
  uint8_t DY_DT = (alarm_mode & 0x10) << 2; // Day/Date bit 6. Date when 0, day of week when 1.
  uint8_t day = (DY_DT) ? weekDay(dt->tm_year, dt->tm_mon, dt->tm_mday) + 1 : dt->tm_mday;

  uint8_t buffer[] = {
    DS3231_ALARM1, 
    _bin2bcd(dt->tm_sec) | A1M1,
    _bin2bcd(dt->tm_min) | A1M2,
    _bin2bcd(dt->tm_hour) | A1M3,
    _bin2bcd(day) | A1M4 | DY_DT
  };
  _write_register(buffer, sizeof(buffer));

  uint8_t ctrl = _read_register(DS3231_CONTROL);
  ctrl |= (1 << DS3231_CONTROL_INTCON | 1 << DS3231_CONTROL_ALARM1_INT_EN);
  _write_register(DS3231_CONTROL, ctrl);

}

/**************************************************************************/
/*!
  @brief  Set alarm 2 and armed/enabled the alarm
  @param 	dt DateTime object
  @param 	alarm_mode Desired mode, see DS3231_ALARM2_t enum
  @details Set the alarm will automatically armed the alarm with both A2IE 
  and INTCN bits in CONTROL register set. If interrupt is setup, the ISR 
  will be called when alarm is fired, alternatively by polling the 
  DS3231::isAlarmFired() method to check alarm status.
*/
/**************************************************************************/
void DS3231::setAlarm2(const DateTime *dt, DS3231_ALARM2_t alarm_mode) {

  uint8_t A2M2 = (alarm_mode & 0x01) << 7;  // Minutes bit 7.
  uint8_t A2M3 = (alarm_mode & 0x02) << 6;  // Hour bit 7.
  uint8_t A2M4 = (alarm_mode & 0x04) << 5;  // Day/Date bit 7.
  uint8_t DY_DT = (alarm_mode & 0x08) << 3; // Day/Date bit 6. Date when 0, day of week when 1.
  uint8_t day = (DY_DT) ? weekDay(dt->tm_year, dt->tm_mon, dt->tm_mday) + 1 : dt->tm_mday;

  uint8_t buffer[] = {
    DS3231_ALARM2, 
    _bin2bcd(dt->tm_min) | A2M2,
    _bin2bcd(dt->tm_hour) | A2M3,
    _bin2bcd(day) | A2M4 | DY_DT
  };
  _write_register(buffer, sizeof(buffer));

  uint8_t ctrl = _read_register(DS3231_CONTROL);
  ctrl |= (1 << DS3231_CONTROL_INTCON | 1 << DS3231_CONTROL_ALARM2_INT_EN);
  _write_register(DS3231_CONTROL, ctrl);
}

/**************************************************************************/
/*!
  @brief  Get Alarm 1 status
  @param  dt Pointer to an empty DateTime object
  @param  mode Pointer to an empty ALARM1_MODE_t object
  @return enum value of DS3231_ALARM1_1, and dt object will populate the day,
  hour, minutes, and seconds fields of the alarm.
*/
/**************************************************************************/
DS3231_ALARM1_t DS3231::getAlarm1Status(DateTime *dt) {

  uint8_t buffer[4] = {0};
  _read_register(DS3231_ALARM1, buffer, sizeof(buffer));

  uint8_t seconds = _bcd2bin(buffer[0] & 0x7F);
  uint8_t minutes = _bcd2bin(buffer[1] & 0x7F);
  uint8_t hours = _bcd2bin(buffer[2] & 0x3F);

  // Determine if the alarm is set to fire based on the weekday or date of a match.
  bool isDayOfWeek = (buffer[3] & 0x40) >> 6;
  uint8_t day;
  if (isDayOfWeek) {
    day = _bcd2bin((buffer[3] & 0x0F) - 1);  // day is the weekday, 0 as Sunday
  } else {
    day = _bcd2bin(buffer[3] & 0x3F);  // day is date (day of the month)
  }

  // On 1 Jan 2024, the day-of-the-week number matches the date number.
  dt->tm_sec	= seconds;
  dt->tm_min	= minutes;
  dt->tm_hour = hours;
  dt->tm_mday = (isDayOfWeek) ? 0 : day;
  dt->tm_wday = (isDayOfWeek) ? day : 0; // 0 as Sunday, 6 as Saturday
  dt->tm_mon	= 1;
  dt->tm_year = 24;

  DS3231_ALARM1_t alarm_mode = (DS3231_ALARM1_t) 
  (   (buffer[0] & 0x80) >> 7  // A1M1 - Seconds bit
    | (buffer[1] & 0x80) >> 6  // A1M2 - Minutes bit
    | (buffer[2] & 0x80) >> 5  // A1M3 - Hour bit
    | (buffer[3] & 0x80) >> 4  // A1M4 - Day/Date bit
    | (buffer[3] & 0x40) >> 2  // DY_DT
  );

  return alarm_mode;
}

/**************************************************************************/
/*!
  @brief  Get the date/time value of Alarm2
  @param  dt Pointer to an empty DateTime object
  @param  mode Pointer to an empty ALARM1_MODE_t object
  @return enum value of DS3231_ALARM1_1, and dt object will populate the day,
  hour, minutes, and seconds fields of the alarm.
*/
/**************************************************************************/
// DateTime DS3231::getAlarm2() {
DS3231_ALARM2_t DS3231::getAlarm2Status(DateTime *dt) {
  uint8_t buffer[3] = {0};
  _read_register(DS3231_ALARM2, buffer, sizeof(buffer));

  uint8_t minutes = _bcd2bin(buffer[0] & 0x7F);
  uint8_t hours = _bcd2bin(buffer[1] & 0x3F);

  // Determine if the alarm is set to fire based on the weekday or date of a match.
  bool isDayOfWeek = (buffer[2] & 0x40) >> 6;
  uint8_t day;
  if (isDayOfWeek) {
    day = _bcd2bin((buffer[2] & 0x0F) - 1);  // day is the weekday, 0 as Sunday
  } else {
    day = _bcd2bin(buffer[2] & 0x3F);  // day is date (day of the month)
  }

  // On 1 Jan 2024, the day-of-the-week number matches the date number.
  dt->tm_sec	= 0;
  dt->tm_min	= minutes;
  dt->tm_hour = hours;
  dt->tm_mday = (isDayOfWeek) ? 0 : day;
  dt->tm_wday = (isDayOfWeek) ? day : 0; // 0 as Sunday, 6 as Saturday
  dt->tm_mon	= 1;
  dt->tm_year = 24;

  DS3231_ALARM2_t alarm_mode = (DS3231_ALARM2_t) 
  (   (buffer[0] & 0x80) >> 7  // A2M2 - Minutes bit
    | (buffer[1] & 0x80) >> 6  // A2M3 - Hour bit
    | (buffer[2] & 0x80) >> 5  // A2M4 - Day/Date bit
    | (buffer[2] & 0x40) >> 3  // DY_DT
  );

  return alarm_mode;
}

/**************************************************************************/
/*!
  @brief  check if alarm is armed (enabled)
  @param 	alarm_num Alarm number
  @return true if alarmed is armed, false if alarm is disarmed
*/
/**************************************************************************/
bool DS3231::isAlarmArmed(uint8_t alarm_num) {
  uint8_t ctrl = _read_register(DS3231_CONTROL);
  if (alarm_num == 1)
    return (ctrl & ((1 << DS3231_CONTROL_ALARM1_INT_EN)));
  else
    return ((ctrl & (1 << DS3231_CONTROL_ALARM2_INT_EN)) >> (alarm_num - 1));
}

/**************************************************************************/
/*!
  @brief  Clear status of alarm
  @param 	alarm_num Alarm number to clear
*/
/**************************************************************************/
void DS3231::clearAlarm(uint8_t alarm_num) {
  uint8_t status = _read_register(DS3231_STATUS);
  _write_register(DS3231_STATUS, status & ~(1 << (alarm_num - 1)));
}

/**************************************************************************/
/*!
  @brief  Disable alarm and clear alarm status
  @param 	alarm_num Alarm number to be disabled, it also calls clearAlarm()
*/
/**************************************************************************/
void DS3231::disableAlarm(uint8_t alarm_num) {
  uint8_t ctrl = _read_register(DS3231_CONTROL);
  _write_register(DS3231_CONTROL, ctrl & ~(1 << (alarm_num - 1)));
  clearAlarm(alarm_num);
}

/**************************************************************************/
/*!
  @brief  Get status of alarm
  @param 	alarm_num Alarm number to check status of
  @return True if alarm has been fired otherwise false
*/
/**************************************************************************/
bool DS3231::alarmFired(uint8_t alarm_num) {
  return (_read_register(DS3231_STATUS) >> (alarm_num - 1)) & 0x1;
}

/**************************************************************************/
/*!
  @brief  Enable 32KHz Output
  @details The 32kHz output is enabled by default. It requires an external
  pull-up resistor to function correctly
*/
/**************************************************************************/
void DS3231::enable32K(void) {
  uint8_t status = _read_register(DS3231_STATUS);
  _write_register(DS3231_STATUS, status | (1 << DS3231_STATUS_EN32KHZ));
}

/**************************************************************************/
/*!
  @brief  Disable 32KHz Output
*/
/**************************************************************************/
void DS3231::disable32K(void) {
  uint8_t status = _read_register(DS3231_STATUS);
  status &= ~(1 << DS3231_STATUS_EN32KHZ);
  _write_register(DS3231_STATUS, status);
}

/**************************************************************************/
/*!
  @brief  Get status of 32KHz Output
  @return True if enabled otherwise false
*/
/**************************************************************************/
bool DS3231::is32KEnabled(void) {
  return (_read_register(DS3231_STATUS) >> DS3231_STATUS_EN32KHZ) & 0x01;
}

/**************************************************************************/
/*!
  @brief  Return the day of the week.
  @param yOff Offset of the year from 2000, i.e. for 2024, it is 24
  @param m Month (1 to 12)
  @param d Day
  @return Day of week as an integer from 0 (Sunday) to 6 (Saturday).
*/
/**************************************************************************/
int8_t DS3231::weekDay(int16_t yOff, int8_t m, int8_t d) {
  const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30};

  // calculate days since year 2000 (valid for year 2000 - 2099)
  uint16_t days = d;
  for (uint8_t i=0; i<m-1; ++i)
    days += daysInMonth[i];
  if (m > 2 && yOff % 4 == 0)
    ++days;
  days = days + 365 * yOff + (yOff + 3) / 4 - 1;

  return (days + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

/**************************************************************************/
/*!
  @brief  Get date and time from string of __DATE__ and __TIME__ 
  @param  d __DATE__ 
  @param  t __TIME__
  @param  dt DateTime object for storing the date/time info
*/
/**************************************************************************/
void DS3231::getDateTime(const char* d, const char* t, DateTime* dt) {

  const char* month_name[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dev"
  };

  // parse date from __DATE__
  char dstr[12]{0};    
  memcpy(dstr, d, 12);
  char* mon = strtok(dstr, " ");
  dt->tm_mday = atoi(strtok(NULL, " "));
  dt->tm_year = atoi(strtok(NULL, " ")) - 2000;
  for (int i=0; i<12; i++) {
      if (strcmp(month_name[i], mon) == 0) {
          dt->tm_mon = i;
          break;
      }
  }
  
  // parse time from __TIME__
  char tstr[9]{0};
  memcpy(tstr, t, 9);
  dt->tm_hour = atoi(strtok(tstr, ":"));
  dt->tm_min = atoi(strtok(NULL, ":"));
  dt->tm_sec = atoi(strtok(NULL, ":"));

  // calculate day of the week
  dt->tm_wday = weekDay(dt->tm_year, dt->tm_mon, dt->tm_mday);
}

/**************************************************************************/
/*!
  @brief  Get date and time from a preset char array "2024/04/02 12:08:00"
  @param  ts pointer to the char array
  @param  dt DateTime object for storing the date/time info
*/
/**************************************************************************/
void DS3231::getDateTime(char * ts, DateTime* dt) { 
  // parse date and time from ts c-string
  dt->tm_year = atoi(strtok(ts, "/: ")) - 2000;
  dt->tm_mon = atoi(strtok(NULL, "/: "));
  dt->tm_mday = atoi(strtok(NULL, "/: "));
  dt->tm_hour = atoi(strtok(NULL, "/: "));
  dt->tm_min = atoi(strtok(NULL, "/: "));
  dt->tm_sec = atoi(strtok(NULL, "/: "));

  // calculate day of the week
  dt->tm_wday = weekDay(dt->tm_year, dt->tm_mon, dt->tm_mday);

}


/**************************************************************************/
/*!
  @brief Write values to multiple registers in sequence
  @param buf pointer to the buffer contains reg_addr + values
  @param len length of the array
*/
/**************************************************************************/
void DS3231::_write_register(uint8_t *buf, uint8_t len) {
  _wire->beginTransmission(DS3231_ADDRESS);
  _wire->write(buf, len);
  _wire->endTransmission();
}

/**************************************************************************/
/*!
  @brief Write a value to register.
  @param reg register address
  @param val value to be written
*/
/**************************************************************************/
void DS3231::_write_register(uint8_t reg, uint8_t val) {
  uint8_t buffer[2] = {reg, val};
  _wire->beginTransmission(DS3231_ADDRESS);
  _wire->write(buffer, 2);
  _wire->endTransmission();
}

/**************************************************************************/
/*!
  @brief Read a value from register.
  @param reg register address
  @return value from the register
*/
/**************************************************************************/
uint8_t DS3231::_read_register(uint8_t reg) {
  _wire->beginTransmission(DS3231_ADDRESS);
  _wire->write(reg);
  _wire->endTransmission(false);

  uint8_t data;
  _wire->requestFrom(DS3231_ADDRESS, 1);
  while(_wire->available())
    data = _wire->read();

  return data;
}

/**************************************************************************/
/*!
  @brief Read a value from register.
  @param reg register address
  @param data pointer to received data buffer
  @return len length of the data to be received
*/
/**************************************************************************/
uint8_t DS3231::_read_register(uint8_t reg, uint8_t *data, uint8_t len) {
  _wire->beginTransmission(DS3231_ADDRESS);
  _wire->write(reg);
  _wire->endTransmission(false);

  int i = 0;
  _wire->requestFrom(DS3231_ADDRESS, len);
  while(_wire->available())
    data[i++] = _wire->read();

  return len;
}
