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
#include <H4AsyncMQTT.h>
#include "Packet.h"
#if H4AT_TLS_SESSION
#include "lwip/apps/altcp_tls_mbedtls_opts.h"
#endif

std::set<int>           H4AsyncMQTT::_inbound;
H4AMC_PACKET_MAP        H4AsyncMQTT::_outbound;

H4_INT_MAP H4AsyncMQTT::_errorNames={
#if H4AMC_DEBUG
    {H4AMC_CONNECT_FAIL,"CONNECT FAIL"},
    {H4AMC_SUBSCRIBE_FAIL,"SUBSCRIBE_FAIL"},
    {H4AMC_UNSUBSCRIBE_FAIL,"UNSUBSCRIBE_FAIL"},
    {H4AMC_INBOUND_QOS_ACK_FAIL,"H4AMC_INBOUND_QOS_ACK_FAIL"},
    {H4AMC_OUTBOUND_QOS_ACK_FAIL,"H4AMC_OUTBOUND_QOS_ACK_FAIL"},
    {H4AMC_INBOUND_PUB_TOO_BIG,"H4AMC_INBOUND_PUB_TOO_BIG"},
    {H4AMC_OUTBOUND_PUB_TOO_BIG,"H4AMC_OUTBOUND_PUB_TOO_BIG"},
    {H4AMC_BOGUS_PACKET,"H4AMC_BOGUS_PACKET"},
    {H4AMC_X_INVALID_LENGTH,"H4AMC_X_INVALID_LENGTH"},
    {H4AMC_USER_LOGIC_ERROR,"USER LOGIC ERROR"},
    {H4AMC_NOT_RUNNING,"NOT RUNNING"},
    {H4AMC_NO_SSL,"H4AMC_NO_SSL"},
//    {H4AMC_KEEPALIVE_TOO_LONG,"KEEPALIVE TOO LONG: MUST BE < H4AS_SCAVENGE_FREQ"},
#if MQTT5
    {H4AMC_SERVER_DISCONNECT,"Server Disconnect"},
	{H4AMC_NO_SERVEROPTIONS,"No Server Options - Missing CONNACK?"},
    {H4AMC_PUBACK_FAIL,"PUBACK FAIL"},
    {H4AMC_PUBREC_FAIL,"PUBREC FAIL"},
    {H4AMC_PUBREL_FAIL,"PUBREL FAIL"},
    {H4AMC_PUBCOMP_FAIL,"PUBCOMP FAIL"},
    {H4AMC_SERVER_RETAIN_UNAVAILABLE,"SERVER_RETAIN_UNAVAILABLE"},
    {H4AMC_SHARED_SUBS_UNAVAILABLE,"SHARED_SUB_UNAVAILABLE"},
    {H4AMC_BAD_SHARED_TOPIC,"BAD_SHARED_TOPIC"},
    {H4AMC_BAD_TOPIC,"BAD TOPIC"},
    {H4AMC_WILDCARD_UNAVAILABLE,"SERVER_WILDCARD_UNAVAILBLE"},
    {H4AMC_ASSIGNED_CLIENTID,"ASSIGNED_CLIENTID"},
    {H4AMC_NO_AUTHENTICATOR,"NO AUTHENTICATOR ASSIGNED"},
    {H4AMC_INVALID_AUTH_METHOD,"INVALID AUTH METHOD"},
    {H4AMC_SUBID_NOT_FOUND,"SUBID NOT FOUND"},
    {H4AMC_PROTOCOL_ERROR,"PROTOCOL ERROR"},
    {H4AMC_INCOMPLETE_PACKET,"INCOMPLETE PACKET"}

#endif
#endif
};
namespace H4AMC_Helpers {
    uint8_t* poke8(uint8_t * p, uint8_t u){
        *p++=u;
        return p;
    }
    uint8_t* poke16(uint8_t* p,uint16_t u){
        *p++=(u & 0xff00) >> 8;
        *p++=u & 0xff;
        return p;
    }
    uint16_t peek16(uint8_t* p){ return (*(p+1))|(*p << 8); }
    
    uint8_t* encodestring(uint8_t* p,const std::string& s){
        p=poke16(p,s.size());
        memcpy(p,s.data(),s.size());
        return p+s.size();
    }

    std::string decodestring(uint8_t** p){
        size_t tlen=peek16(*p);//payload+=2;
        std::string rv((const char*) *(p)+2,tlen);
        *p+=2+tlen;
        return rv;
    }
    uint8_t* encodeBinary(uint8_t* p, const H4AMC_BinaryData& data)
    {
        p=poke16(p,data.size());
        std::copy_n(data.begin(), data.size(), p);
        return p+data.size();
    }
    H4AMC_BinaryData decodeBinary(uint8_t** p)
    {
        uint16_t len=peek16(*p);
        *p+=2;
        H4AMC_BinaryData rv;
        rv.reserve(len);
        std::copy_n(*p, len, std::back_inserter(rv));
        *p+=len;
        return rv;
    }
    uint32_t decodeVariableByteInteger(uint8_t** p) {
        uint32_t multiplier = 1;
        uint32_t value = 0;
        uint8_t encodedByte;
        do {
            encodedByte = *(*p)++;  // Ensure this is the right pointer dealing.
            value += (encodedByte & 127) * multiplier;
            multiplier *= 128;
            if (multiplier > 128 * 128 * 128)
                return UINT32_MAX; // Malformed
        } while ((encodedByte & 128) != 0);
        return value;
    }
    uint8_t* encodeVariableByteInteger(uint8_t* p, uint32_t value) {
        do {
            uint8_t encodedByte = value % 128;
            value /= 128;
            // if there are more data to encode, set the top bit of this byte
            if (value > 0)
                encodedByte |= 128;
            *p++=encodedByte;
        } while (value > 0);
        return p;
    }
    uint8_t varBytesLength(uint32_t value){
        if (value > 268435455) return 0; // Error. 4 bytes max
        uint8_t len=1;
        while(value>127){
            value>>=7;
            len++;
        }
        return len;
    }
}
H4AsyncMQTT::H4AsyncMQTT(){

}

