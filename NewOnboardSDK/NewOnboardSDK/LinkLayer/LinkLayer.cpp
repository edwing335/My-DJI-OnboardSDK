#include "LinkLayerStates.h"
#include "LinkLayerSharedData.h"
#include "LinkLayerStructs.h"
#include "Utility/StateMachine.h"
#include "Serial/serial.h"
#include "codec/aes256.h"
#include "codec/crc.h"

#include "LinkLayer.h"

#include <cassert>
#include <iostream>
#include <thread>
#include <limits>
#include <chrono>

using namespace std;
using namespace DJIOnboardSDK;

const static unsigned int SERIAL_TIMEOUT = 1; // ms
const static unsigned int RECV_CALLBACK_THREAD_POOL_SIZE = 8; 

Session::Session()
{
    m_stopped = false;
    m_available = true;
    m_seqNum = 0;
    m_tryTimes = 0;

    m_replied = false;
    memset(m_repliedData, 0, 1024);
    m_repliedLength = 0;

    m_waitingThread = thread([this]()
    {
        while (true)
        {
            {
                unique_lock<mutex> lock(m_availableLock);
                m_threadCond.wait(lock, [this](){return m_stopped || !m_available; });
                if (m_stopped)
                    return;
            }

            AckCallback successCallback;
            FailCallback failCallback;
            uint8_t repliedData[1024];
            size_t repliedLength;

            do
            {
                m_tryTimes--;
                // wait for reply
                unique_lock<mutex> lock(m_repliedLock);
                bool success = m_repliedCond.wait_for(lock, chrono::milliseconds(m_timeout),
                    [this](){return m_stopped || m_replied; });
                if (m_stopped)
                    return;

                if (success && m_replyFunc != nullptr)
                {
                    repliedLength = m_repliedLength; 
                    memcpy(repliedData, m_repliedData, repliedLength); 
                    successCallback = move(m_replyFunc);
                    break;
                }
                else if (!success && m_timeoutCallback != nullptr && m_tryTimes == 0)
                {
                    failCallback = move(m_timeoutCallback);
                    break;
                }

            } while (m_tryTimes > 0);

            // reset the session status
            {
                lock_guard<mutex> lock(m_repliedLock);
                m_replied = false;
            }
            {
                lock_guard<mutex> lock(m_availableLock);
                m_available = true;
            }

            // handle reply
            if (successCallback)
            {
                successCallback(repliedData, repliedLength); 
            }
            else if (failCallback)
            {
                failCallback(); 
            }
        }
    });
}

Session::~Session()
{
    m_stopped = true;
    m_waitingThread.join();

}

LinkLayer::LinkLayer() : m_serialPort(NULL), m_recvFSM(NULL),
m_noReplyHandler(nullptr), m_replyHandler(nullptr), 
m_recvCallbackThreadPool(RECV_CALLBACK_THREAD_POOL_SIZE)
{
    m_serialPort = new serial::Serial();
    m_recvFSM = new FSM::StateMachine();

    m_currentSeqNum = numeric_limits<uint16_t>::max(); // the next available seq num is 0

    m_stopped = false;
}

LinkLayer::~LinkLayer()
{
    if (m_recvSharedData)
    {
        delete m_recvSharedData;
        m_recvSharedData = NULL;
    }

    if (m_recvFSM)
    {
        m_recvFSM->delete_all_states();
        delete m_recvFSM;
        m_recvFSM = NULL;
    }
}

void LinkLayer::initialize(string portName, string key)
{
    init_states();
    init_shared_data(key);
    init_serial_port(portName);
    start_serial_port();

    start_send_thread();
    start_recv_thread();
}

void LinkLayer::destroy()
{
    m_serialPort->close(); 
    stop_serial_threads();
}

int LinkLayer::send(uint8_t* data, size_t length)
{
    RawDataPtr rawData = package_data_frame(data, length, 0, 0, false);
    push_raw_data(rawData);
    return 0;
}

