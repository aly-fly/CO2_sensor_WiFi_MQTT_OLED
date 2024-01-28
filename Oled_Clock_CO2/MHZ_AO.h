
#ifndef MHZ_H
#define MHZ_H

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define LPF_STRENGTH  5  // how strongly is the data filtered
#define MAX_FILTER_LAG  60  // max difference between sensor readout and filter is +/-60 ppm

// types of sensors.
#define MHZ14A  141
#define MHZ19B  192
#define MHZ19C  193
#define MHZ_2K   2000
#define MHZ_5K   5000
#define MHZ_10K 10000

// status codes
#define coSTATUS_OK                   1
#define coSTATUS_INIT               -99
#define coSTATUS_NO_RESPONSE         -2
#define coSTATUS_CHECKSUM_MISMATCH   -3
#define coSTATUS_INCOMPLETE          -4
#define coSTATUS_NOT_READY           -5


class MHZ {
 public:
  MHZ(uint8_t rxpin, uint8_t txpin, uint8_t type);

  void setDebug(boolean enable);

  boolean isPreHeating();
  boolean isReady();
  void setAutoCalibrate(boolean b);
  void calibrateZero();
  void setRange(int range);
 // void calibrateSpan(int range); //only for professional use... see implementation and Dataheet.

  bool ReadUART();
  int Temperature;
  int CO2ppm;
  int CO2ppmFiltered = 0;
  int DataValid;

 private:
  uint8_t _type;
  boolean debug = false;

  byte getCheckSum(byte *packet);

  unsigned long lastRequest = 0;
  int InternalFilter = 0;  // SHL 8 bits for precision
};

#endif
