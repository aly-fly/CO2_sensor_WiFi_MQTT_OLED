#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>
#include "_USER_DEFINES.h"
#include <TimeLib.h>

// For NTP
#include <WiFi.h>
#include "NTPClient_AO.h"

enum timeValid_t {invalid, unsynced, ok};
extern bool time_sync_ok;

class Clock {
public:
  Clock() : loop_time(0), local_time(0), time_valid(false) {}
  
  // The global WiFi from WiFi.h must already be .begin()'d before calling Clock::begin()
  void begin(); 
  void loop();

  // Calls NTPClient::getEpochTime() or RTC::get() as appropriate
  // This has to be static to pass to TimeLib::setSyncProvider.
  static time_t syncProvider();

  // Internal time is kept in UTC. This affects the displayed time.
  void setTimeZoneOffset(time_t offset) { time_zone_offset = offset; }

  // Proxy C functions from TimeLib.h
  time_t  getLocalTime()   { return local_time; }
  uint16_t getYear()       { return year(local_time); }
  uint8_t getMonth()       { return month(local_time); }
  uint8_t getDay()         { return day(local_time); }
  uint8_t getHour()        { return hour(local_time); }
  uint8_t getHour12()      { return hourFormat12(local_time); }
  uint8_t getHour24()      { return hour(local_time); }
  uint8_t getMinute()      { return minute(local_time); }
  uint8_t getSecond()      { return second(local_time); }
  bool isAm()              { return isAM(local_time); }
  bool isPm()              { return isPM(local_time); }

  static NTPClient ntpTimeClient;
  bool time_valid;

private:
  time_t loop_time, local_time, time_zone_offset;

  // Static variables needed for syncProvider()
  static WiFiUDP ntpUDP;
};



#endif // CLOCK_H
