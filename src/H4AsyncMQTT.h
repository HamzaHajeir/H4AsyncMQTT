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

#include<Arduino.h>
#include<H4AsyncTCP.h>
#include"h4amc_common.h"
#include"Properties.h"


enum H4AMC_MQTT_CNX_FLAG : uint8_t {
    USERNAME      = 0x80,
    PASSWORD      = 0x40,
    WILL_RETAIN   = 0x20,
    WILL_QOS1     = 0x08,
    WILL_QOS2     = 0x10,
    WILL          = 0x04,
    CLEAN_START   = 0x02    // Called CLEAN_SESSION for v3.1.1 
};

#if MQTT5
struct Server_Options {
    uint32_t session_expiry_interval=MQTT_CONNECT_SESSION_EXPRITY_INTERVAL;
    uint16_t receive_max= UINT16_MAX;
    uint16_t topic_alias_max=0;
    uint32_t maximum_packet_size= UINT32_MAX;
    bool retain_available=true;
    bool wildcard_subscription_available=true;
    bool subscriptions_identifiers_available=true;
    bool shared_subscription_available=true;
    uint8_t maximum_qos=2;
    std::string response_information;
};
class H4Authenticator;
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT

struct SubscriptionResource {
    H4AMC_cbMessage cb;
    std::set<std::string> topix;
    // SubscriptionResource():cb(nullptr) {}
    // SubscriptionResource(H4AMC_cbMessage cb, std::set<std::string> topics) : cb(cb), topix(topics) {}
};
#endif // MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
#define AUTHPACKET_REASONSTR_USERPROPS 0
struct AuthOptions {
    H4AMC_MQTT_ReasonCode code;
#if AUTHPACKET_REASONSTR_USERPROPS
    std::string reasonStr;
    H4AMC_USER_PROPERTIES props;
    AuthOptions(H4AMC_MQTT_ReasonCode code, H4AMC_USER_PROPERTIES properties={}, const std::string& reason_string="") : code(code), props(properties), reasonStr(reason_string) {}
#else
    AuthOptions(H4AMC_MQTT_ReasonCode code) :code(code){}
#endif
};
#endif // MQTT5
class H4AMC_SubscriptionOptions {
    uint8_t qos;
#if MQTT5
    H4AMC_cbMessage cb=nullptr;
    bool nolocal=MQTT5_SUBSCRIPTION_OPTION_NO_LOCAL;
    bool rap=MQTT5_SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED;
    uint8_t rh=MQTT5_SUBSCRIPTION_OPTION_RETAIN_HANDLING;
    H4AMC_USER_PROPERTIES user_properties;
#endif
public:
#if MQTT5
    H4AMC_SubscriptionOptions(uint8_t QoS=0,
                              H4AMC_cbMessage callback=nullptr,
                              bool no_local = MQTT5_SUBSCRIPTION_OPTION_NO_LOCAL,
                              bool retain_as_published = MQTT5_SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED,
                              uint8_t retain_handling = MQTT5_SUBSCRIPTION_OPTION_RETAIN_HANDLING,
                              H4AMC_USER_PROPERTIES user_properties =H4AMC_USER_PROPERTIES{}) 
                              : qos(QoS), cb(callback), nolocal(no_local), rap(retain_as_published), rh(retain_handling), user_properties(user_properties)
    {
    }
#else
    H4AMC_SubscriptionOptions(uint8_t QoS=0) : qos(QoS) {}
#endif
#if MQTT5
            bool                getNoLocal(){ return nolocal; }
            bool                getRetainAsPublished(){ return rap; }
            uint8_t             getRetainHandling(){ return rh; }
            H4AMC_cbMessage     getCallback() { return cb; }
            H4AMC_USER_PROPERTIES& getUserProperties() { return user_properties; }
            void                setNoLocal(bool value) { nolocal=value; }
            void                setRetainAsPublished(bool value) { rap=value; }
            void                setRetainHandling(uint8_t value) { rh=value; }
            void                setCallback(H4AMC_cbMessage callback) {cb=callback; }
            void                setUserProperties(H4AMC_USER_PROPERTIES& properties) { user_properties=properties; }
#endif
            uint8_t             getQos(){ return qos; }
            void                setQos(uint8_t value) { qos=value;}

};

