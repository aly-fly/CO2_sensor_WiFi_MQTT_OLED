#include "MHZ_AO.h"

// internal
const unsigned long MHZ14A_PREHEATING_TIME = 3L * 60L * 1000L;
const unsigned long MHZ19B_PREHEATING_TIME = 3L * 60L * 1000L;
const unsigned long MHZ19C_PREHEATING_TIME = 1L * 60L * 1000L;

const unsigned long MHZ14A_RESPONSE_TIME = (unsigned long)60 * 1000;
const unsigned long MHZ19B_RESPONSE_TIME = (unsigned long)120 * 1000;
const unsigned long MHZ19C_RESPONSE_TIME = (unsigned long)120 * 1000;


MHZ::MHZ(uint8_t rxpin, uint8_t txpin, uint8_t type) {
  _type = type;

  Serial2.begin(9600, SERIAL_8N1, rxpin, txpin);
}


/**
 * Enables or disables the debug mode (more logging).
 */
void MHZ::setDebug(boolean enable) {
  debug = enable;
  if (debug) {
    Serial.println(F("MHZ: debug mode ENABLED"));
  } else {
    Serial.println(F("MHZ: debug mode DISABLED"));
  }
}

boolean MHZ::isPreHeating() {
  if (_type == MHZ14A) {
    return millis() < (MHZ14A_PREHEATING_TIME);
  } else if (_type == MHZ19B) {
    return millis() < (MHZ19B_PREHEATING_TIME);
  } else if (_type == MHZ19C) {
    return millis() < (MHZ19C_PREHEATING_TIME);
  } else {
    Serial.println(F("MHZ::isPreHeating() => UNKNOWN SENSOR"));
    return false;
  }
}
/*
boolean MHZ::isReady() {
  if (isPreHeating()) {
    return false;
  } else if (_type == MHZ14A) {
    return lastRequest < millis() - MHZ14A_RESPONSE_TIME;
  } else if (_type == MHZ19B) {
    return lastRequest < millis() - MHZ19B_RESPONSE_TIME;
  } else if (_type == MHZ19C) {
    return lastRequest < millis() - MHZ19C_RESPONSE_TIME;
  } else {
    Serial.print(F("MHZ::isReady() => UNKNOWN SENSOR \""));
    Serial.print(_type);
    Serial.println(F("\""));
    return true;
  }
}
*/

bool MHZ::ReadUART() {
  DataValid = coSTATUS_INIT;
//  if (!isReady()) return STATUS_NOT_READY;
  if (debug) Serial.println(F("-- read CO2 uart ---"));
  byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  byte response[9];  // for answer

  if (debug) Serial.print(F("  >> Sending CO2 request"));
  Serial2.write(cmd, 9);  // request PPM CO2
  lastRequest = millis();

  // clear the buffer
  memset(response, 0, 9);

  int waited = 0;
  while (Serial2.available() == 0) {
    if (debug) Serial.print(".");
    delay(100);  // wait a short moment to avoid false reading
    if (waited++ > 10) {
      if (debug) Serial.println(F("No response after 10 seconds"));
      Serial2.flush();
      DataValid = coSTATUS_NO_RESPONSE;
      return false;
    }
  }
  if (debug) Serial.println();

  // The serial HardwareSerial can get out of sync. The response starts with 0xff, try
  // to resync.
  // TODO: I think this might be wrong any only happens during initialization?
  boolean skip = false;
  while (Serial2.available() > 0 && (unsigned char)Serial2.peek() != 0xFF) {
    if (!skip) {
      Serial.print(F("MHZ: - skipping unexpected readings:"));
      skip = true;
    }
    Serial.print(" ");
    Serial.print(Serial2.peek(), HEX);
    Serial2.read();
  }
  if (skip) Serial.println();

  if (Serial2.available() > 0) {
    int count = Serial2.readBytes(response, 9);
    if (count < 9) {
      Serial2.flush();
      DataValid = coSTATUS_INCOMPLETE;
      return false;
    }
  } else {
    Serial2.flush();
    DataValid = coSTATUS_INCOMPLETE;
    return false;
  }

  if (debug) {
    // print out the response in hexa
    Serial.print(F("  << "));
    for (int i = 0; i < 9; i++) {
      Serial.print(response[i], HEX);
      Serial.print(F("  "));
    }
    Serial.println(F(""));
  }

  // checksum
  byte check = getCheckSum(response);
  if (response[8] != check) {
    Serial.println(F("MHZ: Checksum not OK!"));
    Serial.print(F("MHZ: Received: "));
    Serial.println(response[8], HEX);
    Serial.print(F("MHZ: Should be: "));
    Serial.println(check, HEX);
    Serial2.flush();
    DataValid = coSTATUS_CHECKSUM_MISMATCH;
    return false;
  }

  CO2ppm = 256 * (int)response[2] + response[3];

  Temperature = response[4] - 44;  // - 40;

  byte status = response[5];
  if (debug) {
    Serial.print(F(" # PPM UART: "));
    Serial.println(CO2ppm);
    Serial.print(F(" # Temperature? "));
    Serial.println(Temperature);
  }

  // Is always 0 for version 14a  and 19b
  // Version 19a?: status != 0x40
  if (debug && status != 0) {
    Serial.print(F(" ! Status maybe not OK : "));
    Serial.println(status, HEX);
  } else if (debug) {
    Serial.print(F(" # Status OK: "));
    Serial.println(status, HEX);
  }

  Serial2.flush();

  if ((CO2ppm > 400) && (CO2ppm != 500) && (Temperature > -30) && (status == 0))
    DataValid = coSTATUS_OK;

  // low-pass filter
  if (DataValid == coSTATUS_OK) {
    if ((InternalFilter == 0) || (CO2ppmFiltered-CO2ppm < -MAX_FILTER_LAG) || (CO2ppmFiltered-CO2ppm > MAX_FILTER_LAG)) InternalFilter = CO2ppm << 8;  // init or big jump (like venting the room); noise is usually +/-50
    // https://github.com/jimmyberg/LowPassFilter
    // out = out + (in-out) / 4
    InternalFilter = InternalFilter + (((CO2ppm << 8) -  InternalFilter) >> LPF_STRENGTH);
    CO2ppmFiltered = InternalFilter >> 8;
  }
  
  return (DataValid == coSTATUS_OK);
}

