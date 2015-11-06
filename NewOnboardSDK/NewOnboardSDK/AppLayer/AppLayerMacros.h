#ifndef APPLAYERMACROS_H_INCLUDED
#define APPLAYERMACROS_H_INCLUDED

#define MAKE_VERSION(a,b,c,d) (((a << 24)&0xff000000) | ((b << 16)&0x00ff0000) | ((c << 8)&0x0000ff00) | (d&0x000000ff))
#define SDK_VERSION           (MAKE_VERSION(2,3,10,0))

#define ACTIVATION_CMD_SET      0x00
#define CTRL_CMD_SET            0x01


#define API_USER_ACTIVATION     0x01
#define API_CTRL_MANAGEMENT     0x00
#define API_CMD_REQUEST       	0x01
#define API_CMD_STATUS_REQUEST 	0x02

#define SDK_ACTIVATE_SUCCESS                0x0000
#define SDK_ACTIVATE_PARAM_ERROR            0x0001
#define SDK_ACTIVATE_DATA_ENC_ERROR         0x0002
#define SDK_ACTIVATE_NEW_DEVICE             0x0003
#define SDK_ACTIVATE_DJI_APP_NOT_CONNECT    0x0004
#define SDK_ACTIVATE_DIJ_APP_NO_INTERNET    0x0005
#define SDK_ACTIVATE_SERVER_REFUSED         0x0006
#define SDK_ACTIVATE_LEVEL_ERROR            0x0007
#define SDK_ACTIVATE_SDK_VERSION_ERROR      0x0008

#define OBTAIN_CONTROL_BYTE     0x01
#define RELEASE_CONTROL_BYTE    0x00

#define TAKE_OFF_CMD_VAL    0x04
#define LANDING_CMD_VAL     0x06
#define GO_HOME_CMD_VAL     0x01

#define SDK_SUCCESS                         0x0000
#define SDK_ERR_COMMAND_NOT_SUPPORTED       0xFF00
#define SDK_ERR_NO_AUTHORIZED               0xFF01
#define SDK_ERR_NO_RIGHTS                   0xFF02
#define SDK_ERR_NO_RESPONSE                 0xFFFF

#define	REQ_TIME_OUT                0x0000
#define REQ_REFUSE                  0x0001
#define CMD_RECIEVE                 0x0002
#define STATUS_CMD_EXECUTING		0x0003
#define STATUS_CMD_EXE_FAIL         0x0004
#define STATUS_CMD_EXE_SUCCESS		0x0005

#define WRONG_CMD_SEQ_NUM           0x0001
#define SWITCHING_IN_PROGRESS       0x0003
#define SWITCHING_FAILED            0x0004
#define SWITCHING_SUCCEED           0x0005



#endif // !APPLAYERMACROS_H_INCLUDED
