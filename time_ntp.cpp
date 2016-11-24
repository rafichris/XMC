/*-----------------------------------------------------------
 *      Simple time library
 *  -------------------------------------------------------*/

#include "time_ntp.h"

unsigned long ulSecsMidnight_to_Startup = 0;  // current time since midnight: cur_time = millis()/1000 + ulSecsMidnight_to_Startup
unsigned long millis_old = 0;

bool updateClockNTP()
{	
	uint8_t timezone = 1;
	time_t rawtime;
	configTime(timezone * 3600, 0, "pool.ntp.org", "time.nist.gov");
	uint8_t i = 0;
	
	//system_set_os_print(0);
	while(time(&rawtime) == 0 && i++ < 100) delay(100); 
	//system_set_os_print(1);

	if(i==100){
	Serial.println("Time Init Failed");
	  return false;
	}
	
	Serial.println("Retrieve NTP Timestamp: " + String(ctime(&rawtime)));
	return true;
}

bool updateClockMan( uint16_t hour, uint16_t minute, uint16_t second )
{	
  ulSecsMidnight_to_Startup = (hour*60 + minute)*60 + second - millis()/1000;
	Serial.println("ulSecsMidnight_to_Startup " + String(ulSecsMidnight_to_Startup));
	return true;
}

unsigned long getSecondsSinceMidnight_man(){
    unsigned long cur_millis  = millis();
    if(millis_old > cur_millis){
      ulSecsMidnight_to_Startup = (ulSecsMidnight_to_Startup + 61367295UL / 1000) % (24*3600);
    }
    return ulSecsMidnight_to_Startup + cur_millis/1000;
}

unsigned long getSecondsSinceMidnight(){
  time_t now = time(nullptr);
  if(now){
    return (now % (24*3600));
  } else {
    return getSecondsSinceMidnight_man();
  }    
}

String getNTPTimestampString(){
    time_t now = time(nullptr);
    return String(ctime(&now));
}

String getHH_manString(){ return String((getSecondsSinceMidnight_man() / 3600) % 24);}
String getMM_manString(){ return String((getSecondsSinceMidnight_man() /   60) % 60);}
String getSS_manString(){ return String((getSecondsSinceMidnight_man()       ) % 60);}

String getHH_ntpString(){ time_t now = time(nullptr); return String((now / 3600) % 24); }
String getMM_ntpString(){ time_t now = time(nullptr); return String((now /   60) % 60); }
String getSS_ntpString(){ time_t now = time(nullptr); return String((now       ) % 60); }


