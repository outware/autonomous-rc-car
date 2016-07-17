/* v202_protocol.cpp -- Handle the v202 protocol.
 *
 * Copyright (C) 2016 execuc
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "v202_protocol.h"

const uint16_t NORMAL_TIMEOUT = 11;
const uint8_t ERROR_JUMP_FREQ = 1;
const uint8_t ERROR_WAIT_PREV_FREQ = 2;
const uint8_t ERROR_WAIT_ONE_FREQ = 3;
const uint8_t NO_ERROR = 0;

/*****************************************************************/
/*	
    This portion of code is extracted from https://bitbucket.org/rivig/v202/src
    Thanks to rivig to his discovery and to share the v202 protocol.
*/

static uint8_t freq_hopping[][16] = {
 { 0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
   0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18 }, //  00
 { 0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
   0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10 }, //  01
 { 0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
   0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17 }, //  02
 { 0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
   0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F }  //  03
};

void v202Protocol::retrieveFrequency()
{
  uint8_t sum;
  sum = mTxid[0] + mTxid[1] + mTxid[2];
  // Base row is defined by lowest 2 bits
  uint8_t (&fh_row)[16] = freq_hopping[sum & 0x03];
  // Higher 3 bits define increment to corresponding row
  uint8_t increment = (sum & 0x1e) >> 2;
  for (int i = 0; i < 16; ++i) {
    uint8_t val = fh_row[i] + increment;
    // Strange avoidance of channels divisible by 16
    mRfChannels[i] = (val & 0x0f) ? val : val - 3;
  }
}
/*****************************************************************/

// This array stores 8 frequencies to test for the bind process
static uint8_t freq_test[8];

v202Protocol::v202Protocol()
{
  mTxid[0]=0;
  mTxid[1]=0;
  mTxid[2]=0;
  mTimeout = 9999;
  mErrorTimeoutCode = NO_ERROR;
  
  // Populate frequency for binding
  // 0x18 channel is present for the 4 frequencies hopping pattern
  // So create the 8 increments possible which is a combination
  // of the 3 higher bits of tx id.
  for(uint8_t i = 0; i < 8; ++i)
  {
    uint8_t freq = 0x18 + (i << 3);
    freq_test[i] = (freq & 0x0F) ? freq : freq - 3;
  }
  
  mState = NO_BIND;
  mLastSignalTime = 0;
  mRfChNum = 0;
}

v202Protocol::~v202Protocol()
{
  /*EMPTY*/
}

bool v202Protocol::checkCRC()
{  
  uint8_t sum = 0;
  for (uint8_t i = 0; i < 15; ++i)
    sum+= mFrame[i];
  return (sum == mFrame[15]);
}

void v202Protocol::init(nrf24l01p *wireless)
{
  mWireless = wireless;
  mWireless->init(16);
  delayMicroseconds(100);
  mWireless->rxMode(freq_test[0]);
  mLastSignalTime = millis();
}

