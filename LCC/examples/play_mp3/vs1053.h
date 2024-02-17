#ifndef VS1053_H
#define VS1053_H

#include <SPI.h>

//VS10xx SCI Registers
#define VS10XX_SCI_MODE 0x00
#define VS10XX_SCI_STATUS 0x01
#define VS10XX_SCI_BASS 0x02
#define VS10XX_SCI_CLOCKF 0x03
#define VS10XX_SCI_DECODE_TIME 0x04
#define VS10XX_SCI_AUDATA 0x05
#define VS10XX_SCI_WRAM 0x06
#define VS10XX_SCI_WRAMADDR 0x07
#define VS10XX_SCI_HDAT0 0x08
#define VS10XX_SCI_HDAT1 0x09
#define VS10XX_SCI_AIADDR 0x0A
#define VS10XX_SCI_VOL 0x0B
#define VS10XX_SCI_AICTRL0 0x0C
#define VS10XX_SCI_AICTRL1 0x0D
#define VS10XX_SCI_AICTRL2 0x0E
#define VS10XX_SCI_AICTRL3 0x0F

class VS1053 {
public:
  VS1053(SPIClass& spi, int control_cs, int data_cs, int dreq, int reset);

  bool begin();

  uint16_t read_register(int regnum);

  void write_register(int regnum, uint16_t regval);

  void set_volume(uint8_t left, uint8_t right);

  /**
   * Transfer 32 bytes of data.
   * buffer MUST be 32 bytes long
   */
  void transfer_32bytes_data(uint8_t* buffer);

  /**
   * Check if the VS1053 needs data
   */
  bool needs_data();

private:
  SPIClass m_spi;
  int m_control_cs;
  int m_data_cs;
  int m_dreq;
  int m_reset;
};

#endif /* VS1053_H */