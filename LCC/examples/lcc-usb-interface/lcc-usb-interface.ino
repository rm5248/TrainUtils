/**
 * This is an example of using the Gridconnect transformation routines in LibLCC in order to create
 * a USB interface to LCC using an Arduino Uno.
 *
 * This code does not cause the Uno to count as a node on the LCC bus, as there is nothing on this
 * hardware that needs to be configured.
 */

#include <ACAN2515.h>

#include <lcc-gridconnect.h>

static const byte MCP2515_CS  = 9 ; // CS input of MCP2515 (adapt to your design) 
static const byte MCP2515_INT =  2 ; // INT output of MCP2515 (adapt to your design)

// The CAN controller.  This example uses the ACAN2515 library from Pierre Molinaro:
// https://github.com/pierremolinaro/acan2515
ACAN2515 can (MCP2515_CS, SPI, MCP2515_INT) ;
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
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  // For the computer interface, we need to increase our buffer sizes so that we can be assured we get all of the data
  settings.mReceiveBufferSize = 16;
  settings.mTransmitBuffer0Size = 16;
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
      Serial.println(gridconnect_out);
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

    if(byte == ';' && byte > 0){
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

  if(millis() >= d5_off_millis){
    digitalWrite(5, 0);
  }

  if (currentMillis - blink_led_time >= 1000) {
    // save the last time you blinked the LED
    blink_led_time = currentMillis;

    digitalWrite(6, blink_val);
    blink_val = !blink_val;
  }
}
