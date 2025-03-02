#define CAN_CHIP_MCP2518 2518
#define CAN_CHIP_MCP2515 2515

// Select which CAN chip to use.  The new Snowball Creek shields(Rev 4) use the MCP2518(compatible with MCP2517)
// Earlier shields use the MCP2515
#define CAN_CHIP CAN_CHIP_MCP2518

// Set to 0 to not blink the builtin LED.
// On the Uno, the builtin LED uses the same line as the SPI SCK.
// This must be set to 0 on the Uno R4, as otherwise the SCK line
// will stay high all the time.  The Uno R3(AVR-based) doesn't
// need this to be set to 0, but it's probably a good idea.
#define BUILTIN_LED_BLINK 0

#if CAN_CHIP == CAN_CHIP_MCP2518
#include <ACAN2517.h>
#else if CAN_CHIP == CAN_CHIP_MCP1515
#include <ACAN2515.h>
#endif
#include <M95_EEPROM.h>
#include <lcc.h>
#include <lcc-common-internal.h>
#include <lcc-datagram.h>
#include <lcc-event.h>

static const byte MCP_CS  = 8 ; // CS input of CAN controller
static const byte MCP_INT =  2 ; // INT output of CAN controller
static const byte EEPROM_CS = 7;

// The CAN controller.  This example uses the ACAN2515 or ACAN2517 library from Pierre Molinaro:
// https://github.com/pierremolinaro/acan2515
// https://github.com/pierremolinaro/acan2517
#if CAN_CHIP == CAN_CHIP_MCP2518
ACAN2517 can (MCP_CS, SPI, MCP_INT) ;
#else if CAN_CHIP == CAN_CHIP_MCP1515
ACAN2515 can (MCP_CS, SPI, MCP_INT) ;
#endif

// EEPROM on the shield
M95_EEPROM eeprom(SPI, EEPROM_CS, 256, 3, true);

static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL ; // 16 MHz

static lcc_context* ctx;
CANMessage frame ;
struct lcc_can_frame lcc_frame;
unsigned long claim_alias_time;
static uint32_t gBlinkLedDate = 0 ;
int inputValue = 0;

const char cdi[] PROGMEM = { "<?xml version='1.0'?>"
};

struct id_page{
  uint64_t node_id;
  uint16_t id_version;
  char manufacturer[32];
  char part_number[21];
  char hw_version[12];
};

static void incoming_lcc_event(struct lcc_context* ctx, uint64_t event_id){
  if(event_id == 0x0101020000FF001Bll){
    // activate switch 10
    digitalWrite(5, 1);
  }else if(event_id == 0x0101020000FF001All){
    // Deactivate switch 10
    digitalWrite(5, 0);
  }else if(event_id == 0x0101020000FF001Dll){
    // Activate switch 11
    digitalWrite(6, 1);
  }else if(event_id == 0x0101020000FF001Cll){
    // Deactivate switch 11
    digitalWrite(6, 0);
  }
}

/**
 * This is a callback function that is called by liblcc in order to write a frame out to the CAN bus.
 */
int lcc_write(struct lcc_context*, struct lcc_can_frame* lcc_frame){
  frame.id = lcc_frame->can_id;
  frame.len = lcc_frame->can_len;
  frame.rtr = false;
  frame.ext = true;
  memcpy(frame.data, lcc_frame->data, 8);
  if(can.tryToSend (frame)){
    Serial.println(F("Send frame OK"));
    Serial.println(frame.id, HEX);
    return LCC_OK;
  }

  return LCC_ERROR_TX;
}