// [ ] Rationalize H4AMC_XXXOptions (struct/class) & APIs
struct H4AMC_PublishOptions {
    friend class H4AsyncMQTT;
    bool retain;
    bool getRetained() { return retain; }
#if MQTT5
    H4AMC_PublishOptions(bool retain = false, MQTT5PublishProperties props={}) : retain(retain), props(props) {}
    MQTT5PublishProperties& getProperties() { return props; }
private:
    MQTT5PublishProperties props;
#else
    H4AMC_PublishOptions(bool retain = false) : retain(retain) {}
#endif
};

class H4AMC_WillOptions {
    bool retain;
#if MQTT5
    MQTT5WillProperties props;
#endif

public:
#if MQTT5
    H4AMC_WillOptions(bool retain = false, MQTT5WillProperties properties={}) : retain(retain), props(properties) {}
    MQTT5WillProperties& getProperties() { return props; }
#else
    H4AMC_WillOptions(bool retain = false) : retain(retain) {}
#endif
    bool getRetained() { return retain; }
};

struct H4AMC_MessageOptions : public H4AMC_PublishOptions {
    uint8_t qos; // Is there an interest for the user to know qos/dup?
    bool dup;
    H4AMC_MessageOptions(uint8_t qos, bool retain, bool dup) : H4AMC_PublishOptions(retain), qos(qos), dup(dup) {}
#if MQTT5
    H4AMC_MessageOptions(uint8_t qos, bool retain, bool dup, MQTT5PublishProperties props) : H4AMC_PublishOptions(retain, props), qos(qos), dup(dup) {}
#endif
};
struct H4AMC_ConnackParam {
    bool session;
#if MQTT5
    H4AMC_USER_PROPERTIES connack_props;
    H4AMC_ConnackParam(bool session, H4AMC_USER_PROPERTIES up) : session(session), connack_props(up){}
#endif
    H4AMC_ConnackParam(bool session) : session(session){}
};

class mqttTraits {
                std::string     _decodestring(uint8_t** p);
        inline  uint16_t        _peek16(uint8_t* p){ return (*(p+1))|(*p << 8); }
        inline  uint16_t        _parse16(uint8_t** p) { auto r=_peek16(*p);  return *p+=2, r;}
        inline  uint8_t         _parse8(uint8_t** p) { return *(*p)++;}
                uint8_t*        data;
        
    public:
                uint8_t         conackflags=0;
                uint32_t        remlen=0;
                uint8_t         offset=0;
                uint8_t         flags=0;

                uint8_t*        protocolpayload=nullptr;
                size_t          len;
                uint8_t         type;
                PacketID        id=0;
                uint8_t         qos=0;
                bool            dup;
                bool            retain;
                std::string     topic;
                uint8_t*        payload;
                size_t          plen;
                bool            pubrec=0;
                size_t          retries=H4AMC_MAX_RETRIES;
                std::pair<uint8_t*,size_t> next;
                std::vector<uint8_t> subreasons;
                uint8_t         reasoncode=0;
                enum class PacketState : uint8_t {
                    OK,
                    MALFORMED,
                    INCOMPLETE
                } packet_state = PacketState::OK;
#if MQTT5
                uint32_t        _topic_index=0;
                uint16_t        _topic_alias=0;
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
                std::set<uint32_t> subscription_ids;
#endif
                std::shared_ptr<MQTT_Properties> properties;
                // MQTT_Properties*  properties = nullptr;     
                MQTT_PROP_PARSERET     initiateProperties(uint8_t* start) { properties=std::make_shared<MQTT_Properties>();
                    auto ret = properties->parseProperties(start);
                    if (ret.first){
                        packet_state=PacketState::MALFORMED;
                        H4AMC_PRINT1("ERROR: Malformed Packet %d\n", ret.first);
                    }
                    return ret;
                }
#endif
                
#if H4AMC_DEBUG
#if MQTT5
        static std::map<H4AMC_MQTT_ReasonCode,char*> rcnames;
        static std::map<H4AMC_MQTT5_Property,char*> propnames;
#else
        static  std::map<uint8_t,char*> connacknames;
        static  std::map<uint8_t,char*> subacknames;
#endif
        static std::map<uint8_t,char*> pktnames;
        std::string             getPktName(){
            uint8_t t=type&0xf0;
            if(pktnames.count(t)) return pktnames[t];
            else return stringFromInt(type,"%02X");
        }
                void            dump();
#else
        std::string             getPktName(){ return stringFromInt(type,"%02X"); }
#endif
        mqttTraits(){};
        mqttTraits(uint8_t* p,size_t s);
        mqttTraits (const mqttTraits&) = default;
        mqttTraits (mqttTraits&&) = delete;
        mqttTraits& operator=(const mqttTraits&) = default;
        mqttTraits& operator=(mqttTraits&&) = delete;
        ~mqttTraits() = default;

