/*
Creative Commons: Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

You are free to:

Share — copy and redistribute the material in any medium or format
Adapt — remix, transform, and build upon the material

The licensor cannot revoke these freedoms as long as you follow the license terms. Under the following terms:

Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. 
You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

NonCommercial — You may not use the material for commercial purposes.

ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions 
under the same license as the original.

No additional restrictions — You may not apply legal terms or technological measures that legally restrict others 
from doing anything the license permits.

Notices:
You do not have to comply with the license for elements of the material in the public domain or where your use is 
permitted by an applicable exception or limitation. To discuss an exception, contact the author:

philbowles2012@gmail.com

No warranties are given. The license may not give you all of the permissions necessary for your intended use. 
For example, other rights such as publicity, privacy, or moral rights may limit how you use the material.
*/
#include<H4AsyncMQTT.h>

#if H4AMC_DEBUG
    std::map<uint8_t,char*> mqttTraits::pktnames={
        {0x10,"CONNECT"},
        {0x20,"CONNACK"},
        {0x30,"PUBLISH"},
        {0x40,"PUBACK"},
        {0x50,"PUBREC"},
        {0x60,"PUBREL"},
        {0x70,"PUBCOMP"},
        {0x80,"SUBSCRIBE"},
        {0x90,"SUBACK"},
        {0xa0,"UNSUBSCRIBE"},
        {0xb0,"UNSUBACK"},
        {0xc0,"PINGREQ"},
        {0xd0,"PINGRESP"},
        {0xe0,"DISCONNECT"}
#if MQTT5
        ,{0xf0,"AUTH"}
#endif
    };

#if MQTT5
    std::map<H4AMC_MQTT_ReasonCode,char*> mqttTraits::rcnames {
        {REASON_SUCCESS, "Success/Normal disconnection/QoS0"},
        {REASON_GRANTED_QOS_0, "Granted QoS 0"},
        {REASON_GRANTED_QOS_1, "Granted QoS 1"},
        {REASON_GRANTED_QOS_2, "Granted QoS 2"},
        {REASON_DISCONNECT_WITH_WILL_MESSAGE, "Disconnect with Will Message"},
        {REASON_NO_MATCHING_SUBSCRIBERS, "No Matching Subscribers"},
        {REASON_NO_SUBSCRIPTION_EXISTED, "No Subscription Existed"},
        {REASON_CONTINUE_AUTHENTICATION, "Continue Authentication"},
        {REASON_RE_AUTHENTICATE, "Re-Authenticate"},
        {REASON_UNSPECIFIED_ERROR, "Unspecified Error"},
        {REASON_MALFORMED_PACKET, "Malformed Packet"},
        {REASON_PROTOCOL_ERROR, "Protocol Error"},
        {REASON_IMPLEMENTATION_SPECIFIC_ERROR, "Implementation Specific Error"},
        {REASON_UNSUPPORTED_PROTOCOL_VERSION, "Unsupported Protocol Version"},
        {REASON_CLIENT_IDENTIFIER_NOT_VALID, "Client Identifier not valid"},
        {REASON_BAD_USER_NAME_OR_PASSWORD, "Bad User Name or Password"},
        {REASON_NOT_AUTHORIZED, "Not Authorized"},
        {REASON_SERVER_UNAVAILABLE, "Server Unavailable"},
        {REASON_SERVER_BUSY, "Server Busy"},
        {REASON_BANNED, "Banned"},
        {REASON_SERVER_SHUTTING_DOWN, "Server Shutting Down"},
        {REASON_BAD_AUTHENTICATION_METHOD, "Bad Authentication Method"},
        {REASON_KEEP_ALIVE_TIMEOUT, "Keep Alive Timeout"},
        {REASON_SESSION_TAKEN_OVER, "Session Taken Over"},
        {REASON_TOPIC_FILTER_INVALID, "Topic Filter Invalid"},
        {REASON_TOPIC_NAME_INVALID, "Topic Name Invalid"},
        {REASON_PACKET_IDENTIFIER_IN_USE, "Packet Identifier In Use"},
        {REASON_PACKET_IDENTIFIER_NOT_FOUND, "Packet Identifier Not Found"},
        {REASON_RECEIVE_MAXIMUM_EXCEEDED, "Receive Maximum Exceeded"},
        {REASON_TOPIC_ALIAS_INVALID, "Topic Alias Invalid"},
        {REASON_PACKET_TOO_LARGE, "Packet Too Large"},
        {REASON_MESSAGE_RATE_TOO_HIGH, "Message Rate Too High"},
        {REASON_QUOTA_EXCEEDED, "Quota Exceeded"},
        {REASON_ADMINISTRATIVE_ACTION, "Administrative Action"},
        {REASON_PAYLOAD_FORMAT_INVALID, "Payload Format Invalid"},
        {REASON_RETAIN_NOT_SUPPORTED, "Retain Not Supported"},
        {REASON_QOS_NOT_SUPPORTED, "QoS Not Supported"},
        {REASON_USE_ANOTHER_SERVER, "Use Another Server"},
        {REASON_SERVER_MOVED, "Server Moved"},
        {REASON_SHARED_SUBSCRIPTIONS_NOT_SUPPORTED, "Shared Subscriptions Not Supported"},
        {REASON_CONNECTION_RATE_EXCEEDED, "Connection Rate Exceeded"},
        {REASON_MAXIMUM_CONNECT_TIME, "Maximum Connect Time"},
        {REASON_SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED, "Subscription Identifiers Not Supported"},
        {REASON_WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED, "Wildcard Subscriptions Not Supported"}
    };

