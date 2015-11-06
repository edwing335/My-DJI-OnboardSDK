#include "DJIDrone.h"
#include "AppLayerMacros.h"
#include "Utility/Semaphore.h"

#include <iostream>
#include <thread>

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define CHECK_RES(res, err_msg, ret_val)    \
{                                           \
    if (res != 0) {                         \
        cerr << err_msg << endl;            \
        return ret_val;                     \
    }                                       \
}

using namespace DJIOnboardSDK;
using namespace std;

const static size_t CMD_SET_SIZE = 2u;
const static unsigned int DEFAULT_TIMEOUT = 500; // ms
const static unsigned int DEFUALT_RETRY_TIMES = 3;


DJIDrone::DJIDrone() :m_initialized(false), m_cmdSeqCnt(numeric_limits<unsigned char>::max())
{

}

DJIDrone::~DJIDrone()
{

}

void DJIDrone::initialize()
{
    init_link_layer(); 
    init_activation_data(); 

    m_initialized = true; 
}

int DJIDrone::activate()
{
    assert(m_initialized);

    uint8_t cmdData[1024];
    size_t cmdDataLength;

    prepare_cmd_data(ACTIVATION_CMD_SET, API_USER_ACTIVATION, (uint8_t*)&m_activationData, sizeof(m_activationData), cmdData, cmdDataLength);

    uint8_t ackData[1024];
    size_t ackLength;
    int res = send_cmd_data(cmdData, cmdDataLength, ackData, ackLength);
    CHECK_RES(res, "Activation failed. ", -1); 

    return handle_activation_ack(ackData, ackLength);
}

int DJIDrone::obtain_control()
{
    return change_control_authorization(OBTAIN_CONTROL_BYTE);
}

int DJIDrone::release_control()
{
    return change_control_authorization(RELEASE_CONTROL_BYTE); 
}

int DJIDrone::take_off()
{
    int res = change_flight_mode(TAKE_OFF_CMD_VAL); 
    CHECK_RES(res, "Take-off failed. ", res); 

    res = wait_for_take_off_finish(); 
    CHECK_RES(res, "Something wrong when waiting for take-off finish. ", -1); 

    return 0; 
}

int DJIDrone::land()
{
    int res = change_flight_mode(LANDING_CMD_VAL);
    CHECK_RES(res, "Landing failed. ", res);

    res = wait_for_landing_finish();
    CHECK_RES(res, "Something wrong when waiting for landing finish. ", -1);

    return 0;
}

void DJIDrone::init_link_layer()
{
    m_linkLayer.initialize("COM5", "7814d36311f37bf6840d4460fc3e7040e737f2c3eb3def19a4cca9e7865e10ee");
    m_linkLayer.set_request_handler(nullptr);
}

void DJIDrone::init_activation_data()
{
    m_activationData.m_appID = 1017182;
    m_activationData.m_apiLevel = 2;
    m_activationData.m_apiVersion = SDK_VERSION;
    memcpy(m_activationData.m_buddleId, "1234567890123456789012", 32);
}

void DJIDrone::prepare_cmd_data(unsigned char cmdSet, unsigned char cmdId, uint8_t* data, size_t length, uint8_t* dest, size_t& newSize)
{
    newSize = length + CMD_SET_SIZE;

    dest[0] = cmdSet;
    dest[1] = cmdId;
    memcpy(dest + 2, data, length);
}

int DJIDrone::send_cmd_data(uint8_t* cmdData, size_t length, uint8_t* ackBuffer, size_t& ackLength)
{
    semaphore semForAck;
    int res = 0;
    m_linkLayer.send(cmdData, length, DEFAULT_TIMEOUT, DEFUALT_RETRY_TIMES,
        [this, &semForAck, &res, ackBuffer, &ackLength](uint8_t* recvData, size_t recvLength)
    {
        res = 0;
        ackLength = recvLength;
        memcpy(ackBuffer, recvData, 2);
        semForAck.signal();
    },
        [this, &semForAck, &res]()
    {
        res = -1;
        semForAck.signal();
    });

    semForAck.wait();
    return res;
}

int DJIDrone::handle_activation_ack(uint8_t* ackData, size_t length)
{
    if (length != 2)
        return -1;

    unsigned short ackNum;
    memcpy((unsigned char*)(&ackNum), ackData, 2);
    if (ackNum == SDK_ACTIVATE_NEW_DEVICE)
    {
        cout << "New device. Need to activate again. " << endl;
        return activate();
    }
    else if (ackNum == SDK_ACTIVATE_SUCCESS)
    {
        cout << "Activation Successfully. " << endl;
        return 0;
    }
    else
    {
        printf("%s,line %d,activate ERR code:0x%X\n", __func__, __LINE__, ackNum);
        return ackNum;
    }

}

int DJIDrone::change_control_authorization(unsigned char controlFlag)
{
    assert(m_initialized);

    uint8_t cmdData[1024];
    size_t cmdDataLength;

    prepare_cmd_data(CTRL_CMD_SET, API_CTRL_MANAGEMENT, (uint8_t*)&controlFlag, 1, cmdData, cmdDataLength);

    uint8_t ackData[1024];
    size_t ackLength;
    int res = send_cmd_data(cmdData, cmdDataLength, ackData, ackLength);
    if (res != 0)
        return -1;

    return handle_control_authorization_ack(ackData, ackLength);
}

