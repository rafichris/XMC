/*
* XMC.h
*
* Project: XMC - An ESP8266 and E1.31 based LED strips driver
*  Copyright (c) 2016 Christoph Rafetzeder 
*  Special thanks to Dieter Reinhardt and Markus Felinger 
*  for the inspiration and their support.
* 
*  Some parts of this project are forked from:
*   1.) http://www.esp8266.com/viewtopic.php?f=152&t=8814 from RichardS 
*   2.) https://github.com/forkineye/ESPixelStick from http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it. Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#ifndef XMC_H_
#define XMC_H_

#include "ChannelDriver.h"
#include "Wire.h"
#include "_E131.h"

/* Name and version */
const char VERSION[] = "1.0";

/* Configuration file params */
const char CONFIG_FILE[] = "/config.json";
#define CONFIG_MAX_SIZE 2048    /* Sanity limit for config file */

#define HTTP_PORT       80      /* Default web server port */
#define BUILT_IN_LED    2       /* BUILTIN_LED */

#define EEPROM_BASE     0       /* EEPROM configuration base address */
#define UNIVERSE_LIMIT  512     /* Universe boundary - 512 Channels */
#define CHANNEL_LIMIT   992     /* Total channel limit - 16 channels per pca9685 * 62 (refer to pca9685 datasheet, available adresses */
#define CONNECT_TIMEOUT 15000   /* 15 seconds */
#define REBOOT_DELAY    100     /* Delay for rebooting once reboot flag is set */
#define LOG_PORT        Serial  /* Serial port for console logging */
#define UPD_INTERVAL    1000    /*  */

/* E1.33 / RDMnet stuff - to be moved to library */
#define RDMNET_DNSSD_SRV_TYPE   "draft-e133.tcp"
#define RDMNET_DEFAULT_SCOPE    "default"
#define RDMNET_DEFAULT_DOMAIN   "local"
#define RDMNET_DNSSD_TXTVERS    1
#define RDMNET_DNSSD_E133VERS   1

// PINOUT            // Pin  Function                      ESP-8266 Pin 
                     //  see http://escapequotes.net/wemos-d1-mini-pins-and-diagram/
#define PIN_D0 16    // D0   IO                            GPIO16
#define PIN_D1  5    // D1   IO, SCL                       GPIO5
#define PIN_D2  4    // D2   IO, SDA                       GPIO4
#define PIN_D3  0    // D3   IO,Pull-up                    GPIO0
#define PIN_D4  2    // D4   IO,pull-up, BUILTIN_LED       GPIO2
#define PIN_D5  14   // D5   IO, SCK                       GPIO14
#define PIN_D6  12   // D6   IO, MISO                      GPIO12
#define PIN_D7  13   // D7   IO, MOSI                      GPIO13
#define PIN_D8  15   // D8   IO,pull-down, SS              GPIO15

/* Configuration structure */
typedef struct {
    /* Device */
    String      id;             /* Device ID */

    /* Network */
    String      ssid;
    String      passphrase;
    String      hostname;
    uint8_t     ip[4];
    uint8_t     netmask[4];
    uint8_t     gateway[4];
    bool        dhcp;           /* Use DHCP */
    bool        ap_fallback;    /* Fallback to AP if fail to associate */

    /* E131 */
    uint16_t    universe;       /* Universe to listen for */
    uint16_t    channel_start;  /* Channel to start listening at - 1 based */
    uint16_t    channel_count;  /* Number of channels */
    bool        multicast;      /* Enable multicast listener */

    /* Channels */
    bool        channel_gamma;   /* Use gamma map? */
    bool        toggle;          /* Use toggle built-in led */
    bool        zero;            /* Reset all PCA9685 values to zero during startup */
    String      mapping;         /* Map DMX channels to XMC output port */

    /* Scheduler */
    bool        schedule;        /* Use scheduler */
    uint8_t     scheduler_vals[4][2][2]; /* 1|2|3|4 / start|end / hh|mm */ 
    uint8_t     dmxTimeout;

} config_t;

/* Globals */
E131                e131;
config_t            config;
uint32_t            *seqError;      /* Sequence error tracking for each universe */
uint16_t            uniLast = 1;    /* Last Universe to listen for */
uint16_t            *mapping;       /* Mapping Array: Map DMX (index) to Output */
bool                reset = false;  /* Flag to reset the ESP */
bool                reboot = false; /* Flag to reboot the ESP */
bool                ntp_update = false; /* Update NTP timestamp */

uint16_t             demoChannelValue = 1;     /* Initial demo value */
uint8_t              demo             = 0;     /* Demo type for sequences 0=off */
static unsigned long lWaitMillis      = 1;     /* Waiting time for demo sequences */
uint8_t              demoCounter      = 0;     /* Auxiliary value for demo sequences */
bool                 gpio2State       = false; /* GPIO 2 state - toggle GPIO2 led if enabled */

unsigned long       curTime;
unsigned long       start_midnight;
unsigned long       end_midnight;
bool                locked = 0;
unsigned long       lastMillis = 0;
unsigned long       lastDMXPacket = 0;

/* Called from web handlers */
void saveConfig();

/* Forward Declarations */
void serializeConfig(String &jsonString, bool pretty = false, bool creds = false);
void loadConfig();
int  initWifi();
void initWeb();
void updateConfig();
bool enableOutput_scheduler();

/* Toggle ESP-12E's built-in LED */
void toggleBuiltInLED(){
  if(config.toggle){
    if(gpio2State){ digitalWrite(PIN_D4, LOW); gpio2State = false;}
    else{ digitalWrite(PIN_D4, HIGH); gpio2State = true;}
  }
}

#endif /* XMC_H_ */
