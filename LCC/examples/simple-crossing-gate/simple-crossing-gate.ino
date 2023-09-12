/**
 * This is an example of a simple crossing gate control.
 *
 * Utlizing 4 inputs, we can determine where a train is in relation to the crossing and
 * set an output appropriately.
 * The status of the crossing gates is also sent to the LCC bus in order to give feedback
 * to the user(and can be used by other LCC connected nodes).
 *
 * This example handles just a single track crossing.
 */
#include <ACAN2515.h>
#include <lcc.h>
#include <lcc-common-internal.h>
#include <lcc-datagram.h>
#include <lcc-event.h>
#include <lcc-memory.h>

const char cdi[] PROGMEM = { "<?xml version=\"1.0\"?>\
<cdi\
    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://openlcb.org/schema/cdi/1/1/cdi.xsd\">\
    <identification>\
        <manufacturer>Snowball Creek</manufacturer>\
        <model>Crossing Gate Controller</model>\
        <hardwareVersion>1.0</hardwareVersion>\
        <softwareVersion>0.1</softwareVersion>\
    </identification>\
    <acdi/>\
        <segment space='251'>\
            <name>Node ID</name>\
            <group>\
                <name>Your name and description for this node</name>\
                <string size='63'>\
                    <name>Node Name</name>\
                </string>\
                <string size='64' offset='1'>\
                    <name>Node Description</name>\
                </string>\
            </group>\
        </segment>\
</cdi>" };

enum TrackState{
  TRACK_UNOCCUPIED,
  PRE_ISLAND_OCCUPIED,
  ISLAND_OCCUPIED_INCOMING,
  ISLAND_OCCUPIED,
  POST_ISLAND_OCCUPIED_INCOMING,
  POST_ISLAND_OCCUPIED,
};

enum Direction{
  DIRECTION_UNKNOWN,
  DIRECTION_LTR,
  DIRECTION_RTL,
};

static const byte MCP2515_CS  = 9 ; // CS input of MCP2515 (adapt to your design) 
static const byte MCP2515_INT =  2 ; // INT output of MCP2515 (adapt to your design)
static const byte EEPROM_CS = 10;
static const byte CROSSING_OCCUPIED_OUTPUT = 5;

static const byte EEPROM_WRITE_ENABLE = 0x6;
static const byte EEPROM_READ_STATUS_REGISTER = 0x5;
static const byte EEPROM_WRITE_STATUS_REGISTER = 0x1;
static const byte EEPROM_READ_MEMORY_ARRAY = 0x3;
static const byte EEPROM_WRITE_MEMORY_ARRAY = 0x2;
static const byte EEPROM_WRITE_DISABLE = 0x4;

static const int LCC_UNIQUE_ID_ADDR = 0x6000;

// The CAN controller.  This example uses the ACAN2515 library from Pierre Molinaro:
// https://github.com/pierremolinaro/acan2515
ACAN2515 can (MCP2515_CS, SPI, MCP2515_INT) ;

static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL ; // 16 MHz

static lcc_context* ctx;
CANMessage frame ;
struct lcc_can_frame lcc_frame;
unsigned long claim_alias_time;
unsigned long blink_led_time = 0;
unsigned long timeout_millis = 25000;
unsigned long incoming_train_time = 0;
int blink_val = 0;
enum TrackState current_state = TRACK_UNOCCUPIED;
enum Direction current_direction = DIRECTION_UNKNOWN;
uint64_t crossing_active_event = 0;
uint64_t crossing_inactive_event = 0;

void eeprom_read(int offset, void* data, int numBytes){
  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_READ_MEMORY_ARRAY);

  SPI.transfer((offset & 0xFF00) >> 8);
  SPI.transfer((offset & 0x00FF) >> 0);

  uint8_t* u8_data = data;
  while(numBytes > 0){
    numBytes--;
    *u8_data = SPI.transfer(0xFF); // dummy byte
    u8_data++;
  }
  Serial.println();

  digitalWrite(EEPROM_CS, HIGH);
}

void eeprom_write(int offset, void* data, int numBytes){
  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_WRITE_ENABLE);
  digitalWrite(EEPROM_CS, HIGH);

  delay(5);

  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_WRITE_MEMORY_ARRAY);
  SPI.transfer((offset & 0xFF00) >> 8);
  SPI.transfer((offset & 0x00FF) >> 0);

  uint8_t* u8_data = data;
  while(numBytes > 0){
    numBytes--;
    SPI.transfer(*u8_data); // data byte
    u8_data++;
  }

  digitalWrite(EEPROM_CS, HIGH);
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
    return LCC_OK;
  }

  Serial.println("Unable to transmit frame");
  return LCC_ERROR_TX;
}

