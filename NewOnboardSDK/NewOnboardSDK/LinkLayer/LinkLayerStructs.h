#ifndef LINKLAYERSTRUCTS_H_INCLUDED
#define LINKLAYERSTRUCTS_H_INCLUDED

namespace DJIOnboardSDK {

struct SDKHeader
{
	unsigned int sof : 8; // 1byte

	unsigned int length : 10;
	unsigned int version : 6; // 2byte
	unsigned int session_id : 5;
	unsigned int is_ack : 1;
	unsigned int reserved0 : 2; // always 0

	unsigned int padding : 5;
	unsigned int enc_type : 3;
    unsigned int reserved1 : 24;

	unsigned int sequence_number : 16;
	unsigned int head_crc : 16;
//	unsigned int magic[0];
};

}

#endif // LINKLAYERSTRUCTS_H_INCLUDED