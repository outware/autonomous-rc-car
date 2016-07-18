/* v202_rx.ino -- An arduino sketch to test the protocol v202
 *
 * Copyright (C) 2016 execuc
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

//NOTE: Bind the controller after the code is running on the arduino

#include <SPI.h>
#include "v202_protocol.h"

nrf24l01p wireless; 
v202Protocol protocol;

unsigned long time = 0;
 
// the setup routine runs once when you press reset:
void setup() {
  // SS pin must be set as output to set SPI to master !
  pinMode(SS, OUTPUT);
  Serial.begin(115200);
  Serial.print("---- Arduino 2.4ghz v202 protocol RECEIVER ----");
  // Set CE pin to D8 (D9 on Mega2560) and CS pin to D7 (D53 on Mega2560)
  wireless.setPins(9, 53);
  protocol.init(&wireless);
  
  time = micros();
  Serial.println("Start");
}

rx_values_t rxValues;

bool bind_in_progress = false;
unsigned long newTime;

void loop() 
{
  time = micros();
  uint8_t value = protocol.run(&rxValues); 
  newTime = micros();
   
  switch( value )
  {
    case  BIND_IN_PROGRESS:
      if(!bind_in_progress)
      {
        bind_in_progress = true;
        Serial.println("Bind in progress");
      }
    break;
    
    case BOUND_NEW_VALUES:
      //newTime = micros();
      /*Serial.print(newTime - time); //120 ms for 16 Mhz
      Serial.print(" :\t");Serial.print(rxValues.throttle);
      Serial.print("\t"); Serial.print(rxValues.yaw);
      Serial.print("\t"); Serial.print(rxValues.pitch);
      Serial.print("\t"); Serial.print(rxValues.roll);
      Serial.print("\t"); Serial.print(rxValues.trim_yaw);
      Serial.print("\t"); Serial.print(rxValues.trim_pitch);
      Serial.print("\t"); Serial.print(rxValues.trim_roll);
      Serial.print("\t"); Serial.print(rxValues.flags);
      Serial.print("\t"); Serial.println(rxValues.crc);*/
      //time = newTime;

      //Serial.print("0x0");Serial.print(String(protocol.mRfChannels[protocol.mRfChNum],HEX));  //Channel the packet was received on
      Serial.print(protocol.mRfChannels[protocol.mRfChNum]);  //Channel the packet was received on
      Serial.print("\t");
      Serial.print(newTime - time); //120 ms for 16 Mhz (currently showing as 180ms)
      Serial.print(" :\t");Serial.print(rxValues.throttle);
      Serial.print(" :\t Throttle:");Serial.print(rxValues.throttle);
      Serial.print("\t Yaw (Steer):"); Serial.print(rxValues.yaw);
      Serial.print("\t Pitch (Throttle):"); Serial.print(rxValues.pitch);
      Serial.print("\t Roll:"); Serial.print(rxValues.roll);
      Serial.print("\t TY (ST):"); Serial.print(rxValues.trim_yaw);
      Serial.print("\t TP (TH):"); Serial.print(rxValues.trim_pitch);
      Serial.print("\t TR:"); Serial.print(rxValues.trim_roll);
      Serial.print("\t f6:"); Serial.print(rxValues.frame6);
      Serial.print("\t f7:"); Serial.print(rxValues.frame7);
      Serial.print("\t f8:"); Serial.print(rxValues.frame8);
      Serial.print("\t f9:"); Serial.print(rxValues.frame9);
      Serial.print("\t f10:"); Serial.print(rxValues.frame10);
      Serial.print("\t f11:"); Serial.print(rxValues.frame11);
      Serial.print("\t f12:"); Serial.print(rxValues.frame12);
      Serial.print("\t f13:"); Serial.print(rxValues.frame13);
      Serial.print("\t Flags:"); Serial.print(rxValues.flags);
      Serial.print("\t CRC:"); Serial.println(rxValues.crc);
      
    break;
    
    case BOUND_NO_VALUES:
      //Serial.print(newTime - time); Serial.println(" : ----"); // 32ms for 16Mhz
    break;

    case ERROR_SIGNAL_LOST:
	    //Serial.print(newTime - time); Serial.println(" : ----");
    break;
    
    default:
    break;
  }
  delay(2);
}