        uint8_t*                get() { return data; }
        void                    set(uint8_t* p) { data = p; }
        inline  bool            isPublish() { return (type & 0xf0) == PUBLISH; }
        inline  uint8_t*        start() { return data+1+offset; }
};

struct WillMessage {
    std::string         payload;
    uint8_t             qos=0;
    bool                retain=false;
    std::string         topic;
#if MQTT5
    MQTT5WillProperties props;
#endif
} ;

class Packet;
class ConnectPacket;
class PublishPacket;

enum {
    H4AMC_DISCONNECTED,
    H4AMC_CONNECTING,
    H4AMC_TCP_ERROR,
    H4AMC_TCP_CONNECTED,
    H4AMC_RUNNING
};

enum NetworkState : uint8_t {
    H4AMC_NETWORK_DISCONNECTED,
    H4AMC_NETWORK_CONNECTED
};
class H4AsyncMQTT {
        friend class Packet;
        friend class ConnectPacket;
        friend class PublishPacket;
        friend class mqttTraits;
                NetworkState        _networkState=H4AMC_NETWORK_CONNECTED;
                uint32_t            _state=H4AMC_DISCONNECTED;
#if MQTT5
                // bool                _cleanStart=true; // _forceCleanStart=true;
                Server_Options*     _serverOptions=nullptr;
                std::map<PacketHeader, std::shared_ptr<H4AMC_USER_PROPERTIES>> _user_static_props;                
                std::map<PacketHeader, H4AMC_FN_DYN_PROPS> _user_dynamic_props;
                MQTT5WillProperties _willProperties;
                H4AMC_FN_STRING     _cbRedirect;
                H4AMC_FN_STRING     _cbReason;
                H4Authenticator*    _authenticator=nullptr;
                std::map<uint16_t, std::string> _rx_topic_alias;
                // std::map<std::string, uint16_t> _tx_topic_alias;
                std::vector<std::string> _tx_topic_alias; // vector<string> because we manage it.
                uint32_t            _inflight=0;
                std::queue<PacketID> _pending;
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
                
                H4AMC_SUBRES_MAP    _subsResources;
                H4AMC_PK_SUBIDS_MAP _packSubIds;
                uint32_t            subId = 0; // VBI.

                void                _proposeDeletion(uint32_t packetId, uint32_t subId) {_packSubIds[packetId]=subId;};
                void                _confirmDeletion(uint32_t packId) { _subsResources.erase(_packSubIds[packId]); _packSubIds.erase(packId); }
#endif

#endif // MQTT5
                H4AMC_cbConnect     _cbMQTTConnect=nullptr;
                H4AMC_FN_VOID       _cbMQTTDisconnect=nullptr;
                H4AMC_cbError       _cbMQTTError=nullptr;
                H4AMC_cbMessage     _cbMessage=nullptr;
                H4AMC_cbPublish     _cbPublish=nullptr;
                std::string         _clientId;
                std::string         _assignedClientId;
        static  H4_INT_MAP          _errorNames;
        // static  H4AMC_PACKET_MAP    _inbound;
        static  std::set<int>       _inbound;
                uint32_t            _keepalive;
                PacketID            _nextId=1000;
        static  H4AMC_PACKET_MAP    _outbound;
                std::string         _password;
                std::string         _url;
                std::string         _username;
                WillMessage         _will;
#if H4AT_TLS
                H4AMC_BinaryData    _caCert;
                H4AMC_BinaryData    _privkey;
                H4AMC_BinaryData    _privkeyPass;
                H4AMC_BinaryData    _clientCert;
#endif
               
                void                _ACK(H4AMC_PACKET_MAP* m,PacketID id); // inout true=INBOUND false=OUTBOUND
                void                _ACKoutbound(PacketID id){ _ACK(&_outbound,id); }

