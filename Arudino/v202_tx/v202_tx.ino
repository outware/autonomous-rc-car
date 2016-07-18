#include <SPI.h>
#include "nRF24L01.h"
#include "V202.h"

#define CE_PIN  9
#define CSN_PIN 53

#if 0
uint8_t txid[3] = { 0xcd, 0x31, 0x71 };
uint8_t rf_channels[16] = { 0x25, 0x2A, 0x1A, 0x3C, 0x37, 0x2B, 0x2E, 0x1D,
                            0x1B, 0x2D, 0x24, 0x3B, 0x13, 0x29, 0x23, 0x22 };
#endif
#if 0
uint8_t txid[3] = { 0x3e, 0x6a, 0xaa };
uint8_t rf_channels[16] = { 0x15, 0x1E, 0x39, 0x28, 0x2C, 0x1C, 0x29, 0x2E,
                            0x36, 0x2D, 0x18, 0x2B, 0x3A, 0x38, 0x1D, 0x1B };
#endif
#if 0
uint8_t txid[3] = { 0xcd, 0x31, 0x72 };
uint8_t rf_channels[16] = { 0x2b, 0x1f, 0x3d, 0x2c, 0x28, 0x26, 0x32, 0x3a,
                            0x1d, 0x25, 0x2d, 0x18, 0x22, 0x16, 0x31, 0x1c };
#endif

//Our transmitter ID (WLToys A9x9 car)
#if 1
uint8_t txid[3] = { 0xa7, 0x59, 0x03 };
#endif

//Blank transmitter ID
//uint8_t txid[3] = { 0x00, 0x00, 0x00 };

nRF24 radio(CE_PIN, CSN_PIN);
V202_TX tx(radio);

bool debug = true;

uint8_t throttle, flags;
int8_t yaw, pitch, roll; //NOTE: Naming based on the drone equivalent channel use

int a0, a1, a2, a3;
int a0min, a0max;
int a1min, a1max;
int a2min, a2max;
int a3min, a3max;

void calibrate()
{
  a0min = 150; a0max=600;
  a1min = 150; a0max=600;
  a2min = 150; a0max=600;
  a3min = 150; a0max=600;
}

void initInput()
{
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  calibrate();
}

bool readInput()
{
  bool changed = false;
  long a;
  a = analogRead(A0);
  if (a < a0min) a0min = a;
  if (a > a0max) a0max = a;
  a = (a-a0min)*255/(a0max-a0min);
  if (a != a0) { changed = true; a0 = a; }

  a = analogRead(A1);
  if (a < a1min) a1min = a;
  if (a > a1max) a1max = a;
  a = (a-a1min)*255/(a1max-a1min);
  if (a != a1) { changed = true; a1 = a; }

  a = analogRead(A2);
  if (a < a2min) a2min = a;
  if (a > a2max) a2max = a;
  a = (a2max-a)*255/(a2max-a2min);
  if (a != a2) { changed = true; a2 = a; }

  a = analogRead(A3);
  if (a < a3min) a3min = a;
  if (a > a3max) a3max = a;
  a = (a3max-a)*255/(a3max-a3min);
  if (a != a3) { changed = true; a3 = a; }
  return changed;
}

void setup() 
{
  initInput();
  readInput();
  Serial.begin(115200);
  Serial.print("---- Arduino 2.4ghz v202 protocol TRANSMITTER ----");
  tx.setTXId(txid);
  Serial.print("Setting up TX. Transmitter ID (txid): {");
  Serial.print(txid[0]); Serial.print(", ");
  Serial.print(txid[1]); Serial.print(", ");
  Serial.print(txid[2]); Serial.println("}");
  
  Serial.print("Setting up TX. Calculated frequencies: {");
  int i;
  for (i = 0; i < sizeof(tx.rf_channels) - 1; i++){
      Serial.print("0x"); Serial.print(String(tx.rf_channels[i],HEX));
      //Serial.print(tx.rf_channels[i]);
      if (i < sizeof(tx.rf_channels) - 2){
        Serial.print(", ");
      }
  }
  Serial.println("}");
  tx.begin();
  throttle = 0; yaw = 0; pitch = 0; roll = 0; flags = 0;

  Serial.write("Reading status\n");
  uint8_t res = radio.read_register(STATUS);
  Serial.write("Result: ");
  Serial.print(res);
  Serial.write("\n");
} 

int counter = 0;
int direction = 1;
bool bind = true;
bool calibrated = false;
void loop() 
{
  //bool changed = readInput();
  if (false) {
//  if (changed) {
    Serial.write("sticks: ");
    Serial.print(a0); Serial.write(" ");
    Serial.print(a1); Serial.write(" ");
    Serial.print(a2); Serial.write(" ");
    Serial.print(a3); Serial.write("\n");
  }
  if (bind) {
    throttle = a0;
    flags = 0xc0;
    // Auto bind in 2.5 sec after turning on
    counter += direction;
    if (direction > 0) {
      if (counter > 256) direction = -1;
    } else {
      if (counter < 0) {
        direction = 1;
        counter = 0;
        bind = false;
        flags = 0;
        Serial.write("Bound\n");
      }
    } 
    if (direction > 0) {
      if (throttle >= 255) direction = -1;
    } else {
      if (throttle == 0) {
        direction = 1;
        counter = 0;
        bind = false;
        flags = 0;
        Serial.write("Bound\n");
        Serial.write("a0min "); Serial.print(a0min);
        Serial.write(" a0max "); Serial.print(a0max);
        Serial.write("\na1min "); Serial.print(a1min);
        Serial.write(" a1max "); Serial.print(a1max);
        Serial.write("\na2min "); Serial.print(a2min);
        Serial.write(" a2max "); Serial.print(a2max);
        Serial.write("\na3min "); Serial.print(a3min);
        Serial.write(" a3max "); Serial.print(a3max);
      }
    }
  } else {
    throttle = a0;
    yaw = a1 < 0x80 ? 0x7f - a1 : a1;
    roll = a2 < 0x80 ? 0x7f - a2 : a2 ;
    pitch = a3 < 0x80 ? 0x7f - a3 : a3;

    // Blinking LED lights
    counter += direction;
    if (direction > 0) {
      if (counter > 255) {
        direction = -1;
        counter = 255;
        flags = 0x10;
      }
    } else {
      if (counter < 0) {
        direction = 1;
        counter = 0;
        flags = 0x00;
      }
    }
    //
  }
  throttle = 0; //N/A on RC car
  yaw = 0; //Steering on RC car
  pitch = 20; //Throttle on RC car
  roll = 0; //Not used on RC car
  //flags = 0x10;
  flags = 0x00;

  tx.command(throttle, yaw, pitch, roll, flags);

  if (debug) {
    Serial.write("Throttle: "); Serial.print(throttle);
    Serial.write(" Yaw: "); Serial.print(yaw);
    Serial.write(" Pitch: "); Serial.print(pitch);
    Serial.write(" Roll: "); Serial.print(roll);
    Serial.write(" Flags: "); Serial.println(flags);
  }
  
  delay(10);
}