void H4AsyncMQTT::_ACK(H4AMC_PACKET_MAP* m,PacketID id){ /// refakta?
    if(m->count(id)){
        uint8_t* data=((*m)[id]).get();
        mbx::clear(data);
        m->erase(id);
    } else _notify(H4AMC_OUTBOUND_QOS_ACK_FAIL,id); //H4AMC_PRINT("WHO TF IS %d???\n",id);
    mbx::dump();
}

void H4AsyncMQTT::_startClean(){
    H4AMC_PRINT4("_startClean clrQQ inbound\n");
    _inbound.clear();
    H4AMC_PRINT4("_startClean clrQQ outbound\n");
    _clearQQ(&_outbound);
}

void H4AsyncMQTT::_clearQQ(H4AMC_PACKET_MAP* m){
    for(auto &i:*m) mbx::clear(i.second.get());
    m->clear();
}

void H4AsyncMQTT::_startPinging(uint32_t keepalive)
{
    _keepalive = keepalive;
    H4AMC_PRINT1("KA = %d\n",_keepalive - H4AMC_HEADROOM);
    static uint8_t PING[]={PINGREQ,0};    
    h4.every(_keepalive - H4AMC_HEADROOM,[this]{
        if(_state==H4AMC_RUNNING){//} && ((millis() - _h4atClient->_lastSeen) > _keepalive)){ // 100 = headroom
            H4AMC_PRINT1("MQTT PINGREQ\n");
            _send(PING, 2, false); /// optimise
        } //else Serial.printf("No ping: activity %d mS ago\n",(millis() - _h4atClient->_lastSeen));
    },nullptr,H4AMC_KA_ID,true);
}

void H4AsyncMQTT::_connect(){
    
    H4AMC_PRINT1("_connect %s _state=%d\n",_url.data(),_state);
    if (_state != H4AMC_DISCONNECTED) {
        H4AMC_PRINT1("Already connecting/connected\n");
        return;
    }
    if (_h4atClient)
        _h4atClient->close();
    
    // if network connected, proceed, else just follow create AsyncClient if not there. ..
    if (_networkState != H4AMC_NETWORK_CONNECTED) {
        H4AMC_PRINT1("Network is disconnected\n"); // If it's connected, inform with informNetworkState() API
        return;
    }
    _h4atClient = new H4AsyncClient;
    _state = H4AMC_CONNECTING;
    
    _h4atClient->onConnect([this](){
        H4AMC_PRINT1("on TCP Connect\n");
        _h4atClient->nagle(true);
        h4.cancelSingleton(H4AMC_RCX_ID);
        _startPinging();
        _state=H4AMC_TCP_CONNECTED;
        h4.queueFunction([this]{ ConnectPacket cp{this}; }); // offload required for esp32 to get off tcpip thread
    });

    static auto onDisconnect = [this] {
        if(_state==H4AMC_RUNNING || _state == H4AMC_TCP_ERROR) if (_cbMQTTDisconnect) _cbMQTTDisconnect();
        _state=H4AMC_DISCONNECTED;
        _h4atClient = nullptr;
        _startReconnector();
    };

    _h4atClient->onConnectFail([this](){
        H4AMC_PRINT1("onConnectFail - reconnect\n");
        onDisconnect();
    });

    _h4atClient->onDisconnect([this]{
        H4AMC_PRINT1("onDisconnect - reconnect STATE %d\n", _state);
        onDisconnect();
#if MQTT5
        _pending={};
        _inflight=0;
#endif
    });

    _h4atClient->onError([this](int error,int info){
        H4AMC_PRINT1("onError %d info=%d\n",error,info);
        if(error||info) _notify(error,info);
        if (error && _state == H4AMC_RUNNING) _state = H4AMC_TCP_ERROR;
        return true;
    });

#if H4AT_TLS_SESSION
    static void* _tlsSession;
    static uint32_t _lastSessionMs;
    static std::string lastURL;
    _h4atClient->enableTLSSession();
    _h4atClient->onSession(
        [this](void *tls_session)
        {
            H4AMC_PRINT1("onSession(%p)\n", tls_session);
            _tlsSession = const_cast<void *>(tls_session);
            _lastSessionMs = millis();
            H4AMC_PRINT3("_tlsSession %p _lastSessionMs %u\n", _tlsSession, _lastSessionMs);
        });


    H4AMC_PRINT4("_url %s lastURL %s millis() %u _lastSessionMs %u diff=%u\n _url==lastURL=%d\tdiff<timeout=%d\n", _url.c_str(), lastURL.c_str(), millis(), _lastSessionMs, millis() - _lastSessionMs, 
    _url == lastURL, (millis() - _lastSessionMs < ALTCP_MBEDTLS_SESSION_CACHE_TIMEOUT_SECONDS * 1000));
    if (_tlsSession && _url == lastURL && (millis() - _lastSessionMs < ALTCP_MBEDTLS_SESSION_CACHE_TIMEOUT_SECONDS * 1000)) {
        _h4atClient->setTLSSession(_tlsSession);
    }
    else {
        if (_tlsSession) {
            _h4atClient->freeTLSSession(_tlsSession);
            _tlsSession = nullptr;
        }
    }
    lastURL = _url;
#endif

    _h4atClient->onRX([this](const uint8_t* data,size_t len){ _handlePacket((uint8_t*) data,len); });

#if H4AT_TLS
    auto cas = _caCert.size();
    auto pks = _privkey.size();
    auto pkps = _privkeyPass.size();
    auto cs = _clientCert.size();
    if (cas) {
     _h4atClient->secureTLS(_caCert.data(), _caCert.size(), 
                                pks ? _privkey.data() : nullptr, pks, 
                                pkps ? _privkeyPass.data() : nullptr, pkps, 
                                cs ? _clientCert.data() : nullptr, cs);
    }
#endif 
    _h4atClient->connect(_url);
}