                bool                _send(const uint8_t *data, uint32_t len, bool copy)
                                    {
                                        // [ ] Concatenate meaaages (optional)
                                        H4AMC_PRINT4("_send(%p,%u,%d)\n", data,len,copy);
                                        if (_state==H4AMC_RUNNING || _state==H4AMC_TCP_CONNECTED){
                                            _h4atClient->TX(data, len, copy);
                                            return true;
                                        }
                                        return false;
                                    }

                void                _startClean();
                void                _clearQQ(H4AMC_PACKET_MAP* m);
                void                _startPinging(uint32_t keepalive=KEEP_ALIVE_INTERVAL);
                void                _connect();
                void                _destroyClient();
                bool                _haveSessionData() { return _inbound.size() || _outbound.size(); }
#if MQTT5
                void                _protocolError(H4AMC_MQTT_ReasonCode reason, int errorNotify=0);
                void                _handleConnackProps(MQTT_Properties& props);
                void                _handleAuthentication(uint8_t reasoncode, MQTT_Properties& props);
                void                _redirect(MQTT_Properties& props);
                void                _handleReasonString(MQTT_Properties& props) {
                                        if (_cbReason && props.isAvailable(PROPERTY_REASON_STRING))
                                            _cbReason(props.getStringProperty(PROPERTY_REASON_STRING));
                                    }

                void                _addUserProp(PacketHeader header, std::shared_ptr<H4AMC_USER_PROPERTIES> nv)  { _user_static_props[header]=nv; }
                void                _addDynamicUserProp(PacketHeader header, H4AMC_FN_DYN_PROPS f)              { _user_dynamic_props[header]=f; }

                bool                _isTXAliasAvailable(const std::string &topic) { auto& ta=_tx_topic_alias; return std::find(ta.begin(), ta.end(), topic) != ta.end(); } // For any fresh start, _tx_topic_alias is empty
                bool                _availableTXAliasSpace();
                uint16_t            _assignTXAlias(const std::string& topic) { _tx_topic_alias.push_back(topic); return _tx_topic_alias.size(); }
                uint16_t            _getTXAlias(const std::string& topic);
                std::string         _getTXAliasTopic(uint16_t alias) { return (alias>_tx_topic_alias.size() || !alias) ? "" : _tx_topic_alias[alias-1]; }
                void                _clearAliases() { _tx_topic_alias.clear(), _rx_topic_alias.clear(); }
#if H4AMC_MQTT5_INSERT_TOPIC_BY_ALIAS
                // Makes an anatomy to the to-send-data, inserting topic name and remaining length.
                bool                _insertTopicAlias(mqttTraits& m, std::vector<std::string>& remaining);
#endif
                void                _runFlow() {
                                        if (_pending.size() && _outbound.count(_pending.front())){
                                            auto m = _outbound[_pending.front()];
                                            H4AMC_PRINT4("RUN FLOW %d\n", m.id);
                                            _send(m.get(), m.len, false);
                                            _pending.pop();
                                        }
                                    }
                void                _blockPublish(PacketID id) { 
                                        H4AMC_PRINT4("BLOCK PUB %d\n", id); 
                                        _pending.push(id); 
                                    }
                void                _publishEnd() { 
                                        if (_pending.size()) _runFlow();
                                        else _inflight--;
                                        H4AMC_PRINT4("_publishEnd _inflight %d\n", _inflight);
                                    }
                bool                _canPublishQoS(PacketID id) {
                                        H4AMC_PRINT4("_canPublishQoS(%d) _inflight=%d rmax=%d\n", id, _inflight, getServerOptions().receive_max);
                                        if (_inflight >= getServerOptions().receive_max){
                                            _blockPublish(id);
                                            return false;
                                        }
                                        _inflight++;
                                        return true;
                                    }
#endif
                void                _handlePacket(uint8_t* data, size_t len, int n_handled=0, uint8_t* copy=nullptr);
                void                _handlePublish(mqttTraits T);
        inline  void                _hpDespatch(mqttTraits T);
        // inline  void                _hpDespatch(uint16_t id);
                void                _resendPartialTxns(bool availableSession);
        inline  void                _runGuard(H4AMC_FN_VOID f);
        template<typename T>
        inline  T                   _runGuard(H4_FN_COUNT f,T dummy) { // For numerics only.
                                        if(_state==H4AMC_RUNNING) return f();
                                        else _notify(0,H4AMC_USER_LOGIC_ERROR);
                                        return 0;
                                    }
        inline  void                _startReconnector();
        inline  uint16_t            _getKeepAliveSeconds() { return _keepalive / 1000; }
    public:
                H4AsyncClient*      _h4atClient;
        H4AsyncMQTT();
                void                connect(const char* url,const char* auth="",const char* pass="",const char* clientId=""/* ,bool clean=true */);
                void                disconnect(H4AMC_MQTT_ReasonCode reason=REASON_NORMAL_DISCONNECTION);
        static  std::string         errorstring(int e);
                std::string         getClientId(){ return _assignedClientId.length() ? _assignedClientId : _clientId; }
                void                onDisconnect(H4AMC_FN_VOID callback){ _cbMQTTDisconnect=callback; }
                void                onConnect(H4AMC_cbConnect callback){ _cbMQTTConnect=callback; }
#if MQTT5
        static  void                printUserProperty(H4AMC_USER_PROPERTIES& props) { for (auto p:props) _H4AMC_PRINTF("%s:%s\n", p.first.c_str(), p.second.c_str());}
                void                onRedirect(H4AMC_FN_STRING f) { _cbRedirect=f; }
                void                onReason(H4AMC_FN_STRING f) { _cbReason=f; }
                void                setAuthenticator(H4Authenticator* authenticator) { _authenticator = authenticator; }
                Server_Options      getServerOptions() { return _serverOptions ? *_serverOptions : Server_Options(); }
                // Valid for CONNECT, PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBSCRIBE, UNSUBSCRIBE, DISCONNECT, AUTH

