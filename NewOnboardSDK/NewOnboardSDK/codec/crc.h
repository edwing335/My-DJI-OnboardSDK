#ifndef CRC_H_INCLUDED
#define CRC_H_INCLUDED

unsigned short crc16_update(unsigned short crc, unsigned char ch);
unsigned int crc32_update(unsigned int crc, unsigned char ch);

#endif // CRC_H_INCLUDED