void H4AsyncMQTT::_destroyClient() {
    H4AMC_PRINT1("DESTROY CLIENT\n");
    if (_h4atClient && _h4atClient->connected()) {
        _h4atClient->close(); // It all should be ending here
    }
    _h4atClient=nullptr; // OR the callback?
}
void H4AsyncMQTT::_hpDespatch(mqttTraits P){ 
    if(_cbMessage) {
#if MQTT5
        MQTT_Properties props{P.properties ? *(P.properties) : MQTT_Properties() };
        H4AMC_MessageOptions opts(P.qos, P.retain, P.dup, props);
#else
        H4AMC_MessageOptions opts(P.qos, P.retain, P.dup);
#endif
#if H4AMC_DEBUG
        if(P.topic=="pango"){
            H4AMC_PRINT1("H4AMC INTERCEPTED\n");
            std::string pl((const char*) P.payload,P.plen);
            if(pl=="mem"){
                _H4AMC_PRINTF("FH: %u\nMB: %u\nMP: %d\n",_HAL_freeHeap(),_HAL_maxHeapBlock(),getMaxPayloadSize());
            } 
            else if(pl=="info"){ 
                _H4AMC_PRINTF("H4AT  Vn: %s\n",H4AT_VERSION);
//                _H4AMC_PRINTF("CHECK FP: %d\n",H4AT_CHECK_FINGERPRINT);
//                _H4AMC_PRINTF("SAFEHEAP: %d\n",H4T_HEAP_SAFETY);
                _H4AMC_PRINTF("H4AMC Vn: %s\n",H4AMC_VERSION);
                _H4AMC_PRINTF("NRETRIES: %d\n",H4AMC_MAX_RETRIES);
                _H4AMC_PRINTF("start : %s\n",_haveSessionData() ? "clean":"dirty");
                _H4AMC_PRINTF("clientID: %s\n",getClientId().data());
                _H4AMC_PRINTF("keepaliv: %d\n",_keepalive);
            }
            else if(pl=="dump"){ dump(); }
        }
        else
#endif
        {
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
        bool delivered=false;
        auto& subIds=P.subscription_ids;
        for (auto subId : subIds) {
            if (_subsResources.count(subId)){
                _subsResources[subId].cb(P.topic.data(), P.payload, P.plen, opts);
                delivered=true;
            }
        }
        if (!delivered)
#endif
        _cbMessage(P.topic.data(), P.payload, P.plen, opts);
        }
    }
}


