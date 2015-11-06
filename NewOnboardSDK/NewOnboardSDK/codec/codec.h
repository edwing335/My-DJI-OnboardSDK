#ifndef CODEC_H_INCLUDED
#define CODEC_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#define _SDK_HEAD_DATA_LEN          (sizeof(SDKHeader) - 2)
#define _SDK_FULL_DATA_SIZE_MIN     (sizeof(SDKHeader) + _SDK_CRC_DATA_SIZE)

#define _SDK_U32_SET(_addr, _val)   (*((unsigned int*)(_addr)) = (_val))
#define _SDK_U16_SET(_addr, _val)   (*((unsigned short*)(_addr)) = (_val))

#define _SDK_CALC_CRC_HEAD(_msg, _len)   sdk_stream_crc16_calc((const unsigned char*)(_msg), _len)
#define _SDK_CALC_CRC_TAIL(_msg, _len)   sdk_stream_crc32_calc((const unsigned char*)(_msg), _len)

typedef struct
{
	unsigned short reuse_index;
	unsigned short reuse_count;
	unsigned short recv_index;
	unsigned char comm_recv_buf[_SDK_MAX_RECV_SIZE];
	// for encrypt
    unsigned char         comm_key[32];
    unsigned char         enc_enabled;
} SDKFilter;


typedef void(*ptr_filter_hook)(SDKHeader* p_head);

void sdk_serial_byte_handle(unsigned char in_data);
void sdk_serial_set_hook(ptr_filter_hook p_hook);

void sdk_set_encrypt_key_interface(const char* sz_key);
unsigned short sdk_encrypt_interface(unsigned char *pdest, const unsigned char *psrc,
		unsigned short w_len,unsigned char is_ack,unsigned char is_enc,unsigned char session_id,unsigned short seq_num);

/*
* Internal functions
*
**/

void sdk_stream_recalc_crc(void* p_data);
unsigned short sdk_stream_crc16_calc(const unsigned char* pMsg, unsigned int nLen);
unsigned int sdk_stream_crc32_calc(const unsigned char* pMsg, unsigned int nLen);
#endif //CODEC_H_INCLUDED
