#ifndef WIFI_WPS_H
#define WIFI_WPS_H

enum WifiState_t {disconnected, connecting, connected, wps_active, wps_success, wps_failed, num_states};
extern WifiState_t WifiState;

extern double GeoLocTZoffset;

#endif // WIFI_WPS_H