void H4AsyncMQTT::_handlePacket(uint8_t* data, size_t len, int n_handled, uint8_t* copy){
    H4AMC_DUMP4(data,len);
    // if(data[0]==PINGRESP){ // THIS PREVENTS PROCESSING FURTHER MQTT PACKETS WITHIN A SHARED TCP PACKET.
    //     H4AMC_PRINT1("MQTT %s\n",mqttTraits::pktnames[data[0]]);
    //     return; // early bath
    // }
#if MQTT5
    if (len>MQTT_CONNECT_MAX_PACKET_SIZE) {
        H4AMC_PRINT1("LARGE inbound Packet size\n");
        _protocolError(REASON_PACKET_TOO_LARGE);
        return;
    }
#endif
    mqttTraits traits(data,len);
    auto pstate = traits.packet_state;
    int errorNotify = 0;
    if (pstate != mqttTraits::PacketState::OK) {
#if MQTT5
    H4AMC_MQTT_ReasonCode rcode = REASON_SUCCESS;
    switch (traits.packet_state)
    {
    case mqttTraits::PacketState::MALFORMED:
        H4AMC_PRINT1("Malformed packet! DISCONNECT\n");
        rcode = REASON_MALFORMED_PACKET;
        break;
    case mqttTraits::PacketState::INCOMPLETE:
        H4AMC_PRINT1("Incomplete packet!\n");
        rcode = REASON_PROTOCOL_ERROR;
        errorNotify = H4AMC_INCOMPLETE_PACKET;
        break;
    default:
        break;
    }
    _protocolError(rcode, errorNotify);
#else
        disconnect();
#endif
        return;
    }
    uint16_t id=traits.id;
#if MQTT5
    auto props = traits.properties ? *(traits.properties) : MQTT_Properties();
#endif
    auto rcode = traits.reasoncode;
    switch (traits.type){
    case PINGRESP: 
        break;
    case CONNACK:
    {
        switch (rcode)
        {
#if MQTT5
        case REASON_SUCCESS:
            if (!traits.properties) {
                H4AMC_PRINT1("INCOMPLIAT SERVER!\n");
                return;
            }
            _handleConnackProps(props);
            if (props.isAvailable(PROPERTY_AUTHENTICATION_METHOD))
                _handleAuthentication(rcode, props);
#else
        case 0x00:
#endif
        {
            if (_state!=H4AMC_TCP_CONNECTED){
                H4AMC_PRINT1("ERR Received a delayed CONNACK, Connection was dropped\n");
            } else {
                _state=H4AMC_RUNNING;
                bool session=traits.conackflags & 0x01;

                /* If the Client does not have Session State and receives Session Present set to 1 it MUST close
                    the Network Connection [MQTT-3.2.2-4]. If it wishes to restart with a new Session the Client can
                    reconnect using Clean Start set to 1.
                 */
                if (session && !_haveSessionData()) {
                    // _cleanStart = true;
                    _destroyClient();
                    return;
                } else if (!session){
                    _startClean();
                }
                H4AMC_PRINT1("CONNECTED FH=%u MaxPL=%u SESSION %s\n",_HAL_maxHeapBlock(),getMaxPayloadSize(),session ? "DIRTY":"CLEAN");
                _resendPartialTxns(session); // Resend before _cbConnect to no resend the same publishes/subsribes .. [ ] We might check for _state afterwards.
#if MQTT5
                _clearAliases();
#endif
                if (_state == H4AMC_RUNNING && _cbMQTTConnect)
#if MQTT5
                    _cbMQTTConnect({session,traits.properties->getUserProperties()});
#else
                    _cbMQTTConnect(session);
#endif
#if H4AMC_DEBUG
                SubscribePacket pango(this,"pango",0); // internal info during beta...will be moved back inside debug #ifdef
#endif
            }
        }
            break;
#if MQTT5
        case REASON_SERVER_MOVED:
            [[fallthrough]];
        case REASON_USE_ANOTHER_SERVER:
            _redirect(*(traits.properties));
            [[fallthrough]];
        default:
            H4AMC_PRINT1("CONNACK %s\n",mqttTraits::rcnames[static_cast<H4AMC_MQTT_ReasonCode>(rcode)]);
            _startClean(); // If a Server sends a CONNACK packet containing a non-zero Reason Code it MUST set Session Present to 0
#else
        default: 
            H4AMC_PRINT1("CONNACK %s\n",mqttTraits::connacknames[rcode]);
#endif
            _notify(H4AMC_CONNECT_FAIL, rcode);
            break;
        }
    }
        break;
    case PUBACK:
        [[fallthrough]];
    case PUBCOMP:
#if MQTT5
        if (rcode)
            _notify(traits.type==PUBACK?H4AMC_PUBACK_FAIL:H4AMC_PUBCOMP_FAIL, rcode);
        // [x] FLOW CONTROL .. DECREMENT inflight and send packets under holding.
        _publishEnd();
#endif
        if (_cbPublish) _cbPublish(id);
        [[fallthrough]];
    case UNSUBACK:
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
        if (_packSubIds.count(id)) {
            _confirmDeletion(id);
        }
#endif
        [[fallthrough]];
    case SUBACK:
    if (traits.type == SUBACK || traits.type == UNSUBACK) // Only for (UN)SUBACK.
    {
        bool badSub=false;
        for (auto &rc : traits.subreasons)
            if (rc>=0x80) badSub=true;

        if (badSub)
            _notify(traits.type==SUBACK?H4AMC_SUBSCRIBE_FAIL:H4AMC_UNSUBSCRIBE_FAIL, id);
    }
        _ACKoutbound(id); // MAX retries applies??
        break;
    case PUBREC:
        {
#if MQTT5
        if (rcode)
            _notify(H4AMC_PUBREC_FAIL, rcode);
        if (rcode>=REASON_UNSPECIFIED_ERROR)// [x] FLOW CONTROL .. Decrement inflight.
        {
            _ACKoutbound(id);
            _publishEnd();
        }
        else
#endif
        {
            _outbound[id].pubrec=true;
            PubrelPacket prp(this,id);
        }
        }
        break;
    case PUBREL:
        {
#if MQTT5
            if (rcode)
                _notify(H4AMC_PUBREL_FAIL, rcode);
#endif
            if(_inbound.count(id)) {
                _inbound.erase(id);
            } else _notify(H4AMC_INBOUND_QOS_ACK_FAIL,id);
            PubcompPacket pcp(this,id); // pubrel
        }
        break;
#if MQTT5
    case DISCONNECT: // Server disconnect.
    {
        switch(rcode) {
            case REASON_SERVER_MOVED:
            case REASON_USE_ANOTHER_SERVER:
                _redirect(props);
                [[fallthrough]];
            default:
                H4AMC_PRINT1("DISCONNECT %s\n", mqttTraits::rcnames[static_cast<H4AMC_MQTT_ReasonCode>(rcode)]);
                _notify(H4AMC_SERVER_DISCONNECT, rcode);
        }
        // _handleReasonString(props);
        _state = H4AMC_DISCONNECTED;
        if (_cbMQTTDisconnect)
            _cbMQTTDisconnect();
        _destroyClient();
    }
        break;
    case AUTH:
    {
        switch (rcode) {
        case REASON_SUCCESS:
        case REASON_CONTINUE_AUTHENTICATION:
            if (_authenticator){
                auto method = props.getStringProperty(PROPERTY_AUTHENTICATION_METHOD);
                auto data = props.getBinaryProperty(PROPERTY_AUTHENTICATION_DATA);
                auto ret = _authenticator->handle(std::make_pair(static_cast<H4AMC_MQTT_ReasonCode>(rcode),std::make_pair(method,data)));
                auto& authreason = ret.first;
                auto& authmethod = ret.second.first;
                auto& authdata = ret.second.second;
                AuthOptions opts{authreason}; // Might add reason string / user properties ...
                AuthenticationPacket auth(this,method,data,opts);
            }
            break;
            
        case REASON_RE_AUTHENTICATE:
            H4AMC_PRINT2("ONLY CLIENT CAN REAUTH!\n");
            _protocolError(REASON_PROTOCOL_ERROR);
            break;
        default:
            H4AMC_PRINT2("UNKNOWN AUTH REASON %02X\n", rcode);
            _protocolError(REASON_PROTOCOL_ERROR);
            break;
        }
    }
        break;
#endif // MQTT5
    default:
        if(traits.isPublish()) _handlePublish(traits);
        else {
            _notify(H4AMC_BOGUS_PACKET,data[0]);
            H4AMC_DUMP3(data,len);
#if MQTT5
            _protocolError(REASON_MALFORMED_PACKET);
#else
            // disconnect();
#endif
        }
        break;
    }
#if MQTT5
        _handleReasonString(props);
#endif
        if (traits.next.second)
        {
            H4AMC_PRINT4("Let's go round again! %p %d\n", traits.next.first, traits.next.second);
            if (n_handled < 10) // FOR LARGE VALUE AT DEBUG IT MAY CAUSE THREAD STACK OVERFLOW
                _handlePacket(traits.next.first, traits.next.second, n_handled + 1);
            else
            { // Relay off to another call tree to avoid memory exhaustion / stack overflow / watchdog reset
                H4AMC_PRINT4("Too many packets to handle, saving stack to another call tree\n");
                auto data=traits.next.first;
                auto len =traits.next.second;
                if (!copy) { // Only copy once
                    mbx m(traits.next.first, traits.next.second);
                    copy=data=m.get();
                    len=m.len;
                    // Serial.printf("copy=%p\n", copy);
                }

                h4.queueFunction([=, this]()
                                    { _handlePacket(data, len, 0, copy); });
            }
        }
        else if (copy) mbx::clear(copy); // Final packet
}

#if MQTT5
void H4AsyncMQTT::_redirect(MQTT_Properties& props)
{
    std::string reference;
    if (props.isAvailable(PROPERTY_SERVER_REFERENCE)) {
        H4AMC_PRINT1("Server Reference=%s\n", props.getStringProperty(PROPERTY_SERVER_REFERENCE).c_str());
        reference=props.getStringProperty(PROPERTY_SERVER_REFERENCE);
    } else {
        H4AMC_PRINT1("Server Reference not available\n");
    }
    if (_cbRedirect) (_cbRedirect(reference));
}

