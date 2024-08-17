#pragma once
#include "h4amc_config.h"
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <memory>
#include <H4Tools.h>
#include <set>
#define _H4AMC_PRINTF(...) _H4T_PRINTF(__VA_ARGS__)
#if H4AMC_DEBUG
#define H4AMC_PRINTF(...) _H4AMC_PRINTF(__VA_ARGS__)
    template<int I, typename... Args>
    void H4AMC_PRINT(const char* fmt, Args... args) {
        if (H4AMC_DEBUG >= I) H4AMC_PRINTF(std::string(std::string("H4AMC:%d: H=%u M=%u ")+fmt).c_str(),I,_HAL_freeHeap(),_HAL_maxHeapBlock(),args...);
    }
    #define H4AMC_PRINT1(...) H4AMC_PRINT<1>(__VA_ARGS__)
    #define H4AMC_PRINT2(...) H4AMC_PRINT<2>(__VA_ARGS__)
    #define H4AMC_PRINT3(...) H4AMC_PRINT<3>(__VA_ARGS__)
    #define H4AMC_PRINT4(...) H4AMC_PRINT<4>(__VA_ARGS__)

    template<int I>
    void h4amcdump(const uint8_t* p, size_t len) { if (H4AMC_DEBUG >= I) dumphex(p,len); }
    #define H4AMC_DUMP3(p,l) h4amcdump<3>((p),l)
    #define H4AMC_DUMP4(p,l) h4amcdump<4>((p),l)
#else
	#define H4AMC_PRINTF(...)
    #define H4AMC_PRINT1(...)
    #define H4AMC_PRINT2(...)
    #define H4AMC_PRINT3(...)
    #define H4AMC_PRINT4(...)

    #define H4AMC_DUMP3(...)
    #define H4AMC_DUMP4(...)
#endif