                bool                addStaticUserProp(PacketHeader header, H4AMC_USER_PROPERTIES user_properties);
                bool                addStaticUserProp(std::initializer_list<PacketHeader> headers, H4AMC_USER_PROPERTIES user_properties);
                bool                addDynamicUserProp(PacketHeader header, H4AMC_FN_DYN_PROPS f);
                bool                addDynamicUserProp(std::initializer_list<PacketHeader> headers, H4AMC_FN_DYN_PROPS f);
                void                resetUserProps() { _user_static_props.clear(); _user_dynamic_props.clear(); }
#endif

                /* secureTLS
                    Make sure activating H4AT_TLS and passing a valid secure url "https", NOT "http" to connect securely.
                    All keys/certificates must contain the null terminator if PEM encoded
                 */
                bool                secureTLS(const u8_t *ca, size_t ca_len, const u8_t *privkey = nullptr, size_t privkey_len=0,
                                            const u8_t *privkey_pass = nullptr, size_t privkey_pass_len = 0,
                                            const u8_t *cert = nullptr, size_t cert_len = 0);
                void                informNetworkState(NetworkState state) { _networkState = state; }
                void                onError(H4AMC_cbError callback){ _cbMQTTError=callback; }
                void                onMessage(H4AMC_cbMessage callback){ _cbMessage=callback; }
                void                onPublish(H4AMC_cbPublish callback){ _cbPublish=callback; }
                PacketID            publish(const char* topic,const uint8_t* payload, size_t length, uint8_t qos=0, H4AMC_PublishOptions opts={}); // Might embed qos inside opts.???
                PacketID            publish(const char* topic,const char* payload, size_t length, uint8_t qos=0, H4AMC_PublishOptions opts={});

#if MQTT5
#define CAST_TYPE   char
#else
#define CAST_TYPE   uint8_t
#endif
                template<typename T>
                PacketID publish(const char* topic,T v,const char* fmt="%d",uint8_t qos=0, H4AMC_PublishOptions opts={}){
                    char buf[16];
                    sprintf(buf,fmt,v);
                    return publish(topic, reinterpret_cast<const CAST_TYPE*>(buf), strlen(buf), qos, opts);
                }
//              Coalesce templates when C++17 available (if constexpr (x))
                PacketID xPublish(const char* topic,const char* value, uint8_t qos=0, H4AMC_PublishOptions opts={}) {
                    return publish(topic,reinterpret_cast<const CAST_TYPE*>(value),strlen(value),qos,opts);
                }
                uint16_t xPublish(const char* topic,String value, uint8_t qos=0, H4AMC_PublishOptions opts={}) {
                    return publish(topic,reinterpret_cast<const CAST_TYPE*>(value.c_str()),value.length(),qos,opts);
                }
                PacketID xPublish(const char* topic,const std::string& value, uint8_t qos=0, H4AMC_PublishOptions opts={}) {
                    return publish(topic,reinterpret_cast<const CAST_TYPE*>(value.c_str()),value.size(),qos,opts);
                }
                template<typename T>
                PacketID xPublish(const char* topic,T value, uint8_t qos=0, H4AMC_PublishOptions opts={}) {
                    return publish(topic,reinterpret_cast<uint8_t*>(&value),sizeof(T),qos,opts);
                }
#ifdef CAST_TYPE
#undef CAST_TYPE
#endif
                void xPayload(const uint8_t* payload,size_t len,char*& cp) {
                    char* p=reinterpret_cast<char*>(malloc(len+1));
                    memcpy(p,payload,len);
                    p[len]='\0';
                    cp=p;
                }
                void xPayload(const uint8_t* payload,size_t len,std::string& ss) {
                    char* cp;
                    xPayload(payload,len,cp);
                    ss.assign(cp,strlen(cp));
                    ::free(cp);
                }
                void xPayload(const uint8_t* payload,size_t len,String& duino) {
                    char* cp;
                    xPayload(payload,len,cp);
                    duino+=cp;
                    ::free(cp);
                }
                template<typename T>
                void xPayload(const uint8_t* payload,size_t len,T& value) {
                    if(len==sizeof(T)) memcpy(reinterpret_cast<T*>(&value),payload,sizeof(T));
                    else _notify(H4AMC_X_INVALID_LENGTH,len);
                }
//                void               setKeepAlive(uint16_t keepAlive){ _keepalive=keepAlive; }
                void               setWill(const char* topic, uint8_t qos, const char* payload = nullptr, H4AMC_WillOptions opts_retain={});
                /**! subscribe
                 *              For MQTT v3.3, one can pass QoS value directly.
                 * \return 
                */
                uint32_t           subscribe(const char* topic, H4AMC_SubscriptionOptions opts_qos={});
                uint32_t           subscribe(std::initializer_list<const char*> topix, H4AMC_SubscriptionOptions opts_qos={});
                void               unsubscribe(const char* topic);
                void               unsubscribe(std::initializer_list<const char*> topix);
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
                void               unsubscribe(uint32_t subscription_id);
#endif
//
//              DO NOT CALL ANY FUNCTION STARTING WITH UNDERSCORE!!! _
//
                void               _notify(int e,int info=0);
                void               dump(); // null if no debug
};