void H4AsyncMQTT::_protocolError(H4AMC_MQTT_ReasonCode reason, int errNotify)
{
    H4AMC_PRINT1("Protocol Error %u:%s err%d\n", reason, mqttTraits::rcnames[reason], errNotify);
    _notify(errNotify ? errNotify : H4AMC_PROTOCOL_ERROR, errNotify ? reason : 0);
    disconnect(reason);
}
void H4AsyncMQTT::_handleConnackProps(MQTT_Properties& props)
{
    if (_serverOptions) 
        delete _serverOptions;
    _serverOptions = new Server_Options;
    for (auto &p : props.available_properties) {
        switch (p) {
        case PROPERTY_SESSION_EXPIRY_INTERVAL:
            _serverOptions->session_expiry_interval = props.getNumericProperty(p);
            break;
        case PROPERTY_RECEIVE_MAXIMUM:
            _serverOptions->receive_max = props.getNumericProperty(p);
            break;
        case PROPERTY_MAXIMUM_QOS:
            _serverOptions->maximum_qos = props.getNumericProperty(p);
            break;
        case PROPERTY_RETAIN_AVAILABLE:
            _serverOptions->retain_available = props.getNumericProperty(p);
            break;
        case PROPERTY_MAXIMUM_PACKET_SIZE:
            _serverOptions->maximum_packet_size = props.getNumericProperty(p);
            break;
        case PROPERTY_ASSIGNED_CLIENT_IDENTIFIER:
            H4AMC_PRINT1("Assigned ClientID=%s\n", props.getStringProperty(p).c_str());
            _assignedClientId=props.getStringProperty(p);
            break;
        case PROPERTY_TOPIC_ALIAS_MAXIMUM:
            _serverOptions->topic_alias_max = props.getNumericProperty(p);
            break;
        case PROPERTY_REASON_STRING:
            H4AMC_PRINT1("Reason String=%s\n", props.getStringProperty(p).c_str());
            break;
        case PROPERTY_WILDCARD_SUBSCRIPTION_AVAILABLE:
            _serverOptions->wildcard_subscription_available = props.getNumericProperty(p);
            break;
        case PROPERTY_SUBSCRIPTION_IDENTIFIER_AVAILABLE:
            _serverOptions->subscriptions_identifiers_available = props.getNumericProperty(p);
            break;
        case PROPERTY_SHARED_SUBSCRIPTION_AVAILABLE:
            _serverOptions->shared_subscription_available = props.getNumericProperty(p);
            break;
        case PROPERTY_SERVER_KEEP_ALIVE:
        {
            auto v = props.getNumericProperty(p);
            if (v < _keepalive) {
                // [x] Change the PINGREQ timer to the new value.
                _startPinging(v);
            }
            break;
        }
        break;
        case PROPERTY_RESPONSE_INFORMATION:
            _serverOptions->response_information = props.getStringProperty(p);
            break;
        default:
            break;
        }
    }
// #if H4AMC_DEBUG
//     props.dump();
// #endif
}

void H4AsyncMQTT::_handleAuthentication(uint8_t reasoncode, MQTT_Properties& props)
{
    if (_authenticator){
        auto method = props.getStringProperty(PROPERTY_AUTHENTICATION_METHOD);
        auto data = props.getBinaryProperty(PROPERTY_AUTHENTICATION_DATA);
        auto ret = _authenticator->handle(std::make_pair(static_cast<H4AMC_MQTT_ReasonCode>(reasoncode),std::make_pair(method,data)));
        auto& authreason = ret.first;
        auto& authmethod = ret.second.first;
        auto& authdata = ret.second.second;
        AuthOptions opts{authreason}; // Might add reason string / user properties ...
        if (reasoncode==REASON_CONTINUE_AUTHENTICATION)
            AuthenticationPacket auth(this,method,data,opts);
    } else {
        _notify(H4AMC_NO_AUTHENTICATOR);
    }
}