enum H4AMC_FAILURE {
    H4AMC_CONNECT_FAIL= H4AMC_ERROR_BASE,
    H4AMC_SUBSCRIBE_FAIL,
	H4AMC_UNSUBSCRIBE_FAIL,
    H4AMC_INBOUND_QOS_ACK_FAIL,
    H4AMC_OUTBOUND_QOS_ACK_FAIL,
    H4AMC_INBOUND_PUB_TOO_BIG,
    H4AMC_OUTBOUND_PUB_TOO_BIG,
    H4AMC_BOGUS_PACKET,
    H4AMC_X_INVALID_LENGTH,
    H4AMC_USER_LOGIC_ERROR,
    H4AMC_NOT_RUNNING,
    H4AMC_NO_SSL,
#if MQTT5
    H4AMC_SERVER_DISCONNECT,
	H4AMC_NO_SERVEROPTIONS,
    H4AMC_PUBACK_FAIL,
    H4AMC_PUBREC_FAIL,
    H4AMC_PUBREL_FAIL,
    H4AMC_PUBCOMP_FAIL,
    H4AMC_SERVER_RETAIN_UNAVAILABLE,
    H4AMC_SHARED_SUBS_UNAVAILABLE,
    H4AMC_BAD_SHARED_TOPIC,
    H4AMC_BAD_TOPIC,
    H4AMC_WILDCARD_UNAVAILABLE,
    H4AMC_ASSIGNED_CLIENTID,
    H4AMC_NO_AUTHENTICATOR,
    H4AMC_INVALID_AUTH_METHOD,
    H4AMC_SUBID_NOT_FOUND,
    H4AMC_PROTOCOL_ERROR,
    H4AMC_INCOMPLETE_PACKET,
#endif
//    H4AMC_KEEPALIVE_TOO_LONG,
    H4AMC_ERROR_MAX
};
enum PacketHeader :uint8_t {
    CONNECT     = 0x10, // x
    CONNACK     = 0x20, // x
    PUBLISH     = 0x30, // x
    PUBACK      = 0x40, // x
    PUBREC      = 0x50, 
    PUBREL      = 0x62,
    PUBCOMP     = 0x70,
    SUBSCRIBE   = 0x82, // x
    SUBACK      = 0x90, // x
    UNSUBSCRIBE = 0xa2, // x
    UNSUBACK    = 0xb0, // x
    PINGREQ     = 0xc0, // x
    PINGRESP    = 0xd0, // x
    DISCONNECT  = 0xe0
#if MQTT5
    ,
    AUTH        = 0xf0
#endif
};
#if MQTT5
enum H4AMC_MQTT_ReasonCode : uint8_t {
    REASON_SUCCESS                          = 0x00, // | CONNACK, PUBACK, PUBREC, PUBREL, PUBCOMP, UNSUBACK, AUTH (S)
    REASON_NORMAL_DISCONNECTION             = 0x00, // | DISCONNECT (C/S)
    REASON_GRANTED_QOS_0                    = 0x00, // | SUBACK
    REASON_GRANTED_QOS_1                    = 0x01, // | SUBACK
    REASON_GRANTED_QOS_2                    = 0x02, // | SUBACK
    REASON_DISCONNECT_WITH_WILL_MESSAGE     = 0x04, // | DISCONNECT (C)
    REASON_NO_MATCHING_SUBSCRIBERS          = 0x10, // | PUBACK, PUBREC
    REASON_NO_SUBSCRIPTION_EXISTED          = 0x11, // | UNSUBACK
    REASON_CONTINUE_AUTHENTICATION          = 0x18, // | AUTH (C/S)
    REASON_RE_AUTHENTICATE                  = 0x19, // | AUTH (C)
    REASON_UNSPECIFIED_ERROR                = 0x80, // | CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT (C/S)
    REASON_MALFORMED_PACKET                 = 0x81, // | CONNACK, DISCONNECT (C/S) | [MP/PE (Malf. Packet / Proto. Error)]
    REASON_PROTOCOL_ERROR                   = 0x82, // | CONNACK, DISCONNECT (C/S) | [MP/PE]
    REASON_IMPLEMENTATION_SPECIFIC_ERROR    = 0x83, // | CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT (C/S)
    REASON_UNSUPPORTED_PROTOCOL_VERSION     = 0x84, // | CONNACK
    REASON_CLIENT_IDENTIFIER_NOT_VALID      = 0x85, // | CONNACK
    REASON_BAD_USER_NAME_OR_PASSWORD        = 0x86, // | CONNACK
    REASON_NOT_AUTHORIZED                   = 0x87, // | CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT (S)
    REASON_SERVER_UNAVAILABLE               = 0x88, // | CONNACK
    REASON_SERVER_BUSY                      = 0x89, // | CONNACK, DISCONNECT (S)
    REASON_BANNED                           = 0x8A, // | CONNACK
    REASON_SERVER_SHUTTING_DOWN             = 0x8B, // | DISCONNECT
    REASON_BAD_AUTHENTICATION_METHOD        = 0x8C, // | CONNACK, DISCONNECT (S)
    REASON_KEEP_ALIVE_TIMEOUT               = 0x8D, // | DISCONNECT (S)
    REASON_SESSION_TAKEN_OVER               = 0x8E, // | DISCONNECT (S)
    REASON_TOPIC_FILTER_INVALID             = 0x8F, // | SUBACK, UNSUBACK, DISCONNECT (S)
    REASON_TOPIC_NAME_INVALID               = 0x90, // | CONNACK, PUBACK, PUBREC, DISCONNECT (C/S)
    REASON_PACKET_IDENTIFIER_IN_USE         = 0x91, // | PUBACK, PUBREC, SUBACK, UNSUBACK
    REASON_PACKET_IDENTIFIER_NOT_FOUND      = 0x92, // | PUBREL, PUBCOMP
    REASON_RECEIVE_MAXIMUM_EXCEEDED         = 0x93, // | DISCONNECT (C/S) | [MP/PE]
    REASON_TOPIC_ALIAS_INVALID              = 0x94, // | DISCONNECT (C/S)
    REASON_PACKET_TOO_LARGE                 = 0x95, // | CONNACK, DISCONNECT (C/S) | [MP/PE]
    REASON_MESSAGE_RATE_TOO_HIGH            = 0x96, // | DISCONNECT (C/S)
    REASON_QUOTA_EXCEEDED                   = 0x97, // | CONNACK, PUBACK, PUBREC, SUBACK, DISCONNECT (C/S)
    REASON_ADMINISTRATIVE_ACTION            = 0x98, // | DISCONNECT (C/S)
    REASON_PAYLOAD_FORMAT_INVALID           = 0x99, // | CONNACK, PUBACK, PUBREC, DISCONNECT (C/S)
    REASON_RETAIN_NOT_SUPPORTED             = 0x9A, // | CONNACK, DISCONNECT (S) | [MP/PE]
    REASON_QOS_NOT_SUPPORTED                = 0x9B, // | CONNACK, DISCONNECT (S) | [MP/PE]
    REASON_USE_ANOTHER_SERVER               = 0x9C, // | CONNACK, DISCONNECT (S)
    REASON_SERVER_MOVED                     = 0x9D, // | CONNACK, DISCONNECT (S)
    REASON_SHARED_SUBSCRIPTIONS_NOT_SUPPORTED = 0x9E, // | SUBACK, DISCONNECT (S) | [MP/PE]
    REASON_CONNECTION_RATE_EXCEEDED         = 0x9F, // | CONNACK, DISCONNECT (S)
    REASON_MAXIMUM_CONNECT_TIME             = 0xA0, // | DISCONNECT (S)
    REASON_SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED = 0xA1, // | SUBACK, DISCONNECT (S) | [MP/PE]
    REASON_WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED = 0xA2 //  |SUBACK, DISCONNECT (S) | [MP/PE]
};