std::map<H4AMC_MQTT5_Property,char*> mqttTraits::propnames {
    {PROPERTY_PAYLOAD_FORMAT_INDICATOR, "Payload Format Indicator"},
    {PROPERTY_MESSAGE_EXPIRY_INTERVAL, "Message Expiry Interval"},
    {PROPERTY_CONTENT_TYPE, "Content Type"},
    {PROPERTY_RESPONSE_TOPIC, "Response Topic"},
    {PROPERTY_CORRELATION_DATA, "Correlation Data"},
    {PROPERTY_SUBSCRIPTION_IDENTIFIER, "Subscription Identifier"},
    {PROPERTY_SESSION_EXPIRY_INTERVAL, "Session Expiry Interval"},
    {PROPERTY_ASSIGNED_CLIENT_IDENTIFIER, "Assigned Client Identifier"},
    {PROPERTY_SERVER_KEEP_ALIVE, "Server Keep Alive"},
    {PROPERTY_AUTHENTICATION_METHOD, "Authentication Method"},
    {PROPERTY_AUTHENTICATION_DATA, "Authentication Data"},
    {PROPERTY_REQUEST_PROBLEM_INFORMATION, "Request Problem Information"},
    {PROPERTY_WILL_DELAY_INTERVAL, "Will Delay Interval"},
    {PROPERTY_REQUEST_RESPONSE_INFORMATION, "Request Response Information"},
    {PROPERTY_RESPONSE_INFORMATION, "Response Information"},
    {PROPERTY_SERVER_REFERENCE, "Server Reference"},
    {PROPERTY_REASON_STRING, "Reason String"},
    {PROPERTY_RECEIVE_MAXIMUM, "Receive Maximum"},
    {PROPERTY_TOPIC_ALIAS_MAXIMUM, "Topic Alias Maximum"},
    {PROPERTY_TOPIC_ALIAS, "Topic Alias"},
    {PROPERTY_MAXIMUM_QOS, "Maximum QoS"},
    {PROPERTY_RETAIN_AVAILABLE, "Retain Available"},
    {PROPERTY_USER_PROPERTY, "User Property"},
    {PROPERTY_MAXIMUM_PACKET_SIZE, "Maximum Packet Size"},
    {PROPERTY_WILDCARD_SUBSCRIPTION_AVAILABLE, "Wildcard Subscription Available"},
    {PROPERTY_SUBSCRIPTION_IDENTIFIER_AVAILABLE, "Subscription Identifier Available"},
    {PROPERTY_SHARED_SUBSCRIPTION_AVAILABLE, "Shared Subscription Available"}
};
#else
    std::map<uint8_t,char*> mqttTraits::connacknames={
        {0x00,"Connection Accepted"},
        {0x01,"Connection Refused, unacceptable protocol version"},
        {0x02,"Connection Refused, identifier rejected"},
        {0x03,"Connection Refused, Server unavailable"},
        {0x04,"Connection Refused, bad user name or password"},
        {0x05,"Connection Refused, not authorized"}
    };
    std::map<uint8_t,char*> mqttTraits::subacknames={
        {0x00,"QoS 0"},
        {0x01,"QoS 1"},
        {0x02,"QoS 2"},
        {0x80,"Failure"}
    };