bool H4AsyncMQTT::_availableTXAliasSpace() {
    if (_serverOptions)
        return _serverOptions->topic_alias_max > _tx_topic_alias.size();
    _notify(H4AMC_NO_SERVEROPTIONS);
    return false;
}
uint16_t H4AsyncMQTT::_getTXAlias(const std::string& topic) { // [ ] Test operations.
    auto& ta=_tx_topic_alias; 
    auto it = std::find(ta.begin(), ta.end(), topic);
    return std::distance(ta.begin(),it)+1;
}
#if H4AMC_MQTT5_INSERT_TOPIC_BY_ALIAS
bool H4AsyncMQTT::_insertTopicAlias(mqttTraits& m, std::vector<std::string>& remaining)
{
    // [ ] TODO: Clean the replacing function.
    // [ ] Might erase the Topic Alias property, to start with a clean set.. Or just clear the _tx_topic_alias after resending..
    // [x] fetch topic
    std::string topic = _getTXAliasTopic(m._topic_alias);
    H4AMC_PRINT2("_insertTopicAlias %s %d alias %d\n", topic.c_str(), m._topic_index, m._topic_alias);
    // H4AMC_DUMP4(m.get(), m.len);
    if (!topic.length() || !m._topic_index) {
        H4AMC_PRINT2("MISSING TOPIC ALIAS OR POS! %d %d\n", m.id, m._topic_index);
        return false;
    }
#define INSERTION_METHOD 1

#if INSERTION_METHOD // Reallocates only once.
    auto topicIt = std::find(remaining.begin(), remaining.end(), topic); 
    if (topicIt == remaining.end()) {
        H4AMC_PRINT2("Topic Alias has been already inserted in a previous packet\n");
        return false;
    }
    // [x] insert topic
    std::vector<uint8_t> copy; 
    // Copy First byte of Fixed header
    std::copy_n(m.get(), 1, std::back_inserter(copy));
    auto rl_len = H4AMC_Helpers::varBytesLength(m.remlen);
    m.remlen += topic.length();
    auto diff_bytes = H4AMC_Helpers::varBytesLength(m.remlen) - rl_len;
    std::vector<uint8_t> frag1{m.get()+1+rl_len, m.get()+m._topic_index};
    std::vector<uint8_t> frag2{m.get()+m._topic_index+2, m.get()+m.len};

    auto ptr = mbx::realloc(m.get(), m.len + topic.length() + diff_bytes);
    
    uint32_t oldLen = m.len;
    if (ptr != nullptr){
        m.set(ptr);
        m.len += topic.length()+diff_bytes;

        copy.reserve(m.len);

        // Encode remlen to a buffer
        uint8_t rembuf[rl_len+diff_bytes];
        H4AMC_Helpers::encodeVariableByteInteger(rembuf, m.remlen);

        // Insert remlen
        copy.insert(copy.end(), rembuf, rembuf+rl_len+diff_bytes);

        // Insert the date between fixed header and the topic index. 
        // [ ] Check if more than one topic is inserted. (Specification does not mention whether one message can only carry one topic alias, or more)
        copy.insert(copy.end(), std::make_move_iterator(frag1.begin()), std::make_move_iterator(frag1.end()));
        frag1.clear();

        // Encode topic to a buffer
        size_t addmeta=topic.length()+2;
        uint8_t buf[addmeta];
        H4AMC_Helpers::encodestring(&buf[0],topic);
        
        // Insert the topic data
        copy.insert(copy.end(), buf, buf+addmeta);

        // Insert the remaining data
        copy.insert(copy.end(), std::make_move_iterator(frag2.begin()), std::make_move_iterator(frag2.end()));
        frag2.clear();

        std::copy_n(copy.begin(), m.len, m.get());
        H4AMC_PRINT4("INSERTED TOPIC NEW BUFFER:\n");
        H4AMC_DUMP4(m.get(), m.len);
        remaining.erase(topicIt);
        return true;
    }
#else // The Replacing method: Which will require a reallocation
    // [x] insert topic
    std::vector<uint8_t> copy; 
    copy.reserve(m.len);

    std::copy_n(m.get(), m.len, std::back_inserter(copy));

    auto rem_bytes = H4AMC_Helpers::varBytesLength(m.remlen);
    m.remlen += topic.length();
    auto diff_bytes = H4AMC_Helpers::varBytesLength(m.remlen) - rem_bytes;

    auto ptr = mbx::realloc(m.get(), m.len + topic.length() + diff_bytes);
    if (ptr != nullptr){
        m.set(ptr);
        m.len += topic.length()+diff_bytes;

        // Encode remlen to a buffer
        uint8_t rembuf[rem_bytes+diff_bytes];
        H4AMC_Helpers::encodeVariableByteInteger(rembuf, m.remlen);

        // Replace remlen
        for (int i=0;i<rem_bytes;i++) copy.erase(copy.begin()+1); // Remove current remlen
        copy.insert(copy.begin()+1, rembuf, rembuf+rem_bytes+diff_bytes);

        // Encode topic to a buffer
        size_t addmeta=topic.length()+2;
        uint8_t buf[addmeta];
        H4AMC_Helpers::encodestring(&buf[0],topic);
        
        // Replace topic
        copy.erase(copy.begin()+m._topic_index); // Erase Topic Length (Field0)
        copy.erase(copy.begin()+m._topic_index); // Erase Topic Length (Field1)
        copy.insert(copy.begin()+m._topic_index, buf, buf+addmeta); // A reallocation which would contribute to heap fragmentation / or abort with OOM

        std::copy_n(copy.begin(), m.len, m.get());
        H4AMC_PRINT4("INSERTED TOPIC NEW BUFFER:\n");
        H4AMC_DUMP4(m.get(), m.len);
        return true;
    }
#endif

    return false;
}
#endif

bool H4AsyncMQTT::addStaticUserProp(PacketHeader header, H4AMC_USER_PROPERTIES user_properties){
    if (header == CONNACK || header == SUBACK || header == UNSUBACK)
        return false;
    _addUserProp(header, std::make_shared<H4AMC_USER_PROPERTIES>(user_properties));
    return true;
}

bool H4AsyncMQTT::addStaticUserProp(std::initializer_list<PacketHeader> headers, H4AMC_USER_PROPERTIES user_properties){
    for (auto header : headers)
        if (header == CONNACK || header == SUBACK || header == UNSUBACK)
            return false;
    auto shared = std::make_shared<H4AMC_USER_PROPERTIES>(user_properties);
    for(auto header : headers)
        _addUserProp(header, shared);
    return true;
}

bool H4AsyncMQTT::addDynamicUserProp(PacketHeader header, H4AMC_FN_DYN_PROPS f){
    if (header == CONNACK || header == SUBACK || header == UNSUBACK)
        return false;
    _addDynamicUserProp(header, f);
    return true;
}

bool H4AsyncMQTT::addDynamicUserProp(std::initializer_list<PacketHeader> headers, H4AMC_FN_DYN_PROPS f){
    for (auto header : headers)
        if (header == CONNACK || header == SUBACK || header == UNSUBACK)
            return false;
    
    for(auto header : headers)
        _addDynamicUserProp(header, f);
    return true;
}
#endif // MQTT5

