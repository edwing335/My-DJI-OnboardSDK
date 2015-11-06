#ifndef LINKLAYERSHAREDDATA_H_INCLUDED
#define LINKLAYERSHAREDDATA_H_INCLUDED

#include "Utility/StateMachine.h"
#include "LinkLayerMacros.h"

namespace DJIOnboardSDK
{
    class LinkLayerSharedData : public FSM::SharedData
    {
    public:
        LinkLayerSharedData() {}
        virtual ~LinkLayerSharedData() {}

        unsigned char m_key[32];
        unsigned short m_recvIndex;
        unsigned char m_recvBuffer[_SDK_MAX_RECV_SIZE];
    };
}

#endif
