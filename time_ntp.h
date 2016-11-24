/*-----------------------------------------------------------
 *      Simple time library
 *  -------------------------------------------------------*/

// note: all timing relates to 01.01.2000

#ifndef _DEF_TIME_NTP_ST_
#define _DEF_TIME_NTP_ST_

#include <Arduino.h>
#include <time.h>

  bool updateClockNTP();
  bool updateClockMan(uint16_t hour, uint16_t minute, uint16_t second);
  unsigned long getSecondsSinceMidnight();
  unsigned long getSecondsSinceMidnight_man();
  String getHH_manString();
  String getMM_manString();
  String getSS_manString();
  String getHH_ntpString();
  String getMM_ntpString();
  String getSS_ntpString();
  String getNTPTimestampString();

#endif