void H4AsyncMQTT::_handlePublish(mqttTraits P){
    uint8_t qos=P.qos;
    uint16_t id=P.id;
    H4AMC_PRINT4("_handlePublish %s id=%d @ QoS%d R=%s DUP=%d PL@%08X PLEN=%d\n",P.topic.data(),id,qos,P.retain ? "true":"false",P.dup,P.payload,P.plen);
#if MQTT5
    if (P.properties && P.properties->isAvailable(PROPERTY_TOPIC_ALIAS)) {
        // Register/Update topic alias
        uint16_t alias = P.properties->getNumericProperty(PROPERTY_TOPIC_ALIAS);
        H4AMC_PRINT2("RECEIVED TOPIC ALIAS %d EXIST %d\n", alias, _rx_topic_alias.count(alias));

        if (_rx_topic_alias.count(alias) && !P.topic.length()) 
        {
            H4AMC_PRINT2("FETCHING TOPIC %s\n", _rx_topic_alias[alias].c_str());
            P.topic = _rx_topic_alias[alias];
        } 
        else if (alias && alias < MQTT5_RX_TOPIC_ALIAS_MAXIMUM && P.topic.length()) // There's Topic and a valid alias
        { 
            H4AMC_PRINT2("ASSIGNING TOPIC ALIAS %d to %s\n", alias, P.topic.c_str());
            _rx_topic_alias[alias] = P.topic;
        } 
        else if (P.topic.length()) // It's an invalid alias (alias==0 OR Topic Alias Exceeded)
        {
            H4AMC_PRINT2("VALID TOPIC - INVALID ALIAS\n");
            _protocolError(REASON_TOPIC_ALIAS_INVALID);
            return;
        } // else NO Topic Alias Mapping found (P.topic.length()==0)
    }
    if (!P.topic.length()) {
        H4AMC_PRINT1("NO Topic Alias Mapping Exist!\n");
        _protocolError(REASON_PROTOCOL_ERROR);
        return;
    }
#endif
    if (qos<2 || !_inbound.count(id)); // For QoS2, Only dispatch to the user once; _inbound holds the id of publishes under PUBREL from us to the server.
        _hpDespatch(P);

    switch(qos){
        // case 0:
        //     _hpDespatch(P);
        //     break;
        case 1:
            { 
                // _hpDespatch(P);
                PubackPacket pap(this,id);
            }
            break;
        case 2:
            {
                _inbound.insert(id);
                // PublishPacket pub(this,P.topic.data(),qos,P.retain,P.payload,P.plen,0,id); // build and HOLD until PUBREL force dup=0
                PubrecPacket pcpthis(this,id);
            }
            break;
    }
}

void H4AsyncMQTT::_notify(int e,int info){ 
    H4AMC_PRINT1("NOTIFY e=%d inf=%d\n",e,info);
    if((e||info) && _cbMQTTError) _cbMQTTError(e,info);
}

void H4AsyncMQTT::_resendPartialTxns(bool availSession){ // [ ] Rename to handleSession
    // Check whether the messages are outdated...
    // OR regularly resend them...?
    H4AMC_PRINT2("_resendPartialTxns(%d) _outbound=%d\n", availSession, _outbound.size());
    std::vector<uint16_t> morituri;
    H4AMC_PACKET_MAP copy;
#if H4AMC_MQTT5_INSERT_TOPIC_BY_ALIAS
    auto aliasCopy = _tx_topic_alias;
#endif
    for(auto const& o:_outbound){
        // [ ] Try them over queueFunction() because of memory runout (if to _insertTopicAlias to large payloads)
        mqttTraits m=o.second;
        if(--(m.retries)){
            if(m.pubrec){
                if (!availSession) morituri.push_back(m.id);
                else {
                    H4AMC_PRINT3("WE ARE PUBREC'D ATTEMPT @ QOS2: SEND %d PUBREL\n",m.id);
                    PubrelPacket prp(this,m.id);
                }
            }
            else {
                if (m.isPublish()) {  // set dup & resend ONLY for PUBlISH packets (..?)
                    H4AMC_PRINT3("SET DUP %d\n", m.id);
                    m.get()[0] |= 0x08;
#if MQTT5
                    if (!m.topic.length()) {
#if H4AMC_MQTT5_INSERT_TOPIC_BY_ALIAS
                        if (_insertTopicAlias(m, aliasCopy)) copy[m.id]=m;
                        else
#endif
                        {
                            morituri.push_back(m.id);
                            continue;
                        }
                    }
#endif
                }
                H4AMC_PRINT3("RESEND %d\n", m.id);
                H4AMC_DUMP4(m.get(), m.len);
#if MQTT5
                if (m.isPublish() && m.id && _canPublishQoS(m.id)){
                    _send(m.get(),m.len,false);
                }
#else
                _send(m.get(),m.len,false);
#endif
            }
        }
        else {
            H4AMC_PRINT4("NO JOY AFTER %d ATTEMPTS: QOS FAIL\n",H4AMC_MAX_RETRIES);
            morituri.push_back(m.id); // all hope exhausted TODO: reconnect?
        }
    }
    for(auto const& i:morituri) _ACKoutbound(i);
    for(auto const& c:copy) _outbound[c.first]=c.second;

#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
    if (!availSession) {
        // [x] Clean subscription id maps.
        _subsResources.clear();
        _packSubIds.clear();
    }
#endif
}

void H4AsyncMQTT::_runGuard(H4AMC_FN_VOID f){
    if(_state==H4AMC_RUNNING) f();
    else _notify(0,H4AMC_NOT_RUNNING);
}

void H4AsyncMQTT::_startReconnector(){ h4.every(5000,[this]{ _connect(); },nullptr,H4AMC_RCX_ID,true); }
//
//      PUBLIC
//
void H4AsyncMQTT::connect(const char* url,const char* auth,const char* pass,const char* clientId){
    H4AMC_PRINT1("H4AsyncMQTT::connect(%s,%s,%s,%s)\n",url,auth,pass,clientId);
    if(_state==H4AMC_DISCONNECTED){
        // _h4atClient=new H4AsyncClient;
        _url=url;
        _username = auth;
        _password = pass;
        // _cleanStart = clean;
        _clientId = (strlen(clientId) ? clientId:_HAL_uniqueName("H4AMC" H4AMC_VERSION));
        informNetworkState(H4AMC_NETWORK_CONNECTED); // Assume being called on Network Connect.
        _connect();
        H4AsyncClient::_scavenge();
    } else if(_cbMQTTError) _cbMQTTError(ERR_ISCONN,H4AMC_USER_LOGIC_ERROR);
}

void H4AsyncMQTT::disconnect(H4AMC_MQTT_ReasonCode reason) { // [ ] DisconnectPacket ...
    static uint8_t G[] = {DISCONNECT, reason}; 
    // [ ] Properties...
        // Session expiry interval
        // Reason String
        // User Property
    H4AMC_PRINT1("USER DCX\n");
    if(_state>=H4AMC_TCP_CONNECTED) _h4atClient->TX(G,2,false); // Generalize to TCP_CONNECTED for MQTT 5.0 prior-Successful-CONNACK disconnect() calls
    else _notify(ERR_CONN,H4AMC_USER_LOGIC_ERROR);
    _destroyClient();
}

