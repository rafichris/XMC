/*
* ChannelDriver.cpp - Channel driver code for XMC
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

#ifndef CHANNELDRIVER_H_
#define CHANNELDRIVER_H_
#include "Wire.h"

#define UART_INV_MASK  (0x3f << 19)
#define UART 1

#define I2C_DATA_PIN    4       /* SDA - DATA */
#define I2C_CLOCK_PIN   5       /* SCL - CLOCK */
#define PCA9685_OE_PIN  13      /* GPIO connected with PCA9685's OE pin */

const uint16_t GAMMA_VAL[256] = {0, 1, 1, 1, 1, 2, 2, 3, 4, 5, 6, 8, 9, 11, 12, 14, 16, 18, 20,
23, 25, 28, 30, 33, 36, 39, 43, 46, 49, 53, 57, 61, 64, 69, 73, 77, 82, 86, 91, 96,
101, 106, 111, 116, 122, 128, 133, 139, 145, 151, 157, 164, 170, 177, 184, 191, 197,
205, 212, 219, 227, 234, 242, 250, 258, 266, 274, 283, 291, 300, 309, 317, 326, 336,
345, 354, 364, 373, 383, 393, 403, 413, 423, 434, 444, 455, 466, 477, 488, 499, 510,
522, 533, 545, 556, 568, 580, 593, 605, 617, 630, 642, 655, 668, 681, 694, 708, 721,
735, 748, 762, 776, 790, 804, 818, 833, 847, 862, 877, 892, 907, 922, 937, 953, 968,
984, 1000, 1016, 1032, 1048, 1064, 1081, 1097, 1114, 1131, 1148, 1165, 1182, 1199, 1217,
1234, 1252, 1270, 1288, 1306, 1324, 1342, 1361, 1379, 1398, 1417, 1436, 1455, 1474, 1494,
1513, 1533, 1552, 1572, 1592, 1612, 1632, 1653, 1673, 1694, 1715, 1735, 1756, 1777, 1799,
1820, 1841, 1863, 1885, 1907, 1929, 1951, 1973, 1995, 2018, 2040, 2063, 2086, 2109, 2132,
2155, 2179, 2202, 2226, 2250, 2273, 2297, 2322, 2346, 2370, 2395, 2419, 2444, 2469, 2494,
2519, 2544, 2570, 2595, 2621, 2647, 2672, 2698, 2725, 2751, 2777, 2804, 2830, 2857, 2884,
2911, 2938, 2965, 2993, 3020, 3048, 3076, 3104, 3132, 3160, 3188, 3217, 3245, 3274, 3303,
3331, 3360, 3390, 3419, 3448, 3478, 3507, 3537, 3567, 3597, 3627, 3658, 3688, 3719, 3749,
3780, 3811, 3842, 3873, 3905, 3936, 3968, 3999, 4031, 4063, 4095}; //gamma corrected channel value.

class ChannelDriver {
 public:
    int begin();
    int begin(uint16_t  numChannels);
    void enableOutput();
    void disableOutput();
    String getOEstatus();
    void setupPins(uint8_t pinSDA, uint8_t pinSCL, uint8_t pinOE);
    void setupPCA9685(bool resetOutputs, uint16_t maxChannels);

    uint16_t getOutputValue(uint8_t inputValue);
    void setGamma(bool gamma);
    void show();

    /* Set channel value at address */
    inline void setValue(uint16_t address, uint8_t value) {
      chandata[address] = value;
    }
    /* Set channel value at address */
    inline uint8_t getValue(uint16_t address) {
        return chandata[address];
    }

 private:
      uint8_t    clockPin;    // Pin for bit-banging
      uint8_t    dataPin;     // Pin for bit-banging
      uint8_t    oePin;       // Pin for bit-banging
      bool       oePinState;
      uint16_t   numChannels; // number of channels we want to process with pca9685 module
      uint8_t    *chandata;   // Channel buffer
      uint8_t    *pbuff;      // GECE Packet Buffer
      boolean    gamma;       // Gamma correction flag
   
};

#endif /* CHANNELDRIVER_H_ */