byte MHZ::getCheckSum(byte* packet) {
  if (debug) Serial.println(F("  getCheckSum()"));
  byte i;
  unsigned char checksum = 0;
  for (i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xff - checksum;
  checksum += 1;
  return checksum;
}


void MHZ::setAutoCalibrate(boolean b)  //only available for MHZ-19B with firmware < 1.6, MHZ-19C and MHZ 14a
{
  uint8_t cmd_enableAutoCal[9] = { 0xFF, 0x01, 0x79, 0xA0, 0x00, 0x00, 0x00, 0x00, 0xE6 };
  uint8_t cmd_disableAutoCal[9] = { 0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
  if (b)
  {
  Serial2.write(cmd_enableAutoCal,9);
   }
  else
  {
    Serial2.write(cmd_disableAutoCal,9);
  }
}

void MHZ::setRange(int range) //only available for MHZ-19B < 1.6 and MH-Z 14a
{ 
   uint8_t cmd_2K[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xD0, 0x8F}; 
  uint8_t cmd_5K[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB};
  uint8_t cmd_10K[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x27, 0x10, 0x2F};
    
  switch(range)
  {
    case 1:
      Serial2.write(cmd_2K,9);
      break;
    case 2:
      Serial2.write(cmd_5K,9);
      break;
    case 3: 
      Serial2.write(cmd_10K,9);
    
  }
}

void MHZ::calibrateZero()
{
  uint8_t cmd[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
  Serial2.write(cmd,9);
}

/***** calibrateSpan() function for professional use. requires a constant atmosphere with 2K, 5k or 10k ppm CO2 and calibrateZero at first.

void MHZ::calibrateSpan(int range)
{
    char cmd_2K[9] = {0xFF, 0x01, 0x88, 0x07, 0xD0, 0x00, 0x00, 0x00, 0xA0};
    char cmd_5K[9] = {oxFF, 0x01, 0x88, 0x13, 0x88, 0x00, 0x00, 0x00, 0xDC};
    char cmd_10K[9]= {0xFF, 0x01, 0x88, 0x27, 0x10, 0x00, 0x00, 0x00, 0x40};
    
    switch(range)
    {
        case 1:
            Serial2.write(cmd_2K,9);
            break;
        case 2:
            Serial2.write(cmd_5K,9);
            break;
        case 3:
             Serial2.write(cmd_10k,9);
      }
      
  }
  ****/
           
        
    
    
