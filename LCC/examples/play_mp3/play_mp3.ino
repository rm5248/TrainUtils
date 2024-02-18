/*
 * This is based off of the SparkFun MP3 Shield code.
 *
 * Because the LCC shield and the MP3 shield are not compatible with each other
 * (they each use the same pins), we need to hook them together with jumper wires.
 * There is /just enough/ RAM and memory on the Uno to run this example.
 *
 * To hook up the shield, connect with jumper wires as follows:
 *
 * MP3 Shield Pin <-----------------> Arduino Pin
 *  2 (MP3 DREQ)    <---------------> A2
 *  6 (MP3 Chip Select) <-----------> A0
 *  7 (MP3 data chip select) <------> A1
 *  8 (MP3 Reset) <-----------------> A3
 *  9 (SD card select) <------------> A4
 */

#include <SPI.h>
#include <ACAN2515.h>
#include <M95_EEPROM.h>
#include <SdFat.h>
#include <lcc.h>
#include <lcc-event.h>
#include <lcc-datagram.h>
#include <lcc-memory.h>

#include "vs1053.h"

#define TRUE  1
#define FALSE  0

static const byte MCP2515_CS  = 8 ; // CS input of MCP2515
static const byte MCP2515_INT =  2 ; // INT output of MCP2515
static const byte EEPROM_CS = 7;
//MP3 Player Shield pin mapping. See the schematic
#define MP3_XCS A0 //Control Chip Select Pin (for accessing SPI Control/Status registers)
#define MP3_XDCS A1 //Data Chip Select / BSYNC Pin
#define MP3_DREQ A2 //Data Request Pin: Player asks for more data
#define MP3_RESET A3 //Reset is active low
static const byte SDCARD_CS = A4;

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(8)

#define SD_CONFIG SdSpiConfig(SDCARD_CS, SHARED_SPI, SPI_CLOCK)

static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL ; // 16 MHz

enum PlayingState{
  NOT_PLAYING,
  PLAYING,
  WAITING_FOR_END,
};

SdFat card;
FatFile track;
ACAN2515 can (MCP2515_CS, SPI, MCP2515_INT) ;
M95_EEPROM eeprom(SPI, EEPROM_CS, 256, 3, true);
static lcc_context* ctx;
CANMessage frame ;
struct lcc_can_frame lcc_frame;
unsigned long claim_alias_time;
static uint32_t gBlinkLedDate = 0 ;
uint8_t mp3DataBuffer[32];
PlayingState playing_state = NOT_PLAYING;
VS1053 vs1053(SPI, MP3_XCS, MP3_XDCS, MP3_DREQ, MP3_RESET);

void incoming_lcc_evt(struct lcc_context* ctx, uint64_t event_id){
  Serial.print("Got event!  low bits: ");
  int low_bits = event_id;
  Serial.print(low_bits, HEX);

  playMP3("track001.mp3");

  // Note: Not enough memory to keep configuration in RAM.
  // Every event that comes in we need to search EEPROM for.
}

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

