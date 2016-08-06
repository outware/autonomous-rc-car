typedef struct __attribute__((__packed__)) {
  uint8_t throttle;
  int8_t yaw;
  int8_t pitch;
  int8_t roll;
  int8_t trim_yaw;
  int8_t trim_pitch;
  int8_t trim_roll;
  uint8_t frame7;
  uint8_t frame8;
  uint8_t frame9;
  int8_t frame10;
  int8_t frame11;
  int8_t frame12;
  int8_t frame13;
  uint8_t flags;
  uint8_t crc;
} rx_values_t;
