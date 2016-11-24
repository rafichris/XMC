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

#include <Arduino.h>
#include <utility>
#include <algorithm>
#include "ChannelDriver.h"

extern "C" {
#include <eagle_soc.h>
#include <ets_sys.h>
#include <uart.h>
#include <uart_register.h>
}

int ChannelDriver::begin() {
    return begin(16);
}

int ChannelDriver::begin(uint16_t  numChannels) {
  int retval = true;
  this->numChannels = numChannels;

  if (chandata) free(chandata);
  if (chandata = static_cast<uint8_t *>(malloc(numChannels))) {
      memset(chandata, 0, numChannels);
      this->numChannels = numChannels;
  } else {
      this->numChannels = 0;
      retval = false;
  }

  return retval;
}

void ChannelDriver::setupPins(uint8_t pinSDA, uint8_t pinSCL, uint8_t pinOE) {
    if (pinSCL >= 0)
        this->clockPin = pinSCL;

    if (pinSDA >= 0)
        this->dataPin = pinSDA;

    if (pinOE >= 0)
        this->oePin = pinOE;
        
    /* Initial pin states */
    pinMode(this->dataPin, OUTPUT);
    digitalWrite(this->dataPin, LOW);
    
    pinMode(this->clockPin, OUTPUT);
    digitalWrite(this->clockPin, LOW);

    pinMode(this->oePin, OUTPUT);
    digitalWrite(this->oePin, HIGH); //set OE high to disable pca9685 outputs
    oePinState = false;
    
    delay(100);
    
    Wire.begin(this->dataPin,this->clockPin); //start Wire and set SDA - SCL

    delay(100);
}

void ChannelDriver::enableOutput(){
    if(oePinState==false){
        digitalWrite(this->oePin, LOW);
        oePinState = true;
    }
}

void ChannelDriver::disableOutput(){
    if(oePinState){
        digitalWrite(this->oePin, HIGH);
        oePinState = false;
    }
}

String ChannelDriver::getOEstatus(){
    if(oePinState) return "OE:enabled";
    else return "OE:disabled";
}

void ChannelDriver::setupPCA9685(bool resetOutputs, uint16_t maxChannels) {
    // Datasheet: https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf
    
    // disable output first
    digitalWrite(this->oePin, HIGH); //set OE high to disable outputs.
    oePinState = false;

    // setup pin assignment
    setupPins(I2C_DATA_PIN, I2C_CLOCK_PIN, PCA9685_OE_PIN);

    // loop over all boards (1 master + x slaves)
    //  starting with the master board in ascending address order
    // number of required boards: ((numChannels - 1) >> 4) + 1
    for (uint8_t i = 0; i < (((numChannels - 1) >> 4) + 1); i++) {
      Wire.beginTransmission(0x40 + i); // start comms with board 0x40.
      Wire.write(00);                   // move pointer to MODE 1 register.
      Wire.write(0xa1);                 // enables ALL CALL ADDRESS, Register Auto-Increment & Restart.
      Wire.endTransmission();           // end comms with board.
    }                                                                            
 
    Wire.beginTransmission(0xE0);   // start comms with all boards (ALLCALLADR).
    Wire.write(01);                 // move pointer to MODE 2 register.
    Wire.write(0x02);               // (0x04) invert outputs for use with FETs - (0x02) to set as source.
    Wire.endTransmission();         // end comms with board.
                                                                                              
    if(resetOutputs){               // set all outputs to zero
      //TODO: Set all outputs to zero
      Wire.beginTransmission(0xE0);   // start comms with all boards (ALLCALLADR).
      Wire.write(0x06);               // move pointer to LED 0 register.
        
      for (uint8_t i = 0; i < 4*16; i++) { //loop 64 times.
        Wire.write(0x0F);             // write on time 0, off time 0 to all LEDs (all off).
      }                                                                                      
      Wire.endTransmission();         // end comms with board.

      // WORKAROUND: if allcalladr is not working
      uint16_t tmp = numChannels;
      numChannels = maxChannels;
      show();
      numChannels = tmp;
    }     

    // enable output
    digitalWrite(this->oePin, LOW); //set OE low to enable outputs.
    oePinState = true;
}

void ChannelDriver::setGamma(bool gamma_flag) {
    gamma = gamma_flag;
}

uint16_t ChannelDriver::getOutputValue(uint8_t inputValue){
  if(gamma)
    return GAMMA_VAL[inputValue];
  
  return (inputValue << 4);
}

void ChannelDriver::show() {
    if (!chandata) return;
    
    uint16_t channelValue = 0;
    uint16_t outputValue  = 0;
    bool comOpen = false;
    
    for (uint16_t uiIdx = 1; uiIdx <= numChannels; uiIdx++) { //cycle through all channels.       
      if(uiIdx % 16 == 1){
        // start communication with boards 
        Wire.beginTransmission(0x40 + ((uiIdx-1) >> 4)); //start comms with board 0x40.
        Wire.write(0x06);                               //move pointer to LED 0 register.
        comOpen = true;
      }

      outputValue = getOutputValue(chandata[uiIdx-1]);

      Wire.write (0x00);               //switch on time LSB - switches on at zero.
      Wire.write (0x00);               //switch on time MSB.
      Wire.write (outputValue & 0xFF); // switch off time LSB - switches off after OUTPUT_VALUE cycles.
      Wire.write (outputValue >> 8);   // bit shift to remove LSB., switch off time MSB.
    
      if(uiIdx % 16 == 0){
        // close communication
        Wire.endTransmission();
        comOpen = false;
      }
    
    }

    if(comOpen){
      // close communication if open
      Wire.endTransmission();
      comOpen = false;
    }

}