#endif
#endif

std::string mqttTraits::_decodestring(uint8_t** p){
    return H4AMC_Helpers::decodestring(p);
}

mqttTraits::mqttTraits(uint8_t* p,size_t s): data(p){ // Properties .. topic alias...
    type=data[0];
    flags=(data[0] & 0xf);
//  CALCULATE RL
    uint32_t multiplier = 1;
    uint8_t encodedByte;//,rl=0;
    uint8_t* pp=&data[1];
    do{
        encodedByte = *pp++;
        offset++; //** Remaining length field
        remlen += (encodedByte & 0x7f) * multiplier;
        multiplier <<= 7;//** multiplier *= 128;
        if (multiplier > 128*128*128) //** if (offset>3)
        {
            H4AMC_PRINT1("Malformed Packet!!, remlen=%d\n",remlen);
            // v3.1.1 #4.8 "If the Client or Server encounters a Transient Error while processing an inbound Control Packet it MUST close the Network Connection on which it received that Control Packet"
            // v5.0 "When a Client detects a Malformed Packet or Protocol Error, and a Reason Code is given in the specification, it SHOULD close the Network Connection."
            packet_state = PacketState::MALFORMED;
            return;
        }
    } while ((encodedByte & 0x80) != 0);
    len=1+offset+remlen; //** Fixed header (Packet Type and Flags + Remaining Length field) + Remaining Length
    H4AMC_PRINT4("PACKET SIZE=%d s=%d\n",len,s);
    if(s > len){
        H4AMC_PRINT4("STUFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFED PACKET!!! %d > %d\n",s,len);
        next.first=p+len;
        next.second=s-len;
    } else if (len > s) {
        H4AMC_PRINT1("INCOMPLETE PACKET! rcv %d packetlen %d\n", s, len);
    }
/*
#if H4AMC_DEBUG
    if(s!=1+offset+remlen) H4AMC_PRINT3("SANITY FAIL! s=%d RL=%d offset=%d L=%d\n",s,remlen,offset,1+offset+remlen);
    else H4AMC_PRINT3("LL s=%d RL=%d offset=%d L=%d\n",s,remlen,offset,1+offset+remlen);
#endif
*/
    auto i = start();
    uint8_t* ii; // helping pointer
    /* Parse Variable Header */
    switch(type){
        case CONNACK:
            // conackflags=i[0];
            conackflags=_parse8(&i);
            // reasoncode=i[1];
#if MQTT5
            // if (i[2]) initiateProperties(&i[2]);
        case DISCONNECT:
        case AUTH:
            if (remlen){
                reasoncode = _parse8(&i);
                if (remlen>=2 && *i){// remlen>=2 is always true for CONNACK, also should *i
                    initiateProperties(i);
                }
            }
#endif
            break;
        case PUBACK:
        case PUBREC:
        case PUBREL:
        case PUBCOMP:
#if MQTT5
            id=_peek16(i);
            if (remlen>2){
                reasoncode = i[2];
                if (remlen>=4 && i[3]) initiateProperties(&i[3]);//discard protocolpayload pointer
            }
            break;
#endif
        case SUBSCRIBE:
        case SUBACK:
        case UNSUBSCRIBE:
        case UNSUBACK:
            id=_peek16(&i[0]);
#if MQTT5
            if (i[2]) {
                auto ret = initiateProperties(&i[2]);
                if (!ret.first) protocolpayload=ret.second;
            } else protocolpayload=&i[3];
#else
            protocolpayload=&i[2];
#endif
            if (type == SUBACK || type == UNSUBACK) {
                int reasons = data+len-protocolpayload;
                for (int ri=0;ri<reasons;ri++){
                    subreasons.push_back(protocolpayload[ri]);
                }
            }
            break;
        default:
            {
                if(isPublish()){
                    retain=flags & 0x01;
                    dup=(flags & 0x8) >> 3;
                    qos=(flags & 0x6) >> 1;
                    ii=i; // Work on a copy of original i=start();
                    topic=_decodestring(&ii);
#if MQTT5
                    if (!topic.length()) // For Inserting the topic out of Topic Alias.
                        _topic_index=i-data; // Stands on Topic length.
#endif
                    if(qos){ id=_peek16(ii);ii+=2; }
                    protocolpayload=ii;
#if MQTT5
                    auto ret = initiateProperties(ii);
                    protocolpayload = ret.second; // or ii
                    H4AMC_PRINT4("FOUND PROPS\n");
                    if (ret.first == REASON_SUCCESS) {
                        _topic_alias = properties->getNumericProperty(PROPERTY_TOPIC_ALIAS);
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
                        auto subIds = properties->getNumericProperties(PROPERTY_SUBSCRIPTION_IDENTIFIER);
                        subscription_ids = std::set<uint32_t>{subIds.begin(), subIds.end()};
#endif
                    } else {
                        packet_state = PacketState::MALFORMED;
                    }
#endif
                    payload=protocolpayload;
                    plen=data+len-protocolpayload;
                    // [ ] compare remlen with plen
                }
            }
            break;
    }

#if H4AMC_DEBUG
    H4AMC_PRINT1("MQTT %s\n",getPktName().data());
    H4AMC_PRINT2(" Data @ %p len=%d RL=%d\n",data,len,remlen);
    H4AMC_DUMP4(data,len);
    switch(type){
        case PINGRESP: break;
        case PUBACK:
        case PUBREC:
        case PUBREL:
        case PUBCOMP:
        case SUBACK:
        case UNSUBACK:
            H4AMC_PRINT3("  id: %d\n",id);
#if MQTT5
            if (type==SUBACK || type==UNSUBACK) {
                H4AMC_PRINT3("  Reason Codes:\t");
                int count=subreasons.size();
                for (auto &rc:subreasons){
                    bool last=count==1;
                    H4AMC_PRINTF("%02X%s", rc, last?"":",");
                    count--;
                }
            } else
                H4AMC_PRINT3("  reasoncode: %d\n",reasoncode);
#endif
            break;
        case CONNECT:
            {
                uint8_t cf=*(i+7);
                H4AMC_PRINT3("  Protocol: %s\n",*(i+6)==0x4 ? "3.1.1":stringFromInt(data[8],"0x%02x").data());
                H4AMC_PRINT4("  Flags: 0x%02x\n",cf);
                H4AMC_PRINT3("  Session: %s\n",((cf & CLEAN_START) >> 1) ? "Clean":"Dirty");
                H4AMC_PRINT3("  Keepalive: %d\n",_peek16(i+8));
                uint8_t* sp=i+10;
#if MQTT5
                // Connect Properties.
                auto ret = initiateProperties(sp);
                if (!ret.first) sp=ret.second;
                
#endif
                H4AMC_PRINT3("  ClientId: %p %s\n",sp,_decodestring(&sp).data());
                if(cf & WILL){
#if MQTT5
                    MQTT_Properties will_props;
                    auto ret = will_props.parseProperties(sp);
                    if (!ret.first) sp=ret.second;
                    H4AMC_PRINT3("  Will Properties:\n");
                    will_props.dump();
#endif
                    H4AMC_PRINT3("  Will Topic: %p %s\n",sp,_decodestring(&sp).data());
                    if(cf & WILL_RETAIN) H4AMC_PRINT3("  Will: RETAIN\n");
                    H4AMC_PRINT3("  Will QoS: %d\n",(cf >> 3) &0x3);
                    H4AMC_PRINT3("  Will Message: %p %s\n",sp,_decodestring(&sp).data());
                }
                if(cf & USERNAME) H4AMC_PRINT3("  Username: %s\n",_decodestring(&sp).data());
                if(cf & PASSWORD) H4AMC_PRINT3("  Password: %s\n",_decodestring(&sp).data());
                break;
            }
        case CONNACK:
            {
#if MQTT5
                H4AMC_PRINT3("  %s%s\n", reasoncode?"Error: " : "", rcnames[static_cast<H4AMC_MQTT_ReasonCode>(reasoncode)]);
#else
                H4AMC_PRINT3("  %s%s\n", reasoncode?"Error: " : "", connacknames[reasoncode]);
#endif
                if (!reasoncode) {
                    H4AMC_PRINT3("  Session: %s\n",(conackflags & 1) ? "Present":"None");
                }
            }
#if MQTT5
        case DISCONNECT:
        case AUTH:
            H4AMC_PRINT3("  ReasonCode %02X\n", reasoncode);
#endif
            break;
        case SUBSCRIBE:
        case UNSUBSCRIBE:
            {
                H4AMC_PRINT3("  id: %d\n",id);
                uint8_t* payload=protocolpayload;
                do {
                    uint16_t len=_peek16(payload);
                    payload+=2;
                    std::string topic((const char*) payload,len);
                    payload+=len;
                    if(type==SUBSCRIBE) {
                        uint8_t subopts=*payload++;
                        uint8_t qos = subopts&0x3;
#if MQTT5
                        bool    nl  = (subopts&0x4)>>SUBSCRIPTION_OPTION_NO_LOCAL_SHIFT;
                        bool    rap = (subopts&0x8)>>SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED_SHIFT;
                        uint8_t rh  = (subopts&0x30)>>SUBSCRIPTION_OPTION_RETAIN_HANDLING_SHIFT;
                        H4AMC_PRINT3("  Topic: QoS%d %s NL%d RAP%d RH%d\n",qos,topic.data(),nl,rap,rh);
#else
                        H4AMC_PRINT3("  Topic: QoS%d %s\n",qos,topic.data());
#endif
                    }
                    else H4AMC_PRINT3("  Topic: %s\n",topic.data());
                } while (payload < (data + len));
            }
            break;

        default:
        {
            if(isPublish()){
                if(qos) H4AMC_PRINT3("  id: %d\n",id);
                H4AMC_PRINT3("  qos: %d\n",qos);
                if(dup) H4AMC_PRINT3("  DUP\n");
                if(retain) H4AMC_PRINT3("  RETAIN\n");
                H4AMC_PRINT3("  Topic: %s\n",topic.data());
                H4AMC_PRINT3("  Payload size: %d\n",plen);
            }
            else H4AMC_PRINT3("WTF99999999999999999999999?\n");
        }
        break;
    }
#if MQTT5
    if (properties) {
        properties->dump();
    }
#endif
}

void mqttTraits::dump(){
    H4AMC_PRINTF("PKTDUMP %s @ %p len=%d RL=%d off=%d flags=0x%02x\n",getPktName().data(),data,len,remlen,offset,flags);
    H4AMC_PRINTF("PKTDUMP %s id=%d qos=%d dup=%d ret=%d PR=%d PL=%p L=%d \n",topic.data(),id,qos,dup,retain,pubrec,payload,plen);
    dumphex(data,len);
}
#else
}
#endif