int LinkLayer::send(uint8_t* data, size_t length, unsigned int timeoutMs, AckCallback ackFunc, FailCallback failFunc)
{
    Session& session1 = get_session(1);

    uint16_t seqNum = get_next_sequence();

    RawDataPtr rawData = package_data_frame(data, length, 1, seqNum, false);

    push_raw_data(rawData);

    if (!register_session(session1, seqNum, 1, timeoutMs, ackFunc, failFunc))
        return -1;

    return 0;
}

int LinkLayer::send(uint8_t* data, size_t length, unsigned int timeoutMs, unsigned int retryTimes, AckCallback ackFunc, FailCallback failFunc)
{
    uint16_t seqNum = get_next_sequence();

    unsigned int i = 2;
    Session* session;
    for (; i < 32; i++)
    {
        session = &get_session(i);
        if (register_session(*session, seqNum, retryTimes + 1, timeoutMs, ackFunc, failFunc))
            break;
    }

    if (i == 32)
    {
        cerr << "All sessions are busy. " << endl;
        return -1;
    }

    RawDataPtr rawData = package_data_frame(data, length, i, seqNum, false);
    push_raw_data(rawData);
    return 0;
}

void LinkLayer::set_request_handler(RequestHandler handler)
{
    lock_guard<mutex> lock(m_noReplyHandlerLock); 
    m_noReplyHandler = handler; 
}

void LinkLayer::set_reply_request_handler(ReplyRequestHandler handler)
{
    lock_guard<mutex> lock(m_replyHandlerLock);
    m_replyHandler = handler;
}

void LinkLayer::init_states()
{
    assert(m_recvFSM != NULL);
    m_recvFSM->set_init_state(new WaitForHeaderState(m_recvFSM));
    new WaitForBodyState(m_recvFSM);
    new CompleteState(m_recvFSM);
}

void LinkLayer::init_shared_data(string key)
{
    m_recvSharedData = new LinkLayerSharedData();
    memset(m_recvSharedData->m_recvBuffer, 0, _SDK_MAX_RECV_SIZE);
    m_recvSharedData->m_recvIndex = 0;

    init_key(key);

    m_recvFSM->set_shared_data(m_recvSharedData);
}

void LinkLayer::init_key(string key)
{
    assert(key.length() == 64);

    unsigned int byte;
    for (int i = 0; i < 32; i++)
    {
#if defined(_WIN32) && !defined(__MINGW32__)
        sscanf_s(key.substr(2 * i, 2).c_str(), "%x", &byte);
#else
        sscanf(key.substr(2 * i, 2).c_str(), "%x", &temp8);
#endif
        m_recvSharedData->m_key[i] = byte;
    }
}

LinkLayerRecvState* LinkLayer::get_current_state()
{
    FSM::State* state = m_recvFSM->get_active_state();
    return dynamic_cast<LinkLayerRecvState*>(state);
}

void LinkLayer::init_serial_port(string portName)
{
    assert(m_serialPort != NULL);

    m_serialPort->setPort(portName);
    m_serialPort->setBaudrate(115200);

    m_serialPort->setTimeout(serial::Timeout::max(), SERIAL_TIMEOUT, 0, SERIAL_TIMEOUT, 0);

    m_serialPort->setBytesize(serial::eightbits);
    m_serialPort->setParity(serial::parity_none);
    m_serialPort->setStopbits(serial::stopbits_one);
}

int LinkLayer::start_serial_port()
{
    try
    {
        m_serialPort->open();
        m_serialPort->flush();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        return -1;
    }
    return 0;
}

Session& LinkLayer::get_session(int index)
{
    return m_sessions[index];
}

bool LinkLayer::register_session(Session& session, uint16_t seqNum, int tryTimes, unsigned int timeout, AckCallback ackFunc, FailCallback failFunc)
{
    // check availability
    if (!session.m_availableLock.try_lock())
    {
        return false;
    }

    if (!session.m_available)
    {
        session.m_availableLock.unlock();
        return false;
    }

    session.m_available = false;
    session.m_seqNum = seqNum;
    session.m_tryTimes = tryTimes;
    session.m_timeout = timeout;
    session.m_replyFunc = ackFunc;
    session.m_timeoutCallback = failFunc;

    session.m_availableLock.unlock();
    session.m_threadCond.notify_one();

    return true;
}