// loop function, can be factorized (for later)
uint8_t v202Protocol::run( rx_values_t *rx_value )
{
  uint8_t returnValue = UNKNOWN;
  switch(mState)
  {
    case BOUND:
    {
      bool incrementChannel = false;
      returnValue = BOUND_NO_VALUES;
      unsigned long newTime = millis();
      if( mWireless->rxFlag() )
      {
        mWireless->resetRxFlag();
        
	      uint8_t lastCRC = 0;
        while ( !mWireless->rxEmpty() )
        {
          mWireless->readPayload(mFrame, 16);

          if( checkCRC() && checkTXaddr() )
          {
	          if(incrementChannel && lastCRC == mFrame[15])
			        continue;

        		if(!incrementChannel)
        		{
        		  mRfChNum++;
        		  if( mRfChNum > 15) 
        		    mRfChNum = 0;
        		  mWireless->switchFreq(mRfChannels[mRfChNum]);
        		 incrementChannel =true;
        		 mTimeout = NORMAL_TIMEOUT;
        		 mErrorTimeoutCode = NO_ERROR;
        		}

		        lastCRC=mFrame[15];

    		    //Serial.println(newTime - mLastSignalTime);
    		    mLastSignalTime = newTime;
    
    		    // a valid frame has been received
    		    //incrementChannel = true;
    		    // Discard bind frame
    		    if( mFrame[14] != 0xc0 )
    		    {
    		      // Extract values
    		      returnValue = BOUND_NEW_VALUES;
    		      rx_value->throttle = mFrame[0];
    		      rx_value->yaw = mFrame[1] < 0x80 ? -mFrame[1] :  mFrame[1] - 0x80;
    		      rx_value->pitch = mFrame[2] < 0x80 ? -mFrame[2] :  mFrame[2] - 0x80;
    		      rx_value->roll = mFrame[3] < 0x80 ? -mFrame[3] :  mFrame[3] - 0x80;
    		      rx_value->trim_yaw = mFrame[4] - 0x40;
    		      rx_value->trim_pitch = mFrame[5] - 0x40;
    		      rx_value->trim_roll = mFrame[6] - 0x40;
              // TX id
              rx_value->frame7 = mFrame[7];
              rx_value->frame8 = mFrame[8];
              rx_value->frame9 = mFrame[9];
              // Unknown
              rx_value->frame10 = mFrame[10];
              rx_value->frame11 = mFrame[11];
              rx_value->frame12 = mFrame[12];
              rx_value->frame13 = mFrame[13];
              // Flags
    		      rx_value->flags = mFrame[14];
    		      rx_value->crc = mFrame[15];
    		    }

          }

        }
      }

      
      if(incrementChannel == false && uint16_t(newTime - mLastSignalTime) > mTimeout)
      {
      	mErrorTimeoutCode++;

      	mLastSignalTime = millis();
        Serial.print("e 0x0");Serial.print(String(mRfChNum, HEX));Serial.print(" ");Serial.println(newTime - mLastSignalTime);
      	uint8_t freq_jump =0;
      	if(mErrorTimeoutCode == ERROR_JUMP_FREQ)
      	{
      		freq_jump  = uint16_t(newTime - mLastSignalTime) / 8 + 1;
      		mTimeout = freq_jump * 8 + 6;
		      Serial.print("1 ");
      	}
      	else if(mErrorTimeoutCode == ERROR_WAIT_PREV_FREQ)
      	{
      		freq_jump  = 10;
      		mTimeout = 120;
		      Serial.print("2 ");
      	}
      	else //if(mErrorTimeoutCode == ERROR_WAIT_ONE_FREQ)
      	{
      		freq_jump = random(1, 15);
      		mTimeout = 250;
		      Serial.print("3 ");
      	}
        

         mRfChNum+=freq_jump;
          if( mRfChNum > 15) 
            mRfChNum = mRfChNum % 16;
          mWireless->switchFreq(mRfChannels[mRfChNum]);

	   Serial.print("0x0");Serial.print(String(mRfChNum,HEX));Serial.print(" ");Serial.println(mTimeout);
      
      }

      if(mErrorTimeoutCode > 0)
        returnValue = ERROR_SIGNAL_LOST;

    
    }
    break;
    // Initial state
    case NO_BIND:
    {
      returnValue = NOT_BOUND;
      unsigned long newTime = millis();
 
      if( !mWireless->rxFlag() )
      {
        // Wait 128ms before switching the frequency
        // 128ms is th time to a TX to emits on all this frequency
        if((newTime - mLastSignalTime) > 128)
        {
          mRfChNum++;
          if( mRfChNum > 7) 
            mRfChNum = 0;
          mWireless->switchFreq(freq_test[mRfChNum]);
          mLastSignalTime = newTime;
        }
      }
      else 
      {
        mWireless->resetRxFlag();
        bool bFrameOk = false;
        while ( !mWireless->rxEmpty() )
        {
          mWireless->readPayload(mFrame, 16);
                           
          if( checkCRC() && mFrame[14] == 0xC0 )
          {
            // Bind frame is OK
            mTxid[0] = mFrame[7];
            mTxid[1] = mFrame[8];
            mTxid[2] = mFrame[9];
            Serial.print("\t mTxid:");
            Serial.print(mTxid[0]);
            Serial.print("\t");
            Serial.print(mTxid[1]);
            Serial.print("\t");
            Serial.println(mTxid[2]);

            // Create TX frequency array
            retrieveFrequency();
            mRfChNum = 0;
            mWireless->switchFreq(mRfChannels[mRfChNum]);
            mLastSignalTime = newTime;  
            mState = WAIT_FIRST_SYNCHRO;
            mWireless->flushRx();
            returnValue = BIND_IN_PROGRESS;
            break;
          }
        }
      }
    }
    break;
    
    // Wait on the first frequency of TX
    case WAIT_FIRST_SYNCHRO:
      returnValue = BIND_IN_PROGRESS;
      if( mWireless->rxFlag() )
      {
        mWireless->resetRxFlag();
        bool incrementChannel = false;
        while ( !mWireless->rxEmpty() )
        {
          mWireless->readPayload(mFrame, 16);
          if( checkCRC() && mFrame[14] != 0xc0 && checkTXaddr())
          {
            incrementChannel = true;
            mState = BOUND;
          }
        }
        
        if(incrementChannel)
        {
          // switch channel
          mRfChNum++;
          if( mRfChNum > 15) 
            mRfChNum = 0;
          mWireless->switchFreq(mRfChannels[mRfChNum]);
        }
      }
    break;
    // Not implement for the moment
    case SIGNAL_LOST:
      returnValue = ERROR_SIGNAL_LOST;
    break;
    
    default:
    break;
  }
  
  return returnValue;
}


