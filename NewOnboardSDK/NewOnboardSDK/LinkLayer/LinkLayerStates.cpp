#include "LinkLayerStates.h"
#include "LinkLayerSharedData.h"
#include "LinkLayerStructs.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <stdint.h>

#include "codec/crc.h"
#include "codec/aes256.h"

using namespace std;
using namespace DJIOnboardSDK;

#define _SDK_U32_SET(_addr, _val)   (*((unsigned int*)(_addr)) = (_val))
#define _SDK_U16_SET(_addr, _val)   (*((unsigned short*)(_addr)) = (_val))

LinkLayerRecvState::LinkLayerRecvState(string id, FSM::StateMachine* fsm):
FSM::State(id, fsm)
{
}

void LinkLayerRecvState::on_reset()
{
    // wrong usage of on_reset
    assert(false);
}

LinkLayerSharedData* LinkLayerRecvState::get_data()
{
    FSM::SharedData* data = get_fsm()->get_shared_data();
    return dynamic_cast<LinkLayerSharedData*>(data);
}

int LinkLayerRecvState::store_data(unsigned char byte)
{
    LinkLayerSharedData* sharedData = get_data();
    sharedData->m_recvBuffer[sharedData->m_recvIndex++] = byte;
    if (sharedData->m_recvIndex > _SDK_MAX_RECV_SIZE)
        return -1;

    return 0;
}

int LinkLayerRecvState::get_recv_data_length()
{
    LinkLayerSharedData* sharedData = get_data();
    assert(sharedData->m_recvIndex >= sizeof(SDKHeader));

    SDKHeader* header = (SDKHeader*)sharedData->m_recvBuffer;
    return header->length;
}

bool LinkLayerRecvState::recv_data_complete()
{
    LinkLayerSharedData* sharedData = get_data();
    assert(sharedData->m_recvIndex >= sizeof(SDKHeader));

    int length = get_recv_data_length();
    if (length == sharedData->m_recvIndex)
        return true;
    else if (length > sharedData->m_recvIndex)
        return false;

    assert(0);
    return false;
}

WaitForHeaderState::WaitForHeaderState(FSM::StateMachine* fsm):
    LinkLayerRecvState(WAIT_FOR_HEADER_STATE_ID, fsm)
{
}

void WaitForHeaderState::on_recv_byte(unsigned char byte)
{
    int res = store_data(byte);
    if (res != 0)
    {
        cerr << "Error occurs when storing the byte. " << endl;
        return;
    }

    if (header_complete())
    {
        if (header_correct())
        {
            if (recv_data_complete())
                // the received data only contains header
                get_fsm()->go_to_state(COMPLETE_STATE_ID);
            else
                get_fsm()->go_to_state(WAIT_FOR_BODY_STATE_ID);

                return;
        }
        else
        {
            // TODO: try to correct the header
            assert(0);
        }
    }
}

bool WaitForHeaderState::header_complete()
{
    LinkLayerSharedData* sharedData = get_data();
    if (sharedData->m_recvIndex == sizeof(SDKHeader))
        return true;
    else
        return false;
}

bool WaitForHeaderState::header_correct()
{
    LinkLayerSharedData* sharedData = get_data();

	SDKHeader* header = (SDKHeader*)(sharedData->m_recvBuffer);

	if ((header->sof == _SDK_SOF) &&
		(header->version == 0) &&
		(header->length <= _SDK_MAX_RECV_SIZE) &&
		(header->reserved0 == 0) &&
		(header->reserved1 == 0) &&
		(check_header_crc())
		)
    {
        return true;
    }
    else
        return false;
}

const static unsigned short CRC_INIT = 0x3AA3;

bool WaitForHeaderState::check_header_crc()
{
	unsigned int i;
	unsigned short wCRC = CRC_INIT;

	unsigned int length = sizeof(SDKHeader);
    unsigned char* msg = get_data()->m_recvBuffer;

	for (i = 0; i < length; i++)
	{
		wCRC = crc16_update(wCRC, msg[i]);
	}

	if (wCRC == 0)
        return true;
    else
        return false;
}


WaitForBodyState::WaitForBodyState(FSM::StateMachine* fsm) :
LinkLayerRecvState(WAIT_FOR_BODY_STATE_ID, fsm)
{
}

void WaitForBodyState::on_recv_byte(unsigned char byte)
{
    int res = store_data(byte);
    if (res != 0)
    {
        cerr << "Error occurs when storing the byte. " << endl;
    }

    if (recv_data_complete())
    {
        if (recv_data_correct())
        {
            get_fsm()->go_to_state(COMPLETE_STATE_ID);
        }
        else
        {
            // the received data is corrupted
            // TODO: fix the corrupted data
            assert(0);
            return;
        }
    }
}

bool WaitForBodyState::recv_data_correct()
{
    return check_recv_data_crc();
}

bool WaitForBodyState::check_recv_data_crc()
{
	unsigned int i;
	unsigned int wCRC = CRC_INIT;

    unsigned int length = get_recv_data_length();
    unsigned char* msg = get_data()->m_recvBuffer;

	for (i = 0; i < length; i++)
	{
		wCRC = crc32_update(wCRC, msg[i]);
	}

	if (wCRC == 0)
        return true;
    else
        return false;
}


CompleteState::CompleteState(FSM::StateMachine* fsm) :
LinkLayerRecvState(COMPLETE_STATE_ID, fsm)
{
}

void CompleteState::on_recv_byte(unsigned char byte)
{
    // wrong usage
    assert(false);
}

void CompleteState::on_reset()
{
    LinkLayerSharedData* sharedData = get_data();

    // reset buffer
    memset(sharedData->m_recvBuffer, 0, _SDK_MAX_RECV_SIZE);
    sharedData->m_recvIndex = 0;

    get_fsm()->go_to_state(WAIT_FOR_HEADER_STATE_ID); 
}

int CompleteState::on_enter()
{
    return decrypt_recv_data();
}

int CompleteState::decrypt_recv_data()
{
    LinkLayerSharedData* sharedData = get_data();
    SDKHeader* header = (SDKHeader*)sharedData->m_recvBuffer;

    unsigned int frameLength = get_recv_data_length();

    if (header->enc_type == 0) // dont need decryption
        return -1;

    if ( frameLength == sizeof(SDKHeader)) // nothing in the body
        return -1;

    assert(frameLength > sizeof(SDKHeader) + _SDK_CRC_DATA_SIZE);

    unsigned char* data = (unsigned char*)header + sizeof(SDKHeader);
    unsigned int dataLength = frameLength - _SDK_CRC_DATA_SIZE - sizeof(SDKHeader);
    unsigned int loopTimes = dataLength / 16;

    aes256_context ctx;
    aes256_init(&ctx, sharedData->m_key);
    for (unsigned int i = 0; i < loopTimes; i++)
    {
        aes256_decrypt_ecb(&ctx, data);
        data += 16;
    }
    aes256_done(&ctx);

    header->length = header->length - header->padding;

    return 0;
}