void LinkLayer::send_raw_data_loop()
{
    while (!m_stopped)
    {
        RawDataPtr rawData;
        m_sendQueue.wait_and_pop(rawData);

        if (m_stopped || !rawData)
            break;

        m_serialPort->write(rawData->m_data, rawData->m_length);
    }

    // clear the queue
    RawDataPtr dummy;
    while (m_sendQueue.try_pop(dummy));
}

void LinkLayer::start_send_thread()
{
    m_sendThread = thread(bind(&LinkLayer::send_raw_data_loop, this));
}

void LinkLayer::push_raw_data(RawDataPtr rawData)
{
    m_sendQueue.push(rawData);
}

RawDataPtr LinkLayer::package_data_frame(uint8_t* data, size_t length, unsigned int sessionNum, uint16_t seqNum, bool ack)
{
    assert(length < 1024);
    // calculate the length after package
    size_t frameSize;
    if (length == 0) // header only
    {
        frameSize = sizeof(SDKHeader);
    }
    else
    {
        frameSize = sizeof(SDKHeader)+_SDK_CRC_DATA_SIZE + length;
    }

    // we assume all the data is encrypted
    frameSize += 16 - length % 16;

    RawDataPtr frame = RawDataPtr(new RawData(NULL, frameSize));

    SDKHeader* header = (SDKHeader*)frame->m_data;

    header->sof = _SDK_SOF;
    header->length = frameSize;
    header->version = 0;
    header->session_id = sessionNum;
    header->is_ack = ack ? 1 : 0;

    header->reserved0 = 0;

    header->padding = length > 0 ? (16 - length % 16) : 0;
    header->enc_type = 1;
    header->reserved1 = 0;

    header->sequence_number = seqNum;

    calc_crc_for_header(frame);

    if (data != NULL && length > 0)
    {
        memcpy(frame->m_data + sizeof(SDKHeader), data, length);
        int res = encrypt_frame(frame);
        if (res != 0)
        {
            cerr << "Error occurs when encrypting the frame. " << endl;
        }

        calc_crc_for_frame_tail(frame);
    }

    return frame;
}

const static unsigned short CRC_INIT = 0x3AA3;
const static unsigned int _SDK_HEAD_DATA_LEN = sizeof(SDKHeader)-2;

int LinkLayer::calc_crc_for_header(RawDataPtr frame)
{
    SDKHeader* header = (SDKHeader*)frame->m_data;
    unsigned int i;
    unsigned short wCRC = CRC_INIT;

    unsigned int length = _SDK_HEAD_DATA_LEN;

    for (i = 0; i < length; i++)
    {
        wCRC = crc16_update(wCRC, frame->m_data[i]);
    }

    header->head_crc = wCRC;
    return 0;
}

int LinkLayer::encrypt_frame(RawDataPtr frame)
{
    SDKHeader* header = (SDKHeader*)frame->m_data;

    size_t frameLength = frame->m_length;

    if (header->enc_type == 0) // dont need decryption
        return -1;

    if (frameLength == sizeof(SDKHeader)) // nothing in the body
        return -1;

    assert(frameLength > sizeof(SDKHeader)+_SDK_CRC_DATA_SIZE);

    unsigned char* data = (unsigned char*)header + sizeof(SDKHeader);
    size_t dataLength = frameLength - _SDK_CRC_DATA_SIZE - sizeof(SDKHeader);
    unsigned int loopTimes = (unsigned int) dataLength / 16;

    aes256_context ctx;
    aes256_init(&ctx, m_recvSharedData->m_key);
    for (unsigned int i = 0; i < loopTimes; i++)
    {
        aes256_encrypt_ecb(&ctx, data);
        data += 16;
    }
    aes256_done(&ctx);

    return 0;
}

