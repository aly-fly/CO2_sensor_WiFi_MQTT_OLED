#include "Clock.h"
#include "WiFi_WPS.h"

bool time_sync_ok;

void Clock::begin() {
  time_sync_ok = false;
  setTimeZoneOffset(0 * 3600);  // UTC
  
  ntpTimeClient.begin();
  ntpTimeClient.update();
  Serial.print("NTP time = ");
  Serial.println(ntpTimeClient.getFormattedTime());
  setSyncProvider(&Clock::syncProvider);
  setSyncInterval(NTP_INTERVAL_SEC); // set the number of seconds between re-sync
}

void Clock::loop() {
  if (timeStatus() == timeNotSet) {
    time_valid = false;
  }
  else {
    loop_time = now();
    local_time = loop_time + time_zone_offset;
    time_valid = time_sync_ok;
  }
}


// Static methods used for sync provider to TimeLib library.
time_t Clock::syncProvider() {
  Serial.println("syncProvider()");
  time_t ntp_now;
  time_sync_ok = false;

  if (WifiState == connected) { 
    // It's time to get a new NTP sync
    Serial.print("Getting NTP.");
    if (ntpTimeClient.update()) {
      Serial.print(".");
      ntp_now = ntpTimeClient.getEpochTime();
      Serial.println("NTP query done.");
      Serial.print("NTP time = ");
      Serial.println(ntpTimeClient.getFormattedTime());
      Serial.print("NTP = ");
      Serial.println(ntp_now);
      Serial.println("Using NTP time.");
      time_sync_ok = true;
      return ntp_now;
    } else {  // NTP valid
    Serial.println("Invalid NTP response!");
    return 0; // do not perform update of the clock
    }
  } // no wifi
  Serial.println("No WiFi!");
  return 0; // do not perform update of the clock
}


WiFiUDP Clock::ntpUDP;
NTPClient Clock::ntpTimeClient(ntpUDP);