enum Subscription_Options {
    SUBSCRIPTION_OPTION_QoS_SHIFT                     = 0,
    SUBSCRIPTION_OPTION_NO_LOCAL_SHIFT                = 2,
    SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED_SHIFT     = 3,
    SUBSCRIPTION_OPTION_RETAIN_HANDLING_SHIFT         = 4
};

#define H4AMC_PAYLOAD_FORMAT_UNDEFINED      0x00
#define H4AMC_PAYLOAD_FORMAT_STRING         0x01


#else // MQTT5
enum H4AMC_MQTT_ReasonCode : uint8_t {
    REASON_SUCCESS                          = 0x00,
    REASON_NORMAL_DISCONNECTION             = 0x00
};
#endif // MQTT5


using H4AMC_BinaryData            =std::vector<uint8_t>;
namespace H4AMC_Helpers {
    uint8_t* poke8(uint8_t* p,uint8_t u);
    uint8_t* poke16(uint8_t* p,uint16_t u);
    uint16_t peek16(uint8_t* p);
    uint8_t* encodestring(uint8_t* p,const std::string& s);
    std::string decodestring(uint8_t** p);
    uint8_t* encodeBinary(uint8_t* p, const H4AMC_BinaryData&);
    H4AMC_BinaryData decodeBinary(uint8_t** p);
    uint32_t decodeVariableByteInteger(uint8_t** p);
    uint8_t* encodeVariableByteInteger(uint8_t* p, uint32_t value);
    uint8_t varBytesLength(uint32_t value);
}
using PacketID              =uint16_t;
class mqttTraits;
struct H4AMC_MessageOptions;
struct H4AMC_ConnackParam;
using H4AMC_FN_VOID        	=std::function	<void(void)>;
using H4AMC_FN_U8PTR       	=std::function	<void(uint8_t*,uint8_t* base)>;
using H4AMC_FN_U8PTRU8     	=std::function	<uint8_t*(uint8_t*)>;
using H4AMC_FN_STRING       =std::function	<void(const std::string&)>;
using H4AMC_cbError         =std::function	<void(int, int)>;
using H4AMC_cbMessage       =std::function	<void(const char* topic, const uint8_t* payload, size_t len, H4AMC_MessageOptions opts)>;
using H4AMC_cbConnect       =std::function	<void(H4AMC_ConnackParam)>; // in MQTT5
using H4AMC_cbPublish 		=std::function	<void(PacketID)>;
using H4AMC_PACKET_MAP      =std::map		<uint16_t,mqttTraits>; // indexed by messageId
using H4AMC_MEM_POOL        =std::unordered_set	<uint8_t*>;
#if MQTT5
using H4AMC_USER_PROPERTIES =H4T_NVP_MAP;
using H4AMC_FN_DYN_PROPS 	=std::function	<H4AMC_USER_PROPERTIES(PacketHeader)>;
using H4AMC_AuthInformation = std::pair<H4AMC_MQTT_ReasonCode, std::pair<std::string, H4AMC_BinaryData>>;
#endif

#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
struct SubscriptionResource;
using H4AMC_SUBRES_MAP 		=std::map 		<uint32_t, SubscriptionResource>;
using H4AMC_PK_SUBIDS_MAP	=std::map		<uint32_t, uint32_t>; // PACKET ID -> SUB ID
#endif