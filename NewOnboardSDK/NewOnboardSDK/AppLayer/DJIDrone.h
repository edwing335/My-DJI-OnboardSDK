#ifndef DJIDRONE_H_INCLUDED
#define DJIDRONE_H_INCLUDED

#include "LinkLayer/LinkLayer.h"
#include "AppLayerTypes.h"

namespace DJIOnboardSDK
{
	class DJIDrone
	{
	public:
		DJIDrone();
		~DJIDrone();

        void initialize(); 

        int activate(); 
        int obtain_control(); 
        int release_control(); 
        int take_off(); 
        int land(); 

	private:
		void init_link_layer(); 
		void init_activation_data(); 

        void prepare_cmd_data(unsigned char cmdSet, unsigned char cmdId, uint8_t* data, size_t length, uint8_t* dest, size_t& newSize); 
        int send_cmd_data(uint8_t* cmdData, size_t length, uint8_t* ackBuffer, size_t& ackLength); 

        int handle_activation_ack(uint8_t* ackData, size_t length);

        int change_control_authorization(unsigned char controlFlag);
        int handle_control_authorization_ack(uint8_t* ackData, size_t length);

        int change_flight_mode(unsigned char flightModeType);
        int start_flight_mode_cmd(unsigned char flightModeType, unsigned char seqNum); 
        int handle_flight_mode_ack(uint8_t* ackData, size_t length); 
        short get_flight_mode_cmd_status(unsigned char seqNum); 
        int handle_flight_mode_cmd_status_ack(uint8_t* ackData, size_t length); 

        int wait_for_take_off_finish(); 
        int wait_for_landing_finish(); 

        unsigned char get_next_cmd_seq(); 

	private: 
		ActivationData m_activationData; 
		LinkLayer m_linkLayer; 

        bool m_initialized; 
        unsigned char m_cmdSeqCnt; 
	};
}

#endif // !DJIDRONE_H_INCLUDED