std::string H4AsyncMQTT::errorstring(int e){
    #ifdef H4AMC_DEBUG
        if(_errorNames.count(e)) return _errorNames[e];
        else return stringFromInt(e); 
    #else
        return stringFromInt(e); 
    #endif
}

PacketID H4AsyncMQTT::publish(const char* topic, const uint8_t* payload, size_t length, uint8_t qos, H4AMC_PublishOptions opts_retain) { return _runGuard([=, this, &opts_retain]{ PublishPacket pub(this,topic,qos,payload,length,opts_retain); return pub.getId(); }, (PacketID)0); }

PacketID H4AsyncMQTT::publish(const char* topic, const char* payload, size_t length, uint8_t qos, H4AMC_PublishOptions opts_retain) { 
#if MQTT5
    opts_retain.props.payload_format_indicator=H4AMC_PAYLOAD_FORMAT_STRING;
#endif
    return publish(topic, reinterpret_cast<const uint8_t*>(payload), length, qos, opts_retain);
}

void H4AsyncMQTT::setWill(const char* topic, uint8_t qos, const char* payload, H4AMC_WillOptions opts) {
    H4AMC_PRINT2("setWill *%s* q=%d r=%d *%s*\n",topic,qos,opts.getRetained(),payload);
    _will.topic = topic;
    _will.qos = qos;
    _will.retain = opts.getRetained();
    _will.payload = payload;
#if MQTT5
    _will.props = opts.getProperties();
#endif
}

uint32_t H4AsyncMQTT::subscribe(const char* topic, H4AMC_SubscriptionOptions opts_qos) { return _runGuard([=, this, &opts_qos](void){ SubscribePacket sub(this,topic,opts_qos); return sub.getId(); }, (uint32_t)0); }

uint32_t H4AsyncMQTT::subscribe(std::initializer_list<const char*> topix, H4AMC_SubscriptionOptions opts_qos) { return _runGuard([=, this, &opts_qos]{ SubscribePacket sub(this,topix,opts_qos); return sub.getId(); }, (uint32_t)0); }

void H4AsyncMQTT::unsubscribe(const char* topic) {_runGuard([=, this]{ UnsubscribePacket usp(this,topic); }); }

void H4AsyncMQTT::unsubscribe(std::initializer_list<const char*> topix) {_runGuard([=, this, &topix]{  UnsubscribePacket usp(this,topix); }); }
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
void H4AsyncMQTT::unsubscribe(uint32_t subscription_id) {
    if (_subsResources.count(subscription_id)) { 
        _runGuard([=, this]{ UnsubscribePacket usp(this, _subsResources[subscription_id].topix); });
    } else {
        _notify(0,H4AMC_SUBID_NOT_FOUND);
    }
}
#endif
//
//
//
void H4AsyncMQTT::dump(){
#if H4AMC_DEBUG

    H4AMC_PRINT4("DUMP ALL %d PACKETS OUTBOUND\n",_outbound.size());
    for(auto & p:_outbound) p.second.dump();

    H4AMC_PRINT4("DUMP ALL %d PACKETS INBOUND\n",_inbound.size());
    for(auto & p:_inbound) H4AMC_PRINT4("%d\t",p);

    H4AMC_PRINT4("\n");
#endif
}

bool H4AsyncMQTT::secureTLS(const u8_t *ca, size_t ca_len, const u8_t *privkey, size_t privkey_len, const u8_t *privkey_pass, size_t privkey_pass_len, const u8_t *cert, size_t cert_len)
{
#if H4AT_TLS
    // Copy to internals 
    H4AMC_PRINT2("secureTLS(%p,%d,%p,%d,%p,%d,%p,%d)\n",ca,ca_len,privkey,privkey_len,privkey_pass,privkey_pass_len,cert,cert_len);

    if (ca) {
        _caCert.reserve(ca_len);
        std::copy_n(ca, ca_len, std::back_inserter(_caCert));
    }
    if (privkey) {
        _privkey.reserve(privkey_len);
        std::copy_n(privkey, privkey_len, std::back_inserter(_privkey));
    }
    if (privkey_pass) {
        _privkeyPass.reserve(privkey_pass_len);
        std::copy_n(privkey_pass, privkey_pass_len, std::back_inserter(_privkeyPass));
    }
    if (cert) {
        _clientCert.reserve(cert_len);
        std::copy_n(cert, cert_len, std::back_inserter(_clientCert));
    }
    return true;
#else
    H4AMC_PRINT1("TLS is not activated within H4AsyncTCP\n");
    _notify(H4AMC_NO_SSL);
	return false;
#endif
}
#if MQTT5
H4AMC_AuthInformation SCRAM_Authenticator::start()
{
    state=CLIENT_FIRST_SENT;
	return H4AMC_AuthInformation(std::make_pair(REASON_CONTINUE_AUTHENTICATION, std::make_pair(_method, _client_first)));
}

H4AMC_AuthInformation SCRAM_Authenticator::handle(H4AMC_AuthInformation data)
{
    auto rcode = data.first;
    H4AMC_AuthInformation result;
/* // [ ] Another mechanism to not reply (other than reasoncode on caller side _handleAuthentication() ?)
    switch (rcode) 
    {
    case REASON_SUCCESS: // Doesn't really return usable data. just return empty H4AMC_AuthInformation{};
        break;
    case REASON_CONTINUE_AUTHENTICATION:
        break;
    case REASON_BAD_AUTHENTICATION_METHOD:
        break;
    default:
        break;
    } 
    */
    if (rcode == REASON_CONTINUE_AUTHENTICATION) {
        auto method = data.second.first;
        auto authdata = data.second.second;
        if (method!=_method){
            // ERROR!
        }

        switch(state) {
            case CLIENT_FIRST_SENT:
            // [ ] Digest authdata 
            // [ ] Return result
                state=CLIENT_FINAL_SENT;
                break;
            case CLIENT_FINAL_SENT:
            // [ ] Digest authdata 
            // [ ] Return result
                state=COMPLETE;
                break;
            default:
                break;
        }
        
    } else if (rcode == REASON_SUCCESS) { // Within SUCCESS CONNACK
    // Might store data...
    }
	return result;
}

#endif