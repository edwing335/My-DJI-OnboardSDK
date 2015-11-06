#ifndef LINKLAYER_H_INCLUDED
#define LINKLAYER_H_INCLUDED

#include "Utility/WaitQueue.h"
#include "Utility/ThreadPool.h"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <map>
#include <string>
#include <cstring>

namespace serial { class Serial; }

namespace FSM { class StateMachine; }

namespace DJIOnboardSDK
{
    struct RawData
    {
        uint8_t* m_data;
        size_t m_length;

        RawData(uint8_t* data, size_t length) :m_length(length)
        {
            m_data = (uint8_t*)malloc(m_length);
            if (data)
                memcpy(m_data, data, m_length);
        }

        ~RawData() { free(m_data); }
    };
    typedef std::shared_ptr<RawData> RawDataPtr;

    typedef std::function<void(uint8_t*, size_t)> AckCallback;
    typedef std::function<void()> FailCallback;
    typedef std::function<void(uint8_t*, size_t)> RequestHandler;
    typedef std::function<void(uint8_t*, size_t, uint8_t*&, size_t&)> ReplyRequestHandler;

    class Session
    {
    public:
        Session();
        ~Session();

        bool m_available;
        std::mutex m_availableLock;
        uint16_t m_seqNum;
        int m_tryTimes;
        unsigned int m_timeout;

        bool m_replied;
        uint8_t m_repliedData[1024];
        size_t m_repliedLength;
        std::mutex m_repliedLock;
        std::condition_variable m_repliedCond;

        std::condition_variable m_threadCond;
        std::thread m_waitingThread;

        AckCallback m_replyFunc;
        FailCallback m_timeoutCallback;

        bool m_stopped;
    };

    typedef std::map<int, Session> Sessions;

    class LinkLayerSharedData;
    class LinkLayerRecvState;

    struct SDKHeader;

    class LinkLayer
    {
    public:
        LinkLayer();
        virtual ~LinkLayer();

        void initialize(std::string portName, std::string key);
        void destroy();

        // send
        int send(uint8_t* data, size_t length);
        int send(uint8_t* data, size_t length, unsigned int timeoutMs, AckCallback, FailCallback);
        int send(uint8_t* data, size_t length, unsigned int timeoutMs, unsigned int retryTimes, AckCallback, FailCallback);

        // receive
        void set_request_handler(RequestHandler);
        void set_reply_request_handler(ReplyRequestHandler); // not used yet

    private:
        void init_states();
        void init_shared_data(std::string);
        void init_key(std::string);
        LinkLayerRecvState* get_current_state();

        void init_serial_port(std::string);
        int start_serial_port();

        Session& get_session(int);
        bool register_session(Session& session, uint16_t seqNum, int tryTimes, unsigned int timeout, AckCallback, FailCallback);

        uint16_t get_next_sequence();

        void send_raw_data_loop();
        void start_send_thread();
        void push_raw_data(RawDataPtr);
        RawDataPtr package_data_frame(uint8_t* data, size_t length, unsigned int sessionNum, uint16_t seqNum, bool ack);
        int calc_crc_for_header(RawDataPtr frame);
        int encrypt_frame(RawDataPtr frame);
        int calc_crc_for_frame_tail(RawDataPtr frame);

        void recv_raw_data_loop();
        void start_recv_thread();
        inline SDKHeader* get_header();
        unsigned int get_data_and_length(uint8_t*& data);
        void dispatch_data(SDKHeader* header, uint8_t* data, unsigned int length);
        int dispatch_to_session(unsigned int sessionId, unsigned int seqNum, uint8_t* data, size_t length); 
        int dispatch_to_no_reply_handler(uint8_t* data, size_t length); 

        void stop_serial_threads();

    private:
        serial::Serial* m_serialPort;

        Sessions m_sessions;

        WaitQueue<RawDataPtr> m_sendQueue;
        std::thread m_sendThread;

        FSM::StateMachine* m_recvFSM;
        LinkLayerSharedData* m_recvSharedData;
        std::thread m_recvThread;

        std::mutex m_noReplyHandlerLock; 
        RequestHandler m_noReplyHandler;
        std::mutex m_replyHandlerLock;
        ReplyRequestHandler m_replyHandler;
        ThreadPool m_recvCallbackThreadPool; 

        uint16_t m_currentSeqNum;

        std::atomic_bool m_stopped;
    };
}

#endif
