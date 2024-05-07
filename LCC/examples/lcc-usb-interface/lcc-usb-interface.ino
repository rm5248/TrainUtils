/**
 * This is an example of using the Gridconnect transformation routines in LibLCC in order to create
 * a USB interface to LCC using an Arduino Uno.
 *
 * This code does not cause the Uno to count as a node on the LCC bus, as there is nothing on this
 * hardware that needs to be configured.
 */

// Updated 2024-05-06

#define CAN_CHIP_MCP2518 2518
#define CAN_CHIP_MCP2515 2515

// Select which CAN chip to use.  The new Snowball Creek shields(Rev 4) use the MCP2518(compatible with MCP2517)
// Earlier shields use the MCP2515
#define CAN_CHIP CAN_CHIP_MCP2515

#if CAN_CHIP == CAN_CHIP_MCP2518
#include <ACAN2517.h>
#else if CAN_CHIP == CAN_CHIP_MCP1515
#include <ACAN2515.h>
#endif

#include <lcc-gridconnect.h>

static const byte MCP_CS  = 8 ; // CS input of CAN controller
static const byte MCP_INT =  2 ; // INT output of CAN controller

// The CAN controller.  This example uses the ACAN2515 or ACAN2517 library from Pierre Molinaro:
// https://github.com/pierremolinaro/acan2515
// https://github.com/pierremolinaro/acan2517
#if CAN_CHIP == CAN_CHIP_MCP2518
ACAN2517 can (MCP_CS, SPI, MCP_INT) ;
#else if CAN_CHIP == CAN_CHIP_MCP1515
ACAN2515 can (MCP_CS, SPI, MCP_INT) ;
#endif
CANMessage frame;
struct lcc_can_frame lcc_frame;
struct lcc_can_frame lcc_frame_in;
char gridconnect_out[32];
char gridconnect_in[32];
int gridconnect_in_pos = 0;
unsigned long d5_off_millis = 0;
unsigned long blink_led_time = 0;
int blink_val = 0;

static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL ; // 16 MHz

/**
 * Write out the CAN frame to the bus
 */
void lcc_frame_write(struct lcc_can_frame* lcc_frame){
  frame.id = lcc_frame->can_id;
  frame.len = lcc_frame->can_len;
  frame.rtr = false;
  frame.ext = true;
  memcpy(frame.data, lcc_frame->data, 8);
  if(can.tryToSend (frame)){
    // Wink D5
    d5_off_millis = millis() + 20;
    digitalWrite(5, 1);
  }
}

void setup() {
  Serial.begin (115200) ;
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }

  // The LEDs on the board
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  SPI.begin();
#if CAN_CHIP == CAN_CHIP_MCP2518
  ACAN2517Settings settings (ACAN2517Settings::OSC_40MHz_DIVIDED_BY_2, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
#else if CAN_CHIP == CAN_CHIP_MCP2515
  Serial.println("MCP2515");
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  // For the computer interface, we need to increase our buffer sizes so that we can be assured we get all of the data
  settings.mReceiveBufferSize = 64;
  settings.mTransmitBuffer0Size = 16;
  // OpenLCB uses the following CAN propogation settings with the MCP2515:
  // CFN3 = 0x02
  // CFN2 = 0x90
  // CFN1 = 0x07
  settings.mPropagationSegment = 1;
  settings.mTripleSampling = false;
  settings.mPhaseSegment1 = 3;
  settings.mPhaseSegment2 = 3;
  settings.mSJW = 1;
  settings.mBitRatePrescaler = 8;
#endif

  const uint16_t errorCode = can.begin (settings, [] { can.isr () ; }) ;
  if (errorCode != 0) {
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (can.available ()) {
    // If we have an incoming CAN frame, turn it into an LCC frame and write it out to
    // our serial port as a gridconnect frame
    can.receive (frame) ;
    lcc_frame.can_id = frame.id;
    lcc_frame.can_len = frame.len;
    memcpy(&lcc_frame.data, frame.data, 8);

    if(lcc_canframe_to_gridconnect(&lcc_frame, gridconnect_out, sizeof(gridconnect_out)) == LCC_OK){
      Serial.print(gridconnect_out);
      Serial.print("\n\n");
    }
  }

  if(Serial.available()){
    int byte = Serial.read();

    // Serial.print("got: ");
    // Serial.println((char)byte);

    if(byte == ':'){
      gridconnect_in_pos = 0;
    }

    gridconnect_in[gridconnect_in_pos] = byte;
    gridconnect_in_pos++;

    if(gridconnect_in_pos > sizeof(gridconnect_in)){
      gridconnect_in_pos = 0;
    }

    if(byte == ';' && gridconnect_in_pos > 0){
      // We have a full frame.  Parse and send out to the CAN bus
      int stat = lcc_gridconnect_to_canframe(gridconnect_in, &lcc_frame_in);
      gridconnect_in_pos = 0;
      if(stat == LCC_OK){
        lcc_frame_write(&lcc_frame_in);
      }
    }else if(byte == ';'){
      gridconnect_in_pos = 0;
    }
  }

  if(currentMillis >= d5_off_millis){
    digitalWrite(5, 0);
  }

  if (currentMillis - blink_led_time >= 1000) {
    // save the last time you blinked the LED
    blink_led_time = currentMillis;

    digitalWrite(6, blink_val);
    blink_val = !blink_val;
  }
}