void handle_ltr(int left_input, int left_island_input, int right_island_input, int right_input){
  if(left_island_input == 1 && 
    current_state == PRE_ISLAND_OCCUPIED){
    current_state = ISLAND_OCCUPIED_INCOMING;
    Serial.println(F("Island occupied incoming"));
  }else if(right_island_input == 1 &&
    current_state == ISLAND_OCCUPIED_INCOMING){
    current_state = ISLAND_OCCUPIED;
    Serial.println(F("island occupied"));
  }else if(right_island_input == 0 &&
    current_state == ISLAND_OCCUPIED){
    current_state = POST_ISLAND_OCCUPIED_INCOMING;
    Serial.println(F("post island occupied incoming"));
    set_output_inactive();
  }else if(right_input == 1 &&
    current_state == POST_ISLAND_OCCUPIED_INCOMING){
    current_state = POST_ISLAND_OCCUPIED;
    Serial.println(F("post island occupied"));
  }else if(right_input == 0 &&
    current_state == POST_ISLAND_OCCUPIED){
    Serial.println(F("train out LTR"));
    current_state = TRACK_UNOCCUPIED;
    current_direction = DIRECTION_UNKNOWN;
  }
}

void handle_rtl(int left_input, int left_island_input, int right_island_input, int right_input){
  if(right_island_input == 1 && 
    current_state == PRE_ISLAND_OCCUPIED){
    current_state = ISLAND_OCCUPIED_INCOMING;
    Serial.println(F("Island occupied incoming"));
  }else if(left_island_input == 1 &&
    current_state == ISLAND_OCCUPIED_INCOMING){
    current_state = ISLAND_OCCUPIED;
    Serial.println(F("island occupied"));
  }else if(left_island_input == 0 &&
    current_state == ISLAND_OCCUPIED){
    current_state = POST_ISLAND_OCCUPIED_INCOMING;
    Serial.println(F("post island occupied incoming"));
    set_output_inactive();
  }else if(left_input == 1 &&
    current_state == POST_ISLAND_OCCUPIED_INCOMING){
    current_state = POST_ISLAND_OCCUPIED;
    Serial.println(F("post island occupied"));
  }else if(left_input == 0 &&
    current_state == POST_ISLAND_OCCUPIED){
    Serial.println(F("train out RTL"));
    current_state = TRACK_UNOCCUPIED;
    current_direction = DIRECTION_UNKNOWN;
  }
}

void set_output_active(){
  // A train has come in - set our output to be active
  digitalWrite(CROSSING_OCCUPIED_OUTPUT, 1);

  // Send the event to notify the LCC bus
  struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
  lcc_event_produce_event(evt_ctx, crossing_active_event);
}

void set_output_inactive(){
  // A train has left - set our output to be inactive
  digitalWrite(CROSSING_OCCUPIED_OUTPUT, 0);

  // Send the event to notify the LCC bus
  struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
  lcc_event_produce_event(evt_ctx, crossing_inactive_event);
}

void check_for_train(){
  // Assume pins are LOW by default
  int left_input = digitalRead(A0);
  int left_island_input = digitalRead(A1);
  int right_island_input = digitalRead(A2);
  int right_input = digitalRead(A3);
  unsigned long millis_diff = millis() - incoming_train_time;

  if(left_input == 1 && current_state == TRACK_UNOCCUPIED){
    // Incoming train, left to right
    current_state = PRE_ISLAND_OCCUPIED;
    current_direction = DIRECTION_LTR;
    incoming_train_time = millis();
    Serial.println(F("Incoming train LTR"));
    set_output_active();
    return;
  }else if(right_input == 1 && current_state == TRACK_UNOCCUPIED){
    // Incoming train, right to left
    current_state = PRE_ISLAND_OCCUPIED;
    current_direction = DIRECTION_RTL;
    incoming_train_time = millis();
    Serial.println(F("Incoming train RTL"));
    set_output_active();
    return;
  }

  if(current_direction == DIRECTION_LTR){
    handle_ltr(left_input, left_island_input, right_island_input, right_input);
  }else if(current_direction == DIRECTION_RTL){
    handle_rtl(left_input, left_island_input, right_island_input, right_input);
  }

  if(millis_diff > timeout_millis &&
    current_state != TRACK_UNOCCUPIED){
    current_state = TRACK_UNOCCUPIED;
    current_direction = DIRECTION_UNKNOWN;
    set_output_inactive();
  }
}

//
// Memory read/write
//

