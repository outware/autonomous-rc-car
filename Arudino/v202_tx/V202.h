#include "Arduino.h"

class nRF24;
class V202_TX {
  nRF24& radio;
  uint8_t txid[3];
  bool debug = true;
  bool debug_headers_written = false;
  bool packet_sent;
  uint8_t rf_ch_num;
public:
  uint8_t rf_channels[16];
  unsigned long prevTime = 0;
  unsigned long newTime = 0;
  V202_TX(nRF24& radio_) :
    radio(radio_)
  {}
  void setTXId(uint8_t txid_[3]);
  void begin();
  void command(uint8_t throttle, int8_t yaw, int8_t pitch, int8_t roll, uint8_t flags);
};

