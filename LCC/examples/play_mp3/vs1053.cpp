#include "vs1053.h"

static const SPISettings vs1053_spi_settings_slow(1UL * 1000UL * 1000UL, MSBFIRST, SPI_MODE0);
static const SPISettings vs1053_spi_settings_data(4UL * 1000UL * 1000UL, MSBFIRST, SPI_MODE0);
  
VS1053::VS1053(SPIClass& spi, int control_cs, int data_cs, int dreq, int reset){
  m_spi = spi;
  m_control_cs = control_cs;
  m_data_cs = data_cs;
  m_dreq = dreq;
  m_reset = reset;
}

bool VS1053::begin(){
  pinMode(m_control_cs, OUTPUT);
  pinMode(m_data_cs, OUTPUT);
  pinMode(m_reset, OUTPUT);
  pinMode(m_dreq, INPUT);
  digitalWrite(m_control_cs, HIGH);
  digitalWrite(m_data_cs, HIGH);

  // Do a hardware reset
  digitalWrite(m_reset, LOW);
  delay(10);
  digitalWrite(m_reset, HIGH); //Bring up VS1053
  delay(5);

  uint16_t mode_reg = read_register(VS10XX_SCI_MODE);

  return mode_reg == 0x4800;
}

uint16_t VS1053::read_register(int regnum){
  while(!digitalRead(m_dreq)) ; //Wait for DREQ to go high indicating IC is available
  m_spi.beginTransaction(vs1053_spi_settings_slow);
  digitalWrite(m_control_cs, LOW); //Select control

  //SCI consists of instruction byte, address byte, and 16-bit data word.
  m_spi.transfer(0x03);  //Read instruction
  m_spi.transfer(regnum & 0xFF);

  uint8_t response1 = m_spi.transfer(0xFF); //Read the first byte
  while(!digitalRead(m_dreq)) ; //Wait for DREQ to go high indicating command is complete
  uint8_t response2 = m_spi.transfer(0xFF); //Read the second byte
  while(!digitalRead(m_dreq)) ; //Wait for DREQ to go high indicating command is complete

  digitalWrite(m_control_cs, HIGH); //Deselect Control
  m_spi.endTransaction();

  unsigned int resultvalue = response1 << 8;
  resultvalue |= response2;
  return resultvalue;
}

void VS1053::write_register(int regnum, uint16_t regval){
  while(!digitalRead(m_dreq)) ; //Wait for DREQ to go high indicating IC is available
  m_spi.beginTransaction(vs1053_spi_settings_slow);
  digitalWrite(m_control_cs, LOW); //Select control

  //SCI consists of instruction byte, address byte, and 16-bit data word.
  m_spi.transfer(0x02); //Write instruction
  m_spi.transfer(regnum);
  m_spi.transfer((regval & 0xFF00) >> 8);
  m_spi.transfer((regval & 0x00FF) >> 0);
  while(!digitalRead(m_dreq)) ; //Wait for DREQ to go high indicating command is complete
  digitalWrite(m_control_cs, HIGH); //Deselect Control
  m_spi.endTransaction();
}

void VS1053::set_volume(uint8_t left, uint8_t right){
  write_register(VS10XX_SCI_VOL, (left << 8) | right & 0xFFFF);
}

void VS1053::transfer_32bytes_data(uint8_t* buffer){
  m_spi.beginTransaction(vs1053_spi_settings_data);
  digitalWrite(m_data_cs, LOW); //Select Data
  for(int y = 0 ; y < 32 ; y++) {
    m_spi.transfer(buffer[y]); // Send SPI byte
  }
  digitalWrite(m_data_cs, HIGH); //Deselect Data
  m_spi.endTransaction();
}

bool VS1053::needs_data(){
  // DREQ is high, VS1053b can take at least 32 bytes of SDI data or one SCI command
  return !!digitalRead(m_dreq);
}