int LinkLayer::calc_crc_for_frame_tail(RawDataPtr frame)
{
    SDKHeader* header = (SDKHeader*)frame->m_data;
    size_t frameLength = frame->m_length;
    unsigned int wCRC = CRC_INIT;

    size_t length = frameLength - _SDK_CRC_DATA_SIZE;

    for (unsigned int i = 0; i < length; i++)
    {
        wCRC = crc32_update(wCRC, frame->m_data[i]);
    }

    *((unsigned int*)(frame->m_data + length)) = wCRC;

    return 0;
}

void LinkLayer::recv_raw_data_loop()
{
    static const size_t bufSize = 1024;
    uint8_t recvBuffer[bufSize];
    size_t readLength;
    LinkLayerRecvState* state;
    while (!m_stopped)
    {
        readLength = m_serialPort->read(recvBuffer, bufSize);
        for (size_t i = 0; i < readLength; i++)
        {
            state = get_current_state();
            state->on_recv_byte(recvBuffer[i]);
            state = get_current_state();

            string stateID = state->get_id();
            if (stateID == COMPLETE_STATE_ID)
            {
                uint8_t* data;
                unsigned int dataLength = get_data_and_length(data);
                SDKHeader* header = get_header();

                dispatch_data(header, data, dataLength);

                state->on_reset();
            }
        }
    }
}

void LinkLayer::start_recv_thread()
{
    m_recvThread = thread(bind(&LinkLayer::recv_raw_data_loop, this));
}

SDKHeader* LinkLayer::get_header()
{
    SDKHeader* header = (SDKHeader*)m_recvSharedData->m_recvBuffer;
    return header;
}

unsigned int LinkLayer::get_data_and_length(uint8_t*& data)
{
    SDKHeader* header = get_header();
    unsigned int dataLength = header->length - sizeof(SDKHeader);
    if (dataLength > 0)
        dataLength -= _SDK_CRC_DATA_SIZE;

    data = m_recvSharedData->m_recvBuffer + sizeof(SDKHeader);
    return dataLength;
}

void LinkLayer::dispatch_data(SDKHeader* header, uint8_t* data, unsigned int length)
{
    unsigned int sessionId = header->session_id;
    unsigned int seqNum = header->sequence_number;
    unsigned int isACK = header->is_ack; 
    if (isACK == 1 && sessionId > 0)
    {
        dispatch_to_session(sessionId, seqNum, data, length); 
    }
    else if (isACK == 0 && sessionId == 0)
    {
        lock_guard<mutex> lock(m_noReplyHandlerLock); 
        if (m_noReplyHandler)
            dispatch_to_no_reply_handler(data, length); 
    }
}

int LinkLayer::dispatch_to_session(unsigned int sessionId, unsigned int seqNum, uint8_t* data, size_t length)
{
    Session& session = get_session(sessionId);
    {
        lock_guard<mutex> availableLock(session.m_availableLock);
        if (session.m_available)
        {
            cerr << "The session is not waiting for a reply. " << endl;
            return -1;
        }

        if (session.m_seqNum != seqNum)
        {
            cerr << "Sequence does not match. " << endl;
            return -1;
        }
    }

    {
        lock_guard<mutex> lock(session.m_repliedLock);
        memcpy(session.m_repliedData, data, length);
        session.m_repliedLength = length;
        session.m_replied = true;
    }

    session.m_repliedCond.notify_one();
    return 0; 
}

int LinkLayer::dispatch_to_no_reply_handler(uint8_t* data, size_t length)
{
    assert(data != NULL && length > 0); 
    uint8_t* tempData = (uint8_t*)malloc(length); 
    memcpy(tempData, data, length); 
    
    m_recvCallbackThreadPool.enqueue(
        [this, tempData, length](){
        m_noReplyHandler(tempData, length); 
        free(tempData); 
        return 0; });

    return 0; 
}

void LinkLayer::stop_serial_threads()
{
    m_stopped = true;
    m_sendQueue.push(RawDataPtr(NULL));
    m_sendThread.join();
    m_recvThread.join();
}

uint16_t LinkLayer::get_next_sequence()
{
    // TODO: thread-safety
    if (m_currentSeqNum == numeric_limits<uint16_t>::max())
        m_currentSeqNum = 0;
    else
        m_currentSeqNum++;
    return m_currentSeqNum;
}
