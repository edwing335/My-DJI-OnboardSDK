#ifndef LINKLAYERSTATES_H_INCLUDED
#define LINKLAYERSTATES_H_INCLUDED

#include "Utility/State.h"
#include <string>

namespace DJIOnboardSDK
{
    class LinkLayerSharedData;
    // State IDs
    const static std::string WAIT_FOR_HEADER_STATE_ID = "waiting for the header to be complete";
    const static std::string WAIT_FOR_BODY_STATE_ID = "waiting for the body to be complete";
    const static std::string COMPLETE_STATE_ID = "message is received completely";

    class LinkLayerRecvState : public FSM::State
    {
    public:
        LinkLayerRecvState(std::string, FSM::StateMachine*);
        virtual ~LinkLayerRecvState(){}

        virtual void on_recv_byte(unsigned char byte){}
        virtual void on_reset();

    protected:
        virtual int on_enter() { return 0; }
        virtual int on_exit() { return 0; }

        inline LinkLayerSharedData* get_data();

        // functions shared by more than one child state
        int store_data(unsigned char);
        int get_recv_data_length();
        bool recv_data_complete();
    };

    class WaitForHeaderState : public LinkLayerRecvState
    {
    public:
        WaitForHeaderState(FSM::StateMachine*);
        virtual ~WaitForHeaderState() {}

        virtual void on_recv_byte(unsigned char byte);

    private:
        bool header_complete();
        bool header_correct();
        int correct_header(); // called when header is corrupted
        bool check_header_crc();
    };

    class WaitForBodyState : public LinkLayerRecvState
    {
    public:
        WaitForBodyState(FSM::StateMachine*);
        virtual ~WaitForBodyState() {}

        virtual void on_recv_byte(unsigned char byte);

    private:
        bool recv_data_correct();
        bool check_recv_data_crc();
        int correct_recv_data();
    };

    class CompleteState : public LinkLayerRecvState
    {
    public:
        CompleteState(FSM::StateMachine*);
        virtual ~CompleteState() {}

        virtual void on_recv_byte(unsigned char byte);
        virtual void on_reset();

    protected:
        virtual int on_enter();

    private:
        int decrypt_recv_data();
    };
}

#endif