#if MQTT5
class H4Authenticator {
    friend class H4AsyncMQTT;
    friend class ConnectPacket;
    public:
                    H4Authenticator(std::string method) : _method(method){}
    protected:
                    std::string                     _method;
    private:
                    /* 
                    * Callable on first AUTH appearance (CONNECT/RE_AUTH) 
                    * Purpose: To fetch initial data and reset state (if any)
                    */
        virtual     H4AMC_AuthInformation           start()=0; // 
        virtual     H4AMC_AuthInformation           handle(H4AMC_AuthInformation data)=0;
};
class SCRAM_Authenticator : protected H4Authenticator {
            enum SCRAM_State : uint8_t {
                START, // SENDS client first
                CLIENT_FIRST_SENT,
                CLIENT_FINAL_SENT,
                // SERVER_FINAL_RECEIVED,
                COMPLETE
            } state;
            H4AMC_BinaryData    _client_first;
                    H4AMC_AuthInformation           start() override;
                    H4AMC_AuthInformation           handle(H4AMC_AuthInformation data) override;
    public:
            SCRAM_Authenticator(H4AMC_BinaryData client_first, std::string method_name="SCRAM-SHA-1") : H4Authenticator(method_name),  _client_first(client_first) {}

};
class KERBEROS_Authenticator : protected H4Authenticator {
            enum KERBEROS_State : uint8_t {
                START, // SENDS Empty data
                STARTED,
                INITIAL_CONTEXT_SENT, // SENDS Initial Context Token
                REPLY_CONTEXT_HANDLED, // SENDS Empty data
                // OUTCOME_RECEIVED,
                COMPLETE

            } state;

                    H4AMC_AuthInformation           start() override;
                    H4AMC_AuthInformation           handle(H4AMC_AuthInformation data) override;
};
#endif