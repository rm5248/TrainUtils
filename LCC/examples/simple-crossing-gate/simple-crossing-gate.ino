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
#include <M95_EEPROM.h>
#include <lcc.h>
#include <lcc-common-internal.h>
#include <lcc-datagram.h>
#include <lcc-event.h>
#include <lcc-memory.h>

// Segment spaces:
// 0xFF = 255 = CDI
// 0xFE = 254 = 'all memory'
// 0xFD = 253 = basic config space
const char cdi[] PROGMEM = { "<?xml version='1.0'?> \
<cdi xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='http://openlcb.org/schema/cdi/1/1/cdi.xsd'> \
<identification> \
<manufacturer>Snowball Creek</manufacturer> \
<model>Crossing Gate Controller</model> \
<hardwareVersion>1.0</hardwareVersion> \
<softwareVersion>0.1</softwareVersion> \
</identification> \
<acdi/> \
<segment space='253'> \
<name>Occupation Events</name> \
<group> \
<name>Occupation events(left-to-right)</name> \
<eventid> \
<name>Occupation pre-island LTR event</name> \
<description>When a train enters the block before the island(left-to-right)</description> \
</eventid> \
<eventid> \
<name>Occupation island LTR event</name> \
<description>When a train enters the island(left-to-right)</description> \
</eventid> \
<eventid> \
<name>Occupation post-island LTR event</name> \
<description>When a train enters the leaves(left-to-right)</description> \
</eventid> \
<eventid> \
<name>Occupation Unoccupied LTR event</name> \
<description>When track is now unoccupied, this event is sent(left-to-right)</description> \
</eventid> \
</group> \
<group> \
<name>Occupation events(right-to-left)</name> \
<eventid> \
<name>Occupation pre-island RTL event</name> \
<description>When a train enters the block before the island(right-to-left)</description> \
</eventid> \
<eventid> \
<name>Occupation island RTL event</name> \
<description>When a train enters the island(right-to-left)</description> \
</eventid> \
<eventid> \
<name>Occupation post-island RTL event</name> \
<description>When a train enters the leaves(right-to-left)</description> \
</eventid> \
<eventid> \
<name>Occupation Unoccupied RTL event</name> \
<description>When track is now unoccupied, this event is sent(right-to-left)</description> \
</eventid> \
</group> \
</segment> \
<segment space='251'> \
<name>Node ID</name> \
<group> \
<name>Your name and description for this node</name> \
<string size='63'> \
<name>Node Name</name> \
</string> \
<string size='64' offset='1'> \
<name>Node Description</name> \
</string> \
</group> \
</segment> \
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

struct id_page{
  uint64_t node_id;
  uint16_t id_version;
  char manufacturer[32];
  char part_number[21];
  char hw_version[12];
};

static const byte MCP_CS  = 8 ; // CS input of CAN controller
static const byte MCP_INT =  2 ; // INT output of CAN controller
static const byte EEPROM_CS = 7;
static const byte CROSSING_OCCUPIED_OUTPUT = 5;

// Address in EEPROM of various important pieces of data
static const int CROSSING_EVENTS_ADDR = 0x0;
static const int NODE_NAME_DESCRIPTION_ADDR = 0x1000;

// The CAN controller.  This example uses the ACAN2515 or ACAN2517 library from Pierre Molinaro:
// https://github.com/pierremolinaro/acan2515
// https://github.com/pierremolinaro/acan2517
#if CAN_CHIP == CAN_CHIP_MCP2518
ACAN2517 can (MCP_CS, SPI, MCP_INT) ;
#else if CAN_CHIP == CAN_CHIP_MCP1515
ACAN2515 can (MCP_CS, SPI, MCP_INT) ;
#endif
M95_EEPROM eeprom(SPI, EEPROM_CS, 256, 3, true);

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

struct occupation {
  uint64_t pre_island;
  uint64_t island_occupied;
  uint64_t post_island;
  uint64_t unoccupied;
};

struct occupation_events {
  struct occupation ltr_events;
  struct occupation rtl_events;
} events;

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

/**
 * This is a callback function that is called by liblcc in order query how big our transmit buffer is
 */
int lcc_buffer_size(struct lcc_context* ctx){
#if CAN_CHIP == CAN_CHIP_MCP2518
  return can.driverTransmitBufferSize() - can.driverTransmitBufferCount();
#else if CAN_CHIP == CAN_CHIP_MCP2515
  return can.transmitBufferSize(0) - can.transmitBufferCount(0);
#endif
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
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    lcc_event_produce_event(evt_ctx, events.ltr_events.island_occupied);
  }else if(right_island_input == 0 &&
    current_state == ISLAND_OCCUPIED){
    current_state = POST_ISLAND_OCCUPIED_INCOMING;
    Serial.println(F("post island occupied incoming"));
    set_output_inactive();
  }else if(right_input == 1 &&
    current_state == POST_ISLAND_OCCUPIED_INCOMING){
    current_state = POST_ISLAND_OCCUPIED;
    Serial.println(F("post island occupied"));
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    lcc_event_produce_event(evt_ctx, events.ltr_events.post_island);
  }else if(right_input == 0 &&
    current_state == POST_ISLAND_OCCUPIED){
    Serial.println(F("train out LTR"));
    current_state = TRACK_UNOCCUPIED;
    current_direction = DIRECTION_UNKNOWN;
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    lcc_event_produce_event(evt_ctx, events.ltr_events.unoccupied);
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
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    lcc_event_produce_event(evt_ctx, events.rtl_events.island_occupied);
  }else if(left_island_input == 0 &&
    current_state == ISLAND_OCCUPIED){
    current_state = POST_ISLAND_OCCUPIED_INCOMING;
    Serial.println(F("post island occupied incoming"));
    set_output_inactive();
  }else if(left_input == 1 &&
    current_state == POST_ISLAND_OCCUPIED_INCOMING){
    current_state = POST_ISLAND_OCCUPIED;
    Serial.println(F("post island occupied"));
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    lcc_event_produce_event(evt_ctx, events.rtl_events.post_island);
  }else if(left_input == 0 &&
    current_state == POST_ISLAND_OCCUPIED){
    Serial.println(F("train out RTL"));
    current_state = TRACK_UNOCCUPIED;
    current_direction = DIRECTION_UNKNOWN;
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    lcc_event_produce_event(evt_ctx, events.rtl_events.unoccupied);
  }
}