void setup_lcc(){
  eeprom.begin();

  ACAN2515Settings settings (QUARTZ_FREQUENCY, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  settings.mReceiveBufferSize = 4;
  settings.mTransmitBuffer0Size = 8;
  // OpenLCB uses the following CAN propogation settings with the MCP2515:
  // CFN3 = 0x02
  // CFN2 = 0x90
  // CFN1 = 0x07
  settings.mPropagationSegment = 1;
  settings.mPhaseSegment1 = 3;
  settings.mPhaseSegment2 = 3;
  settings.mSJW = 1;
  settings.mTripleSampling = false;
  settings.mBitRatePrescaler = 8;
  const uint16_t errorCode = can.begin (settings, [] { can.isr () ; }) ;
  if (errorCode != 0) {
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }

  uint64_t unique_id;
  eeprom.read_id_page(8, &unique_id);

  // Create an LCC context that determines our communications
  ctx = lcc_context_new();

  // Set the unique identifier that this node will use
  lcc_context_set_unique_identifer(ctx, unique_id);

  // Set the callback function that will be called to write  frame out to the bus
  lcc_context_set_write_function(ctx, lcc_write);

  // Set simple node information that is handled by the 'simple node information protocol'
  lcc_context_set_simple_node_information(ctx,
                                        "NVMR",
                                        "Audio Player",
                                        "1.0",
                                        "0.1");

  int val = lcc_context_generate_alias(ctx);
  if(val != LCC_OK){
    Serial.println(F("ERROR: Can't generate alias!"));
    while(1){}
  }
  
  claim_alias_time = millis() + 220;

  struct lcc_event_context* evt_ctx = lcc_event_new(ctx);
  lcc_event_add_event_consumed(evt_ctx, 0ll);

  lcc_event_set_incoming_event_function(evt_ctx, incoming_lcc_evt);
  lcc_event_set_listen_all_events(evt_ctx, 1);

  struct lcc_datagram_context* datagram_ctx = lcc_datagram_context_new(ctx);
  struct lcc_memory_context* mem_ctx = lcc_memory_new(ctx);
}

void setup() {
  pinMode(MCP2515_CS, OUTPUT);
  pinMode(EEPROM_CS, OUTPUT);
  pinMode(SDCARD_CS, OUTPUT);
  pinMode(MP3_XCS, OUTPUT);
  pinMode(MP3_XDCS, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH);
  digitalWrite(MCP2515_CS, HIGH);
  digitalWrite(EEPROM_CS, HIGH);
  digitalWrite(MP3_XCS, HIGH);
  digitalWrite(MP3_XDCS, HIGH);

  SPI.begin();
  eeprom.begin();

  // pinMode(MP3_DREQ, INPUT);
  // pinMode(MP3_XCS, OUTPUT);
  // pinMode(MP3_XDCS, OUTPUT);
  // pinMode(MP3_RESET, OUTPUT);


  // digitalWrite(MP3_XCS, HIGH); //Deselect Control
  // digitalWrite(MP3_XDCS, HIGH); //Deselect Data
  // digitalWrite(MP3_RESET, LOW); //Put VS1053 into hardware reset
  


  Serial.begin(115200); //Use serial for debugging 
  Serial.println("Setup");

  //Setup SD card interface
  delay(5);

  if (!card.begin(SD_CONFIG)){
    Serial.println("Error: Card init"); //Initialize the SD card and configure the I/O pins.
    Serial.println(card.sdErrorCode(), HEX);
    Serial.println(card.sdErrorData(), HEX);
  }else{
    Serial.println("Card init OK.  Listing files: ");
    card.ls(LS_R | LS_DATE | LS_SIZE);
  }


  // Serial.println("Waiting 1 second");
  // delay(1000);
  // Serial.println("Wait forever");
  // while(1){}

  if(!vs1053.begin()){
    Serial.println("Error: Can't find VS1053");
  }

  //Let's check the status of the VS1053
  unsigned int MP3Mode = vs1053.read_register(VS10XX_SCI_MODE);
  unsigned int MP3Status = vs1053.read_register(VS10XX_SCI_STATUS);
  unsigned int MP3Clock = vs1053.read_register(VS10XX_SCI_CLOCKF);

  Serial.print("SCI_Mode (0x4800) = 0x");
  Serial.println(MP3Mode, HEX);

  Serial.print("SCI_Status (0x48) = 0x");
  Serial.println(MP3Status, HEX);

  int vsVersion = (MP3Status >> 4) & 0x000F; //Mask out only the four version bits
  Serial.print("VS Version (VS1053 is 4) = ");
  Serial.println(vsVersion, DEC); //The 1053B should respond with 4. VS1001 = 0, VS1011 = 1, VS1002 = 2, VS1003 = 3

  // Now that we have the VS1053 up and running, increase the internal clock multiplier.
  // This is needed so it doesn't sound like demon music
  vs1053.write_register(VS10XX_SCI_CLOCKF, 0x6000); //Set multiplier to 3.0x

  vs1053.set_volume(40, 40);

  setup_lcc();

    Serial.print(F("Free at start: "));
  Serial.println(freeMemory());
}

void check_lcc_available(){
  if (can.available ()) {
    // If we have an incoming CAN frame, turn it into an LCC frame and push it to liblcc
    can.receive (frame) ;
    lcc_frame.can_id = frame.id;
    lcc_frame.can_len = frame.len;
    memcpy(&lcc_frame.data, frame.data, 8);
    lcc_context_incoming_frame(ctx, &lcc_frame);
  }
}

void check_claim_alias(){
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

void loop(){
  check_lcc_available();
  check_claim_alias();

  play_chunk();
}

void play_chunk(){
  if(playing_state == NOT_PLAYING){
    return;
  }

  if(playing_state == PLAYING){
    while(vs1053.needs_data()){
      vs1053.transfer_32bytes_data(mp3DataBuffer);

      if(!track.read(mp3DataBuffer, sizeof(mp3DataBuffer))) { //Try reading 32 new bytes of the song
        // No more data left to read: no more bytes to the mp3
        playing_state = WAITING_FOR_END;
      }
    }
  }else if(playing_state == WAITING_FOR_END){
    if(vs1053.needs_data()){
      track.close();
      playing_state = NOT_PLAYING;
    }
  }
}

//PlayMP3 pulls 32 byte chunks from the SD card and throws them at the VS1053
//We monitor the DREQ (data request pin). If it goes low then we determine if
//we need new data or not. If yes, pull new from SD card. Then throw the data
//at the VS1053 until it is full.
void playMP3(char* fileName) {
  if(playing_state != NOT_PLAYING){
    return;
  }

  playing_state = PLAYING;

  if (!track.open(fileName, O_READ)) { //Open the file in read mode.
    Serial.print("Failed to open ");
    Serial.println(fileName);
    playing_state = NOT_PLAYING;
    while(1){}
    return;
  }

  Serial.print("Track open: ");
  Serial.println(fileName);

  if(!track.read(mp3DataBuffer, sizeof(mp3DataBuffer))) {
    playing_state = NOT_PLAYING;
    track.close();
  }
}

//Write to VS10xx register
//SCI: Data transfers are always 16bit. When a new SCI operation comes in 
//DREQ goes low. We then have to wait for DREQ to go high again.
//XCS should be low for the full duration of operation.
// void Mp3WriteRegister(unsigned char addressbyte, unsigned char highbyte, unsigned char lowbyte){
//   SPI.beginTransaction(*vs1053_spi_settings);
//   while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating IC is available
//   digitalWrite(MP3_XCS, LOW); //Select control

//   // delayMicroseconds(5000);

//   //SCI consists of instruction byte, address byte, and 16-bit data word.
//   SPI.transfer(0x02); //Write instruction
//   SPI.transfer(addressbyte);
//   SPI.transfer(highbyte);
//   SPI.transfer(lowbyte);
//   while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete
//   digitalWrite(MP3_XCS, HIGH); //Deselect Control
//   SPI.endTransaction();
// }

// //Read the 16-bit value of a VS10xx register
// unsigned int Mp3ReadRegister (unsigned char addressbyte){
//   SPI.beginTransaction(*vs1053_spi_settings);
//   while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating IC is available
//   digitalWrite(MP3_XCS, LOW); //Select control

//   //SCI consists of instruction byte, address byte, and 16-bit data word.
//   SPI.transfer(0x03);  //Read instruction
//   SPI.transfer(addressbyte);

//   uint8_t response1 = SPI.transfer(0xFF); //Read the first byte
//   while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete
//   uint8_t response2 = SPI.transfer(0xFF); //Read the second byte
//   while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete

//   digitalWrite(MP3_XCS, HIGH); //Deselect Control
//   SPI.endTransaction();

//   Serial.print("resp1: ");
//   Serial.println(response1, HEX);
//     Serial.print("resp2: ");
//   Serial.println(response2, HEX);

//   unsigned int resultvalue = response1 << 8;
//   resultvalue |= response2;
//   return resultvalue;
// }

// //Set VS10xx Volume Register
// void Mp3SetVolume(unsigned char leftchannel, unsigned char rightchannel){
//   Mp3WriteRegister(SCI_VOL, leftchannel, rightchannel);
// }