void setup () {
  Serial.begin (9600) ;
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }

  // Delay our startup.  The EEPROM seems to get into a bad state where it will not talk
  // if we come up and come down(which happens when flashing from Arduino IDE)
  delay(2000);

  SPI.begin () ;
  eeprom.begin();

  // Define a unique ID for your node.
  // Assuming we are using the Snowball Creek LCC Shield, we can just read
  // the unique ID from the ID page of the EEPROM
  // We will also set the manufacturer and other related information from the ID page
  struct id_page id;
  uint64_t unique_id;
  eeprom.read_id_page(sizeof(id), &id);
  unique_id = id.node_id;
  Serial.print("LCC ID: ");
  char buffer[20];
  lcc_node_id_to_dotted_format(unique_id, buffer, sizeof(buffer));
  Serial.print(buffer);
  Serial.println();
  Serial.print("ID page version: ");
  Serial.println(id.id_version);
  Serial.print("Manufacturer: ");
  Serial.println(id.manufacturer);
  Serial.print("Part number: ");
  Serial.println(id.part_number);
  Serial.print("HW Version: ");
  Serial.println(id.hw_version);

  // Create an LCC context that determines our communications
  ctx = lcc_context_new();

  // Set the unique identifier that this node will use
  lcc_context_set_unique_identifer(ctx, unique_id);

  // Set the callback function that will be called to write  frame out to the bus
  lcc_context_set_write_function(ctx, lcc_write, NULL);

  // Set simple node information that is handled by the 'simple node information protocol'
  lcc_context_set_simple_node_information(ctx,
                                        id.manufacturer,
                                        id.part_number,
                                        id.hw_version,
                                        "1.0");

  // Contexts:
  // * Datagram - allows transfers of datagrams to/from the device
  // * Event - event producer/consumer
  // * Memory -  memory read/writing on the node.  Requires a datagram context to exist
  // All contexts are owned by the parent lcc_context and are not free'd by the caller
  lcc_datagram_context_new(ctx);
  struct lcc_event_context* evt_ctx = lcc_event_new(ctx);

  // Listen for the specific events that we are interested in.
  // In this case, we will listen for events that correspond to throwing/closing
  // DCC accessory switches 11 and 12.
  lcc_event_add_event_consumed(evt_ctx, 0x0101020000FF001All);
  lcc_event_add_event_consumed(evt_ctx, 0x0101020000FF001Bll);
  lcc_event_add_event_consumed(evt_ctx, 0x0101020000FF001Cll);
  lcc_event_add_event_consumed(evt_ctx, 0x0101020000FF001Dll);

  // Add a handler function that will be called when an event that we are interested in
  // is received from the LCC bus
  lcc_event_set_incoming_event_function(evt_ctx, incoming_lcc_event);

  if(BUILTIN_LED_BLINK){
    pinMode (LED_BUILTIN, OUTPUT) ;
    digitalWrite (LED_BUILTIN, HIGH) ;
  }

  // The LEDs that correspond to the switches we are controlling
  // LED 5 corresponds to switch 10
  // LED 6 corresponds to switch 11
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

#if CAN_CHIP == CAN_CHIP_MCP2518
  ACAN2517Settings settings (ACAN2517Settings::OSC_40MHz_DIVIDED_BY_2, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
#else if CAN_CHIP == CAN_CHIP_MCP2515
  Serial.println("MCP2515");
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  settings.mReceiveBufferSize = 4;
  settings.mTransmitBuffer0Size = 8;
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

  // Generate an LCC alias and request it.
  // Note that generating an alias is a two-part step: you must generate the alias,
  // wait at least 200ms, and then claim the alias
  int val = lcc_context_generate_alias(ctx);
  if(val != LCC_OK){
    Serial.println(F("ERROR: Can't generate alias!"));
    while(1){}
  }
  
  claim_alias_time = millis() + 220;
}

void loop() {
  if (can.available ()) {
    // If we have an incoming CAN frame, turn it into an LCC frame and push it to liblcc
    can.receive (frame) ;
    lcc_frame.can_id = frame.id;
    lcc_frame.can_len = frame.len;
    memcpy(&lcc_frame.data, frame.data, 8);
    lcc_context_incoming_frame(ctx, &lcc_frame);
  }

  if (gBlinkLedDate < millis () && BUILTIN_LED_BLINK) {
    gBlinkLedDate += 1000 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }

  if(millis() >= claim_alias_time &&
    lcc_context_current_state(ctx) == LCC_STATE_INHIBITED){
    int stat = lcc_context_claim_alias(ctx);
    if(stat != LCC_OK){
      // If we were unable to claim our alias, we need to generate a new one and start over
      lcc_context_generate_alias(ctx);
      claim_alias_time = millis() + 220;
    }else{
      Serial.print(F("Claimed alias "));
      Serial.println(lcc_context_alias(ctx), HEX);
    }
  }
}