void set_output_active(){
  // A train has come in - set our output to be active
  digitalWrite(CROSSING_OCCUPIED_OUTPUT, 1);
}

void set_output_inactive(){
  // A train has left - set our output to be inactive
  digitalWrite(CROSSING_OCCUPIED_OUTPUT, 0);
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
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    lcc_event_produce_event(evt_ctx, events.ltr_events.pre_island);
    return;
  }else if(right_input == 1 && current_state == TRACK_UNOCCUPIED){
    // Incoming train, right to left
    current_state = PRE_ISLAND_OCCUPIED;
    current_direction = DIRECTION_RTL;
    incoming_train_time = millis();
    Serial.println(F("Incoming train RTL"));
    set_output_active();
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    lcc_event_produce_event(evt_ctx, events.rtl_events.pre_island);
    return;
  }

  if(current_direction == DIRECTION_LTR){
    handle_ltr(left_input, left_island_input, right_island_input, right_input);
  }else if(current_direction == DIRECTION_RTL){
    handle_rtl(left_input, left_island_input, right_island_input, right_input);
  }

  if(millis_diff > timeout_millis &&
    current_state != TRACK_UNOCCUPIED){
    struct lcc_event_context* evt_ctx = lcc_context_get_event_context(ctx);
    if(current_direction == DIRECTION_RTL){
      lcc_event_produce_event(evt_ctx, events.rtl_events.unoccupied);
    }else{
      lcc_event_produce_event(evt_ctx, events.ltr_events.unoccupied);
    }
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
  }else if(address_space == 253){
    // basic config space
    lcc_memory_respond_information_query(ctx, alias, 1, address_space, sizeof(events), 0, 0);
  }else{
    // This memory space does not exist: return an error
    lcc_memory_respond_information_query(ctx, alias, 0, address_space, 0, 0, 0);
  }
}

void mem_address_space_read(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint8_t read_count){
  if(address_space == 251){
    // This space is what we use for node name/description
    uint8_t buffer[64];
    eeprom.read(starting_address, read_count, buffer);

    // For any blank data, we will read 0xFF
    // In this example, we know that we have strings, so replace all 0xFF with 0x00
    for(int x = 0; x < sizeof(buffer); x++){
      if(buffer[x] == 0xFF){
        buffer[x] = 0x00;
      }
    }

    lcc_memory_respond_read_reply_ok(ctx, alias, address_space, starting_address, buffer, read_count);
  }else if(address_space == 253){
    // Basic config space
    if(starting_address + read_count > sizeof(events)){
      // trying to read too much memory
      lcc_memory_respond_read_reply_fail(ctx, alias, address_space, 0, 0, NULL);
      return;
    }

    // LCC is defined as big-endian: Use built-in GCC functions to swap to big endian
    struct occupation_events events_big = events;
    events_big.ltr_events.island_occupied = __builtin_bswap64(events_big.ltr_events.island_occupied);
    events_big.ltr_events.pre_island = __builtin_bswap64(events_big.ltr_events.pre_island);
    events_big.ltr_events.post_island = __builtin_bswap64(events_big.ltr_events.post_island);
    events_big.ltr_events.unoccupied = __builtin_bswap64(events_big.ltr_events.unoccupied);
    events_big.rtl_events.island_occupied = __builtin_bswap64(events_big.rtl_events.island_occupied);
    events_big.rtl_events.pre_island = __builtin_bswap64(events_big.rtl_events.pre_island);
    events_big.rtl_events.post_island = __builtin_bswap64(events_big.rtl_events.post_island);
    events_big.rtl_events.unoccupied = __builtin_bswap64(events_big.rtl_events.unoccupied);
    uint8_t* events_as_u8 = (uint8_t*)&events_big;

    lcc_memory_respond_read_reply_ok(ctx, alias, address_space, starting_address, events_as_u8, read_count);
  }else{
    lcc_memory_respond_read_reply_fail(ctx, alias, address_space, 0, 0, NULL);
    return;
  }
}

