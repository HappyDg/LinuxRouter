#define BPDU_PROTOCOL_ID        0x00
#define BPDU_VERSION_ID         0x00
#define BPDU_TC_TYPE            0x80
#define BPDU_CONFIG_TYPE        0x00
#define TC_BIT     0x01
#define TC_ACK_BIT 0x80

#define DISABLED 0
#define LISTENING 1
#define LEARNING 2
#define FORWARDING 3
#define BLOCKING 4

#define STP_ENABLED 1
#define STP_DISABLED 0

#define MESSAGE_AGE_INCR 1

/*802.1D STP compatibility Range*/
#define STP_MIN_BRIDGE_PRIORITY 0
#define STP_MAX_BRIDGE_PRIORITY 65535

#define STP_MIN_PORT_PRIORITY 0
#define STP_MAX_PORT_PRIORITY 240

#define STP_MIN_PATH_COST 1
#define STP_MAX_PATH_COST 200000000

#define STP_MIN_HELLO_TIME 1
#define STP_MAX_HELLO_TIME 10

#define STP_MIN_MAX_AGE  6
#define STP_MAX_MAX_AGE  40

#define STP_MIN_FORWARD_DELAY  2
#define STP_MAX_FORWARD_DELAY 30

enum STP_PROTO_SPEC {
	UNKNOWN = 1,
	DEClB100 = 2,
	IEEE8021D = 3
};

enum STP_DEF_VALUES {
        STP_DEF_PRIORITY = 32768,
        STP_DEF_MAX_AGE = 20,
        STP_DEF_HELLO_TIME = 2,
        STP_DEF_FWD_DELAY = 15,
        STP_DEF_HOLD_COUNT = 6,
        STP_DEF_PORT_PRIO = 128,
        STP_DEF_DESG_COST = 20000,
        STP_DEF_PATH_COST = 20000
};
