#include <ACAN2517.h>
#include <M95_EEPROM.h>
#include <lcc.h>
#include <lcc-common-internal.h>
#include <lcc-datagram.h>
#include <lcc-event.h>

// Updated 2024-03-19

static const byte MCP2517_CS  = 8 ; // CS input of MCP2517
static const byte MCP2517_INT =  2 ; // INT output of MCP2517
static const byte EEPROM_CS = 7;

// The CAN controller.  This example uses the ACAN2515 library from Pierre Molinaro:
// https://github.com/pierremolinaro/acan2515
ACAN2517 can (MCP2517_CS, SPI, MCP2517_INT) ;

// EEPROM on the shield
M95_EEPROM eeprom(SPI, EEPROM_CS, 256, 3, true);

static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL ; // 16 MHz

static lcc_context* ctx;
CANMessage frame ;
struct lcc_can_frame lcc_frame;
unsigned long claim_alias_time;
static uint32_t gBlinkLedDate = 0 ;
int inputValue = 0;

struct id_page{
  uint64_t node_id;
  uint16_t id_version;
  char manufacturer[32];
  char part_number[21];
  char hw_version[12];
};

void print_liblcc_version(){
  uint32_t lib_version = lcc_library_version();

  Serial.print("LibLCC version: ");
  Serial.print(LCC_VERSION_MAJOR(lib_version));
  Serial.print(".");
  Serial.print(LCC_VERSION_MINOR(lib_version));
  Serial.print(".");
  Serial.print(LCC_VERSION_MICRO(lib_version));
  Serial.println();
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

int lcc_buffer_size(struct lcc_context* ctx){
  return can.driverTransmitBufferSize() - can.driverTransmitBufferCount();
}

void setup () {
  struct id_page id;

  Serial.begin (9600) ;
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }

  // First let's print out the version of LibLCC
  print_liblcc_version();

  // Delay our startup.  The EEPROM seems to get into a bad state where it will not talk
  // if we come up and come down(which happens when flashing from Arduino IDE)
  delay(2000);

  SPI.begin () ;
  eeprom.begin();

  // Define a unique ID for your node.
  // Assuming we are using the Snowball Creek LCC Shield, we can just read
  // the unique ID from the ID page of the EEPROM
  // We will also set the manufacturer and other related information from the ID page
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
  // Also give it a callback function that tells LibLCC how many frames can be buffered
  lcc_context_set_write_function(ctx, lcc_write, lcc_buffer_size);

  // Set simple node information that is handled by the 'simple node information protocol'
  lcc_context_set_simple_node_information(ctx,
                                        id.manufacturer,
                                        id.part_number,
                                        id.hw_version,
                                        "1.0");

  // Optional: create other contexts to handle other parts of LCC communication
  // Contexts:
  // * Datagram - allows transfers of datagrams to/from the device
  // * Event - event producer/consumer
  // * Memory -  memory read/writing on the node.  Requires a datagram context to exist
  // All contexts are owned by the parent lcc_context and are not free'd by the caller
  lcc_datagram_context_new(ctx);
  struct lcc_event_context* evt_ctx = lcc_event_new(ctx);

  uint64_t event_id = unique_id << 16;
  lcc_event_add_event_produced(evt_ctx, event_id);
  lcc_event_add_event_produced(evt_ctx, event_id + 1);
  lcc_event_add_event_produced(evt_ctx, event_id + 2);


  pinMode (LED_BUILTIN, OUTPUT) ;
  digitalWrite (LED_BUILTIN, HIGH) ;

  // This particular code uses the SparkFun CAN-BUS shield, where pins 7 and 8
  // are LED outputs.
  // Pin 4 is used as a sample digital input that will generate LCC events when it
  // changes state.  Make sure to put a pull-down on this pin, and you can then trigger
  // events by connecting and disconnecting it from the +5v rail.
  pinMode(4, INPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  Serial.println ("Configure ACAN2517") ;
  ACAN2517Settings settings (QUARTZ_FREQUENCY, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
  // settings.mRequestedMode = ACAN2517Settings::NormalMode;
  // We need to lower the transmit and receive buffer size(at least on the Uno), as otherwise
  // the ACAN2515 library will allocate too much memory
  // settings.mReceiveBufferSize = 4;
  // settings.mTransmitBuffer0Size = 8;
  // OpenLCB uses the following CAN propogation settings with the MCP2515:
  // CFN3 = 0x02
  // CFN2 = 0x90
  // CFN1 = 0x07
  // settings.mPropagationSegment = 1;
  settings.mPhaseSegment1 = 3;
  settings.mPhaseSegment2 = 3;
  settings.mSJW = 1;
  // settings.mTripleSampling = false;
  settings.mBitRatePrescaler = 8;
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

  if (gBlinkLedDate < millis ()) {
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

  // Read the value of pin 4 - if it changes state, send an event
  int currentVal = !!digitalRead(4);
  if(currentVal != inputValue){
    inputValue = currentVal;
    uint64_t event_id = lcc_context_unique_id(ctx) << 16llu;

    if(currentVal == 0){
      lcc_event_produce_event(lcc_context_get_event_context(ctx), event_id);
    }else{
      lcc_event_produce_event(lcc_context_get_event_context(ctx), event_id + 1);
    }

    // Light up LED with current status
    digitalWrite(7, currentVal);
  }
}