void mem_address_space_write(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* data, int data_len){
  if(address_space == 251){
    eeprom.write(starting_address, data_len, data );

    lcc_memory_respond_write_reply_ok(ctx, alias, address_space, starting_address);
  }else if(address_space == 253){
    // In this example, we know that we only have event IDs that we want to store,
    // So we will swap all of our incoming data to store
    for(int offset = 0; offset < data_len % 8; offset++){
      uint64_t* u64_data = (uint64_t*)data;
      u64_data[offset] = __builtin_bswap64(u64_data[offset]);
    }

    // Copy the new data to our events data
    uint8_t* events_u8 = (uint8_t*)&events;
    memcpy(events_u8 + starting_address, data, data_len);

    // Write out to EEPROM
    eeprom.write(CROSSING_EVENTS_ADDR, sizeof(events), &events);

    lcc_memory_respond_write_reply_ok(ctx, alias, address_space, starting_address);
  }else{
    lcc_memory_respond_write_reply_fail(ctx, alias, address_space, starting_address, 0, NULL);
    return;
  }
}

void initialize_events_if_needed(uint64_t unique_id){
  // do a sanity check on our events: if all of our events are 0xFF...
  // That means our memory is not initialized yet.
  bool initialized = false;
  for(int x = 0; x < sizeof(events); x++){
    uint8_t* u8_data = (uint8_t*)&events;
    if(u8_data[x] != 0xFF){
      initialized = true;
      break;
    }
  }

  if(initialized){
    return;
  }

  // Let's go and initialize all of our events since they are
  // all 0xFF
  uint64_t event_id = unique_id << 16;

  events.ltr_events.pre_island = event_id++;
  events.ltr_events.island_occupied = event_id++;
  events.ltr_events.post_island = event_id++;
  events.ltr_events.unoccupied = event_id++;
  events.rtl_events.pre_island = event_id++;
  events.rtl_events.island_occupied = event_id++;
  events.rtl_events.post_island = event_id++;
  events.rtl_events.unoccupied = event_id++;
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
 
  // Delay our startup.  The EEPROM seems to get into a bad state where it will not talk
  // if we come up and come down(which happens when flashing from Arduino IDE)
  delay(3000);

  SPI.begin();
  eeprom.begin();

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
  // We will read the unique ID from the ID page of the EEPROM.
  uint64_t unique_id;
  struct id_page id;
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
  // lcc_datagram_context_new(ctx);
  struct lcc_event_context* evt_ctx = lcc_event_new(ctx);
  lcc_datagram_context_new(ctx);
  struct lcc_memory_context* mem_ctx = lcc_memory_new(ctx);

  lcc_memory_set_cdi(mem_ctx, cdi, sizeof(cdi), LCC_MEMORY_CDI_FLAG_ARDUINO_PROGMEM);
  lcc_memory_set_memory_functions(mem_ctx, 
    mem_address_space_information_query,
    mem_address_space_read,
    mem_address_space_write);

  // Load our events
  eeprom.read(CROSSING_EVENTS_ADDR, sizeof(events), &events);
  initialize_events_if_needed(unique_id);

  lcc_event_add_event_produced(evt_ctx, events.ltr_events.pre_island);
  lcc_event_add_event_produced(evt_ctx, events.ltr_events.island_occupied);
  lcc_event_add_event_produced(evt_ctx, events.ltr_events.post_island);
  lcc_event_add_event_produced(evt_ctx, events.ltr_events.unoccupied);
  lcc_event_add_event_produced(evt_ctx, events.rtl_events.pre_island);
  lcc_event_add_event_produced(evt_ctx, events.rtl_events.island_occupied);
  lcc_event_add_event_produced(evt_ctx, events.rtl_events.post_island);
  lcc_event_add_event_produced(evt_ctx, events.rtl_events.unoccupied);

#if CAN_CHIP == CAN_CHIP_MCP2518
  ACAN2517Settings settings (ACAN2517Settings::OSC_40MHz_DIVIDED_BY_2, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
#else if CAN_CHIP == CAN_CHIP_MCP2515
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  // We need to lower the transmit and receive buffer size(at least on the Uno), as otherwise
  // the ACAN2515 library will allocate too much memory
  settings.mReceiveBufferSize = 4;
  settings.mTransmitBuffer0Size = 10;
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
