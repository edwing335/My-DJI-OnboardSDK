#ifndef APPLAYERTYPES_H_INCLUDED
#define APPLAYERTYPES_H_INCLUDED

namespace DJIOnboardSDK{

	struct ActivationData
	{
		unsigned int	m_appID;
		unsigned int	m_apiLevel;
		unsigned int	m_apiVersion;
		unsigned char	m_buddleId[32];
	};

    struct FlightModeData
    {
        unsigned char			m_cmdSeq;
        unsigned char			m_flightMode;
    };

}


#endif // !APPLAYERTYPES_H_INCLUDED