int DJIDrone::handle_control_authorization_ack(uint8_t* ackData, size_t length)
{
    if (length != 2)
    {
        cout << "Received ack is un-recognized. " << endl;
        return -1;
    }

    unsigned short ackNum;
    memcpy((unsigned char*)(&ackNum), ackData, 2);

    switch (ackNum)
    {
    case 0x0001:
        printf("%s,line %d, release control successfully\n", __func__, __LINE__);
        return 0;
    case 0x0002:
        printf("%s,line %d, obtain control successfully\n", __func__, __LINE__);
        return 0;
    case 0x0003:
        printf("%s,line %d, obtain control failed\n", __func__, __LINE__);
        return -1;
    default:
        printf("%s,line %d, there is unkown error,ack=0x%X\n", __func__, __LINE__, ackNum);
        return -1;
    }

}

int DJIDrone::change_flight_mode(unsigned char flightMode)
{
    assert(m_initialized);
    unsigned char seqNum = get_next_cmd_seq(); 
    int res = start_flight_mode_cmd(flightMode, seqNum); 
    CHECK_RES(res, "Starting flight mode cmd failed. ", -1); 

    short status; 
    do
    {
        status = get_flight_mode_cmd_status(seqNum); 
    } while (status == SWITCHING_IN_PROGRESS); 

    switch (status)
    {
    case WRONG_CMD_SEQ_NUM: 
        cerr << "Other command is in progress. " << endl; 
        return -1; 
    case SWITCHING_FAILED: 
        cerr << "Command failed. " << endl; 
        return -1; 
    case SWITCHING_SUCCEED: 
        return 0; 
    case -1: 
        cerr << "Other errors. " << endl; 
        return -1; 
    default: 
        cerr << "Unrecognized status. " << endl; 
        return -1; 
    }
}

int DJIDrone::start_flight_mode_cmd(unsigned char flightMode, unsigned char seqNum)
{
    FlightModeData data;
    data.m_flightMode = flightMode;
    data.m_cmdSeq = seqNum;

    uint8_t cmdData[1024];
    size_t cmdDataLength;

    prepare_cmd_data(CTRL_CMD_SET, API_CMD_REQUEST, (uint8_t*)&data, sizeof(FlightModeData), cmdData, cmdDataLength);

    uint8_t ackData[1024];
    size_t ackLength;
    int res = send_cmd_data(cmdData, cmdDataLength, ackData, ackLength);
    if (res != 0)
    {
        cerr << "Timeout. " << endl;
        return -1;
    }

    res = handle_flight_mode_ack(ackData, ackLength);
    if (res != 0)
    {
        cerr << "Changing Flight mode failed. " << endl;
        return -1;
    }

    return 0; 
}

int DJIDrone::handle_flight_mode_ack(uint8_t* ackData, size_t length)
{
    if (length != 2)
    {
        cout << "Received ack is un-recognized. " << endl;
        return -1;
    }

    unsigned short ackNum;
    memcpy((unsigned char*)(&ackNum), ackData, 2);

    if (ackNum == CMD_RECIEVE)
    {
        return 0;
    }
    else if (ackNum == SDK_ERR_NO_RESPONSE || ackNum == SDK_ERR_COMMAND_NOT_SUPPORTED
        || ackNum == REQ_TIME_OUT || ackNum == REQ_REFUSE)
    {
        static const char err[3][16] = { { "didn't get ack" }, { "timeout" }, { "cmd refuse" } };
        printf("%s,line %d,Status Ctrl %s,Return 0x%X\n", __func__, __LINE__,
            &err[ackNum + 1][0], ackNum);
        return -1; 
    }
    else
    {
        cerr << "Unknow ack. " << endl; 
        return -1; 
    }

}
short DJIDrone::get_flight_mode_cmd_status(unsigned char seqNum)
{
    uint8_t cmdData[1024];
    size_t cmdDataLength;

    prepare_cmd_data(CTRL_CMD_SET, API_CMD_STATUS_REQUEST, (uint8_t*)&seqNum, 1, cmdData, cmdDataLength);

    uint8_t ackData[1024];
    size_t ackLength;
    int res = send_cmd_data(cmdData, cmdDataLength, ackData, ackLength);
    if (res != 0)
    {
        cerr << "Timeout. " << endl;
        return -1;
    }

    if (ackLength != 2)
    {
        cout << "Received ack is un-recognized. " << endl;
        return -1;
    }

    unsigned short ackNum;
    memcpy((unsigned char*)(&ackNum), ackData, 2);

    return ackNum; 
}

int DJIDrone::wait_for_take_off_finish()
{
    this_thread::sleep_for(chrono::seconds(6)); 
    return 0; 
}

int DJIDrone::wait_for_landing_finish()
{
    // TODO: use status instead of sleep
    this_thread::sleep_for(chrono::seconds(6));
    return 0;
}

unsigned char DJIDrone::get_next_cmd_seq()
{
    if (m_cmdSeqCnt == numeric_limits<unsigned char>::max())
        m_cmdSeqCnt = 0;
    else
        m_cmdSeqCnt++; 
    return m_cmdSeqCnt; 
}