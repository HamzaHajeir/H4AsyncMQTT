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
#pragma once
#include <H4AsyncMQTT.h>

#define STRINGBLOCKS_USE_MBX 0
#if STRINGBLOCKS_USE_MBX
using H4AMC_BLOCK_Q        = std::queue<mbx>;
#else
using H4AMC_BLOCK_Q        = std::queue<std::string>;
#endif

class Packet {
    protected:
#if MQTT5
                // uint8_t          _reasonCode=0; // If we meant to imply it.
                H4AMC_FN_U8PTRU8 _properties=[](uint8_t* p){ return p; };
                uint32_t         _propertyLength=0;

                std::shared_ptr<H4AMC_USER_PROPERTIES> _dynProps;

                uint32_t         __fetchSize(H4AMC_USER_PROPERTIES& props);
                uint32_t         __fetchPassedProps(H4AMC_USER_PROPERTIES& props);
                uint32_t         __fetchStaticProps();
                uint32_t         __fetchDynamicProps();
                uint8_t*         __embedProps(uint8_t* p, H4AMC_USER_PROPERTIES& props);
                uint8_t*         __embedPassedProps(uint8_t* p, H4AMC_USER_PROPERTIES& props);
                uint8_t*         __embedStaticProps(uint8_t* p);
                uint8_t*         __embedDynamicProps(uint8_t* p);
                uint32_t         _fetchUserProperties(H4AMC_USER_PROPERTIES& publish_userProps) {
                    uint32_t total=0;
                    total+=__fetchPassedProps(publish_userProps);
                    total+=__fetchStaticProps();
                    total+=__fetchDynamicProps();
                    return total;
                }

                uint8_t*          _serializeUserProperties(uint8_t* p, H4AMC_USER_PROPERTIES& props) {
                    p=__embedPassedProps(p, props);
                    p=__embedStaticProps(p);
                    return __embedDynamicProps(p);
                }
#endif
                H4AsyncMQTT*     _parent;
                PacketID         _id=0; 
                uint8_t          _controlcode;
                H4AMC_BLOCK_Q    _blox;
                uint32_t         _bs=0;
                H4AMC_FN_VOID    _begin=[]{};   //** Setup the packet traits
                H4AMC_FN_U8PTRU8 _varHeader=[](uint8_t* p){ return p; };
                H4AMC_FN_U8PTR   _protocolPayload=[](uint8_t* p,uint8_t* base){};
                void	         _build();
                void             _idGarbage(PacketID id);
                void             _multiTopic(std::set<std::string> topix,H4AMC_SubscriptionOptions opts={});
                uint8_t*         _poke16(uint8_t* p,uint16_t u);
                void             _stringblock(const std::string& s);
#if STRINGBLOCKS_USE_MBX
                uint8_t*         _serializeblock(uint8_t* p, mbx& block);
#else
                uint8_t*         _serializeblock(uint8_t* p, std::string& block);
#endif
                uint8_t*         _applyfront(uint8_t* p);
        inline  void             _notify(H4AMC_FAILURE e, int i=0) { _parent->_notify(e,i); }
    public:
        Packet(H4AsyncMQTT* p,uint8_t controlcode): _parent(p),_controlcode(controlcode){}
};

class ConnectPacket: public Packet {
#if MQTT5
                H4AMC_FN_U8PTRU8 _willproperties=[](uint8_t* p){ return H4AMC_Helpers::encodeVariableByteInteger(p, 0); }; // default to encodeVariableByte(0)
                std::string _authmethod;
                std::vector<uint8_t> _authdata;
#endif
            uint8_t  protocol[8]={0x0,0x4,'M','Q','T','T',MQTT_VERSION,0};
    public:
        ConnectPacket(H4AsyncMQTT* p);
};

struct PubackPacket: public Packet {
    public:
        PubackPacket(H4AsyncMQTT* p,PacketID id): Packet(p,PUBACK) { _idGarbage(id); /* Dismiss Properties? */ }
};
class PubrecPacket: public Packet {
    public:
        PubrecPacket(H4AsyncMQTT* p,PacketID id): Packet(p,PUBREC) { _idGarbage(id); }
};
class PubrelPacket: public Packet {
    public:
        PubrelPacket(H4AsyncMQTT* p,PacketID id): Packet(p,PUBREL) { _idGarbage(id); }
};
class PubcompPacket: public Packet {
    public:
        PubcompPacket(H4AsyncMQTT* p,PacketID id): Packet(p,PUBCOMP) { _idGarbage(id); }  
};
class SubscribePacket: public Packet {
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
    uint32_t subscription_id=0;
#endif
    public:
        SubscribePacket(H4AsyncMQTT* p,const std::string& topic,H4AMC_SubscriptionOptions opts): Packet(p,SUBSCRIBE) { _multiTopic({topic.data()},opts); }
        SubscribePacket(H4AsyncMQTT* p,std::initializer_list<const char*> topix,H4AMC_SubscriptionOptions opts): Packet(p,SUBSCRIBE) { _multiTopic(std::set<std::string>(topix.begin(), topix.end()),opts); }
        uint32_t    getId() { 
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
            return subscription_id;
#else
            return 0;
#endif

        }
};

class UnsubscribePacket: public Packet {
    public:
        UnsubscribePacket(H4AsyncMQTT* p,const std::string& topic): Packet(p,UNSUBSCRIBE) { _multiTopic({topic.data()}); }
        UnsubscribePacket(H4AsyncMQTT* p,std::initializer_list<const char*> topix): Packet(p,UNSUBSCRIBE) { _multiTopic(std::set<std::string>(topix.begin(), topix.end())); }
        UnsubscribePacket(H4AsyncMQTT* p,std::set<std::string> topix): Packet(p,UNSUBSCRIBE) { _multiTopic(topix); }
};

class PublishPacket: public Packet {
        std::string     _topic;
        uint8_t         _qos;
        bool            _retain;
        size_t          _length;
        // bool            _dup;
    public:
        PublishPacket(H4AsyncMQTT* p,const char* topic, uint8_t qos, const uint8_t* payload, size_t length, H4AMC_PublishOptions opts_retain);
        PacketID getId(){ return _id; }
};

#if MQTT5
class AuthenticationPacket: public Packet {
    public:
        AuthenticationPacket(H4AsyncMQTT* p, const std::string& method, const H4AMC_BinaryData& data, AuthOptions& auth);
};
#endif