void mem_address_space_information_query(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space){
  if(address_space == 251){
    lcc_memory_respond_information_query(ctx, alias, 1, address_space, 64 + 64, 0, 0);
  }else{
    // This memory space does not exist: return an error
    lcc_memory_respond_information_query(ctx, alias, 0, address_space, 0, 0, 0);
  }
}

void mem_address_space_read(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint8_t read_count){
  if(address_space != 251){
    lcc_memory_respond_read_reply_fail(ctx, alias, address_space, 0, 0, NULL);
    return;
  }

  uint8_t buffer[64];
  eeprom_read(starting_address, buffer, read_count);

  // For any blank data, we will read 0xFF
  // In this example, we know that we have all strings, so replace all 0xFF with 0x00
  for(int x = 0; x < sizeof(buffer); x++){
    if(buffer[x] == 0xFF){
      buffer[x] = 0x00;
    }
  }

  lcc_memory_respond_read_reply_ok(ctx, alias, address_space, starting_address, buffer, read_count);
}

void mem_address_space_write(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* data, int data_len){
  if(address_space != 251){
    lcc_memory_respond_write_reply_fail(ctx, alias, address_space, starting_address, 0, NULL);
    return;
  }

  eeprom_write(starting_address, data, data_len);

  lcc_memory_respond_write_reply_ok(ctx, alias, address_space, starting_address);
}

//
// Standard Arduino functions
//

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin (9600) ;
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }

  SPI.begin();

  // Output LED to tell us if the track is occupied
  pinMode(CROSSING_OCCUPIED_OUTPUT, OUTPUT);

  // Input sensors for the track
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);

  // Heartbeat LED
  pinMode(6, OUTPUT);

  // Define a unique ID for your node.  The generation of this unique ID can be
  // found in the LCC specifications, specifically the unique identifiers standard
  uint64_t unique_id;
  eeprom_read(LCC_UNIQUE_ID_ADDR, &unique_id, 8);

  // Create an LCC context that determines our communications
  ctx = lcc_context_new();

  // Set the unique identifier that this node will use
  lcc_context_set_unique_identifer(ctx, unique_id);

  // Set the callback function that will be called to write  frame out to the bus
  lcc_context_set_write_function(ctx, lcc_write);

  // Set simple node information that is handled by the 'simple node information protocol'
  lcc_context_set_simple_node_information(ctx,
                                        "Snowball Creek",
                                        "Simple Crossing",
                                        "1.0",
                                        "0.1");

  // Optional: create other contexts to handle other parts of LCC communication
  // Contexts:
  // * Datagram - allows transfers of datagrams to/from the device
  // * Event - event producer/consumer
  // * Memory -  memory read/writing on the node.  Requires a datagram context to exist
  // All contexts are owned by the parent lcc_context and are not free'd by the caller
  // lcc_datagram_context_new(ctx);
  struct lcc_event_context* evt_ctx = lcc_event_new(ctx);
  lcc_datagram_context_new(ctx);
  struct lcc_memory_context* mem_ctx = lcc_memory_new(ctx);

  lcc_memory_set_cdi(mem_ctx, cdi, sizeof(cdi), LCC_MEMORY_CDI_FLAG_ARDUINO_PROGMEM);
  lcc_memory_set_memory_functions(mem_ctx, 
    mem_address_space_information_query,
    mem_address_space_read,
    mem_address_space_write);

  uint64_t event_id = unique_id << 16;
  crossing_active_event = event_id;
  crossing_inactive_event = event_id + 1;
  lcc_event_add_event_produced(evt_ctx, event_id);
  lcc_event_add_event_produced(evt_ctx, event_id + 1);

  ACAN2515Settings settings (QUARTZ_FREQUENCY, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  // We need to lower the transmit and receive buffer size(at least on the Uno), as otherwise
  // the ACAN2515 library will allocate too much memory
  settings.mReceiveBufferSize = 4;
  settings.mTransmitBuffer0Size = 10;
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
  unsigned long currentMillis = millis();

  if (can.available ()) {
    // If we have an incoming CAN frame, turn it into an LCC frame and push it to liblcc
    can.receive (frame) ;
    lcc_frame.can_id = frame.id;
    lcc_frame.can_len = frame.len;
    memcpy(&lcc_frame.data, frame.data, 8);
    lcc_context_incoming_frame(ctx, &lcc_frame);
  }

  if(currentMillis >= claim_alias_time &&
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

  if(lcc_context_current_state(ctx) != LCC_STATE_INHIBITED){
    check_for_train();
  }

  if ((currentMillis - blink_led_time) >= 1000) {
    // save the last time you blinked the LED
    blink_led_time = currentMillis;

    digitalWrite(6, blink_val);
    blink_val = !blink_val;
  }
}
