/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
              Modified to use with Arduipi board http://hallard.me/arduipi
						  Changed to use modified bcm2835 and RF24 library
TMRh20 2014 - Updated to work with optimized RF24 Arduino library

 */

/**
 * V202 RX
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <RF24/RF24.h>
#include "v202.h"

using namespace std;


// RX frame of size 16
bool checkCRC (uint8_t * frame)
{
  uint8_t sum = 0;
  for (uint8_t i = 0; i < 15; ++i)
    {
      sum += frame[i];
    }
  return (sum == frame[15]);
}

// Check 3 byte TX ID against frame
inline bool checkTXaddr (uint8_t * frame, uint8_t * txID)
{
  return (txID[0] == frame[7] && txID[1] == frame[8] && txID[2] == frame[9]);
}

//
// Hardware configuration
// Configure the appropriate pins for your connections

/****************** Raspberry Pi ***********************/

// Radio CE Pin, CSN Pin, SPI Speed
// See http://www.airspayce.com/mikem/bcm2835/group__constants.html#ga63c029bd6500167152db4e57736d0939 and the related enumerations for pin information.

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ);

// NEW: Setup for RPi B+
//RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// Setup for GPIO 15 CE and CE0 CSN with SPI Speed @ 8Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

// RPi generic:
RF24 radio (22, 0);

/*** RPi Alternate ***/
//Note: Specify SPI BUS 0 or 1 instead of CS pin number.
// See http://tmrh20.github.io/RF24/RPi.html for more information on usage

//RPi Alternate, with MRAA
//RF24 radio(15,0);

//RPi Alternate, with SPIDEV - Note: Edit RF24/arch/BBB/spi.cpp and  set 'this->device = "/dev/spidev0.0";;' or as listed in /dev
//RF24 radio(22,0);


/****************** Linux (BBB,x86,etc) ***********************/

// See http://tmrh20.github.io/RF24/pages.html for more information on usage
// See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
// See https://www.kernel.org/doc/Documentation/spi/spidev for more information on SPIDEV

// Setup for ARM(Linux) devices like BBB using spidev (default is "/dev/spidev1.0" )
//RF24 radio(115,0);

//BBB Alternate, with mraa
// CE pin = (Header P9, Pin 13) = 59 = 13 + 46 
//Note: Specify SPI BUS 0 or 1 instead of CS pin number. 
//RF24 radio(59,0);

/********************************/

const uint8_t START_CHANNEL = 0;
const uint8_t END_CHANNEL = 125;

int
main (int argc, char **argv)
{

  // Setup and configure rf radio
  radio.begin ();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries (15, 15);
  radio.setDataRate (RF24_1MBPS);
  radio.setChannel (0x17);
  radio.setAutoAck (false);
  radio.setCRCLength (RF24_CRC_DISABLED);
  radio.printDetails ();

  string input = "";
  cout << "Press any key to start ...\n>";
  getline (cin, input);

  /***********************************/
  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  uint8_t txid[3] = { 0xa7, 0x59, 0x03 };
  const uint8_t rx_tx_addr[5] = { 0x66, 0x88, 0x68, 0x68, 0x68 };

  radio.openReadingPipe (0, rx_tx_addr);

  uint8_t ch = START_CHANNEL;
  //radio.startListening ();

  while (1)
    {
    radio.setChannel(ch);
    radio.startListening ();
    delay(1000);
      // if there is data ready
      if (radio.available ())
	{
	  uint8_t frame[16];

	  // Fetch the payload, and see if this was the last one.
	  while (radio.available ())
	    {
	      radio.read (&frame, 16);
	    }
	  bool crc_ok = checkCRC (frame);
	  bool tx_id_match = checkTXaddr (frame, txid);
	  if (crc_ok && tx_id_match)
	    {
	      printf ("[%d] Frame(%d) throttle:%d, yaw:%d, pitch:%d\n",
		      ch, sizeof (frame), frame[0], frame[1], frame[2]);
	    }
	}
    radio.stopListening();
      ch = ch + 1;
      if (ch > END_CHANNEL)
	      ch = START_CHANNEL;

    } // forever loop

  return 0;
}
