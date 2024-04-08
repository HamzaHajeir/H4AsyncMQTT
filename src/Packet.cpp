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
#include<Packet.h>

#if MQTT5
H4AMC_USER_PROPERTIES dummy;
H4AMC_USER_PROPERTIES connectProps {{"User-Agent","H4AsyncMQTT v" H4AMC_VERSION}};

uint32_t Packet::__fetchSize(H4AMC_USER_PROPERTIES &props)
{
    uint32_t total_size=0;
    for (auto& up : props) { 
        total_size += 5 + up.first.size() + up.second.size(); // 1 PropID 2 keylen 2valuelen
    }
    return total_size;
}

uint32_t Packet::__fetchPassedProps(H4AMC_USER_PROPERTIES &props)
{
    return __fetchSize(props);
}

uint32_t Packet::__fetchStaticProps()
{
    auto isPublish = (_controlcode & 0xf0) == PUBLISH;

    auto header = static_cast<PacketHeader>(_controlcode);
    if (isPublish) header=PUBLISH;
    if (_parent->_user_static_props.count(header)){
        auto u_props = *(_parent->_user_static_props[header]);
        return __fetchSize(u_props);
    }
    return 0;
}

uint32_t Packet::__fetchDynamicProps()
{
    auto isPublish = (_controlcode & 0xf0) == PUBLISH;
    auto header = static_cast<PacketHeader>(_controlcode);
    if (isPublish) header=PUBLISH;
    if (_parent->_user_dynamic_props.count(header)) {
        auto &cbDynamic = _parent->_user_dynamic_props[header];
        _dynProps = std::make_shared<H4AMC_USER_PROPERTIES>(cbDynamic(header));
        return __fetchSize(*_dynProps);
    }
    return 0;
}

uint8_t *Packet::__embedProps(uint8_t* p, H4AMC_USER_PROPERTIES &props)
{
    p=MQTT_Properties::serializeUserProperties(p,props);
    return p;
}

uint8_t *Packet::__embedPassedProps(uint8_t *p, H4AMC_USER_PROPERTIES &props)
{
    if (props.size()) {
        p=__embedProps(p, props);
    }
	return p;
}

uint8_t *Packet::__embedStaticProps(uint8_t *p)
{
    auto isPublish = (_controlcode & 0xf0) == PUBLISH;
    auto header = static_cast<PacketHeader>(_controlcode);
    if (isPublish) header=PUBLISH;
    if (_parent->_user_static_props.count(header)) {
        p=__embedProps(p,*_parent->_user_static_props[header]);
    }
    return p;
}

uint8_t *Packet::__embedDynamicProps(uint8_t *p)
{
    if (_dynProps) {
        p=__embedProps(p, *_dynProps);
    }
    return p;
}
#endif

void Packet::_build(){
	uint8_t* virgin;
    _begin();
    if(_id) _bs+=2;
    // calc rl
    uint32_t X=_bs;
    std::vector<uint8_t> rl;
    uint8_t encodedByte;
    do{
        encodedByte = X % 128;
        X = X / 128;
        if ( X > 0 ) encodedByte = encodedByte | 128;
        rl.push_back(encodedByte);
    } while ( X > 0 );
    _bs+=1+rl.size();
    uint8_t* snd_buf=virgin=mbx::getMemory(_bs);
    H4AMC_PRINT4("PACKET CREATED @ %p len=%d\n",snd_buf,_bs);
    if(snd_buf){
        *snd_buf++=_controlcode;
        for(auto const& r:rl) *snd_buf++=r;
        snd_buf=_varHeader(snd_buf);
        _protocolPayload(snd_buf,virgin);
        auto isPublish = (_controlcode & 0xf0) == PUBLISH;
#if MQTT5
        if (_controlcode!=CONNECT && _controlcode!=AUTH && _parent->_serverOptions->maximum_packet_size < _bs) {
            H4AMC_PRINT1("Server doesn't permit that size\n");
            _notify(H4AMC_OUTBOUND_PUB_TOO_BIG, _bs);
            if (_id) _parent->_ACKoutbound(_id); // Discard and clear
            else mbx::clear(virgin); // clear data
            return;
        }
#endif
        // [x] APPLY FLOW CONTROL if PUBLISH
#if MQTT5
        bool send=true;
        if (isPublish && _id){
            send = _parent->_canPublishQoS(_id);
        }
        if (send)
#endif
        _parent->_send(virgin,_bs,!_id); // Might also copy further for AUTH/DISCONNECT/...

        if (!_id) mbx::clear(virgin); // For QoS0, AUTH, CONNECT (OR Any Packet other that PUBLISH QoS>0 && Sub/UnSub)
    } else _parent->_notify(ERR_MEM,_bs);
}

void Packet::_idGarbage(uint16_t id){
    uint8_t hi=(id & 0xff00) >> 8;
    uint8_t lo=id & 0xff;
    uint8_t  G[]={_controlcode,2,hi,lo};
#if H4AMC_DEBUG
    mqttTraits(&G[0],4);
#endif
    _parent->_send(&G[0],4,true);
}

void Packet::_multiTopic(std::set<std::string> topics,H4AMC_SubscriptionOptions opts){
    _id=++_parent->_nextId;
#if MQTT5
#if H4AMC_ENABLE_CHECKS
        auto& svrOpts = _parent->_serverOptions;
        auto& shareSubAvailable = svrOpts->shared_subscription_available;
        auto& wildcardAvailable = svrOpts->wildcard_subscription_available;
        for (auto& t:topics) {
            auto levels = split(t, "/");
            auto shareTopic = levels[0] == "$share";

            auto multilevelwild = std::find(levels.begin(), levels.end(), "#");
            auto contains_multi = multilevelwild!=levels.end();
            if (contains_multi && multilevelwild!=levels.end()-1) {
                H4AMC_PRINT2("ERROR # comes in middle!\n");
                _notify(H4AMC_BAD_TOPIC);
                return;
            }
            if (!wildcardAvailable) {
                auto singlelevelwild = std::find(levels.begin(), levels.end(), "+");
                auto contains_single = singlelevelwild != levels.end();
                if (contains_multi || contains_single) {
                    _notify(H4AMC_WILDCARD_UNAVAILABLE);
                    return;
                }
            }

            if (shareTopic) {
                if (shareSubAvailable) {
                    if (levels[1]=="+" || levels[1]=="#") {
                        _notify(H4AMC_BAD_SHARED_TOPIC);
                        H4AMC_PRINT1("ERROR: Bad Share Name");
                        return;
                    }
                } else {
                    _notify(H4AMC_SHARED_SUBS_UNAVAILABLE);
                    H4AMC_PRINT1("ERROR: Shared subscriptions is not supported by the server [topic=%s]\n", t);
                    return;
                }
            }
        }
#endif
#endif
    _begin=[&,this]{
        for(auto &t:topics){
            _bs+=(_controlcode==SUBSCRIBE ? 3:2)+t.length(); // 3 because of subscription options/QoS (3.1.1).
        }
#if MQTT5
        bool use_subId = false;
#endif
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
        // [ ] If not found a matching topics, fetch the available matching topics from a the set of topics pulling them out. (Is it necessary/have a use case?)
        bool unsub = _controlcode == UNSUBSCRIBE;
        use_subId = svrOpts->subscriptions_identifiers_available && _controlcode == SUBSCRIBE && (opts.getCallback() != nullptr || unsub); // [ ] UNSUBSCRIBE Management

        uint32_t used_subId;
        if (use_subId) { // [x] Verify subID management.
            auto it = std::find_if(_parent->_subsResources.begin(), _parent->_subsResources.end(), [&topics](const std::pair<uint32_t, SubscriptionResource>& pair) { return pair.second.topix==topics; });
            if (unsub) {
                if (it != _parent->_subsResources.end()) {
                    _parent->_proposeDeletion(_id, it->first);
                }
            } else { // It's SUBSCRIBE
                if (it != _parent->_subsResources.end()) {
                    // Reuse the subId;
                    used_subId = it->first;
                    // override the cbf;
                    it->second.cb = opts.getCallback();
                } else {
                    used_subId=++_parent->subId;
                    _parent->_subsResources[used_subId] = SubscriptionResource{opts.getCallback(), topics};
                }
                _propertyLength += H4AMC_Helpers::varBytesLength(used_subId) + 1; // One for the Property ID, one for the length of the value.
            }
        }
#endif // MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
#if MQTT5
        // [x] Add the properties to the length (property length) + store them
        _propertyLength += _fetchUserProperties(opts.getUserProperties());

        _bs += _propertyLength + H4AMC_Helpers::varBytesLength(_propertyLength);

        _properties = [=, &opts](uint8_t* p){
            p = H4AMC_Helpers::encodeVariableByteInteger(p, _propertyLength);
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
            if (use_subId && !unsub)
                p = MQTT_Properties::serializeProperty(PROPERTY_SUBSCRIPTION_IDENTIFIER, p, used_subId);
#endif // MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
            p = _serializeUserProperties(p, opts.getUserProperties());
            return p;
        };
#endif // MQTT5
    };
    _varHeader = [this](uint8_t *p)
    {
        p = _poke16(p, _id);
#if MQTT5
        p = _properties(p);
#endif // MQTT5
        return p;
    };

    _protocolPayload=[=, &opts](uint8_t* p,uint8_t* base){
        for(auto const& t:topics){ // not blocks because it's passed already.
            size_t n=t.size();
            p=_poke16(p,n);
            if (n) memcpy(p,t.data(),n);
            p+=n;
            if(_controlcode==SUBSCRIBE){
                *p = opts.getQos();
#if MQTT5
                *p |= opts.getNoLocal() << SUBSCRIPTION_OPTION_NO_LOCAL_SHIFT;
                *p |= opts.getRetainAsPublished() << SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED_SHIFT;
                *p |= opts.getRetainHandling() << SUBSCRIPTION_OPTION_RETAIN_HANDLING_SHIFT;
#endif
                p++;
            }
        }
        mqttTraits T(base,_bs);
        H4AsyncMQTT::_outbound[_id]=T;  // To be released on (UN)SUBACK
    };

    _build();
}

uint8_t* Packet::_poke16(uint8_t* p,uint16_t u){
    return H4AMC_Helpers::poke16(p,u);
}

void Packet::_stringblock(const std::string& s){ 
    size_t sz=s.size();
    _bs+=sz+2;
    _blox.push(mbx((uint8_t*) s.data(),sz,s.length())); // s.length() for copy.
}

uint8_t *Packet::_serializeblock(uint8_t* p_pos, mbx block)
{
    uint16_t n=block.len;
    uint8_t* p=block.data;
    p_pos=_poke16(p_pos,n);
    if (n) // For empty strings
        memcpy(p_pos,p,n);
    p_pos+=n;
	return p_pos;
}
uint8_t* Packet::_applyfront(uint8_t* p_pos)
{
    if (_blox.size()){
        mbx tmp = _blox.front();
        p_pos = _serializeblock(p_pos, tmp);
        tmp.clear();
        _blox.pop();
    }
    return p_pos;
}

ConnectPacket::ConnectPacket(H4AsyncMQTT* p): Packet(p,CONNECT){
    _bs=10;

    _begin=[=]{
        // if((_parent->_cleanStart && !_parent->_haveSessionData()) || !H4AsyncMQTT::_outbound.size()) protocol[7]|=CLEAN_START;
        if(!_parent->_haveSessionData()) protocol[7]|=CLEAN_START;
        // clientID --> Will properties --> wil lTopic --> willPayload --> username --> password
        _stringblock(_parent->_clientId);
        if(_parent->_will.topic.size()){
#if MQTT5
        uint32_t willPropLen=0;
        auto& props = _parent->_will.props;
        if (props.payload_format_indicator) willPropLen += 2;
        if (props.message_expiry_interval)  willPropLen += 5;
        if (props.will_delay_interval)      willPropLen += 5;
        if (props.content_type.length())    willPropLen += 3 + props.content_type.length(); // 3= 1 PropID + 2 Length
        if (props.response_topic.length())  willPropLen += 3 + props.response_topic.length();
        if (props.correlation_data.size())  willPropLen += 3 + props.correlation_data.size();
        if (props.user_properties.size())   willPropLen += __fetchPassedProps(props.user_properties);
        _bs += willPropLen + H4AMC_Helpers::varBytesLength(willPropLen);
        // Serial.printf("willPropLen=%d\n", willPropLen);

        if (willPropLen) {
            _willproperties = [&, willPropLen](uint8_t* p) {
                p=H4AMC_Helpers::encodeVariableByteInteger(p, willPropLen);
                if (props.payload_format_indicator) p = MQTT_Properties::serializeProperty(PROPERTY_PAYLOAD_FORMAT_INDICATOR, p, props.payload_format_indicator);
                if (props.message_expiry_interval)  p = MQTT_Properties::serializeProperty(PROPERTY_MESSAGE_EXPIRY_INTERVAL, p, props.message_expiry_interval);
                if (props.will_delay_interval)      p = MQTT_Properties::serializeProperty(PROPERTY_WILL_DELAY_INTERVAL, p, props.will_delay_interval);
                if (props.content_type.length())    p = MQTT_Properties::serializeProperty(PROPERTY_CONTENT_TYPE, p, props.content_type);
                if (props.response_topic.length())  p = MQTT_Properties::serializeProperty(PROPERTY_RESPONSE_TOPIC, p, props.response_topic);
                if (props.correlation_data.size())  p = MQTT_Properties::serializeProperty(PROPERTY_CORRELATION_DATA, p, props.correlation_data);
                if (props.user_properties.size())   p = __embedProps(p, props.user_properties);
                return p;
            };
        }
#endif
            if(_parent->_will.retain) protocol[7]|=WILL_RETAIN;
            if(_parent->_will.qos) protocol[7]|=(_parent->_will.qos==1) ? WILL_QOS1:WILL_QOS2;
            _stringblock(_parent->_will.topic);
            _stringblock(_parent->_will.payload);
            protocol[7]|=WILL;
        }
        if(_parent->_username.size()){
            _stringblock(_parent->_username);
            protocol[7]|=USERNAME;
        }
        if(_parent->_password.size()){
            _stringblock(_parent->_password);
            protocol[7]|=PASSWORD;
        }
#if MQTT5
        // [x] Add the properties to the length (property length) + store them

        // **Session Expiry Interval wont be used. 0 is the default value**
        _propertyLength += 5; // SESSION EXPIRY INTERVAL
        _propertyLength += 3; // RECEIVE_MAXIMUM ID + 2 bytes
        _propertyLength += 5; // MAX PACKET SIZE ID + 4 bytes
        _propertyLength += 3; // TOPIC ALIAS MAXIMUM ID + 2 bytes
#if MQTT_CONNECT_REQUEST_PROBLEM_INFORMATION
        _propertyLength += 2; // REQUEST PROBLEM INFORMATION ID + 1 byte
#endif
#if MQTT_CONNECT_REQUEST_RESPONSE_INFORMATION
        _propertyLength += 2; // REQUEST RESPONSE INFORMATION ID + 1 byte
#endif
        _propertyLength += _fetchUserProperties(connectProps); // CALL ONLY ONCE per Packet::Packet()
        if (_parent->_authenticator) {
            auto ret = _parent->_authenticator->start();
            _authmethod = ret.second.first;
            _authdata = ret.second.second;
            _propertyLength += 3 + _authmethod.length();
            _propertyLength += 3 + _authdata.size();
        }
        _bs += _propertyLength + H4AMC_Helpers::varBytesLength(_propertyLength);

        _properties=[=](uint8_t* p){
            p = H4AMC_Helpers::encodeVariableByteInteger(p, _propertyLength);
            p = MQTT_Properties::serializeProperty(PROPERTY_SESSION_EXPIRY_INTERVAL, p, MQTT_CONNECT_SESSION_EXPRITY_INTERVAL);
            p = MQTT_Properties::serializeProperty(PROPERTY_RECEIVE_MAXIMUM, p, MQTT_CONNECT_RECEIVE_MAXIMUM);
            p = MQTT_Properties::serializeProperty(PROPERTY_MAXIMUM_PACKET_SIZE, p, MQTT_CONNECT_MAX_PACKET_SIZE);
            p = MQTT_Properties::serializeProperty(PROPERTY_TOPIC_ALIAS_MAXIMUM, p, MQTT_CONNECT_TOPIC_ALIAS_MAX);
            if (_authmethod.length()) {
                p = MQTT_Properties::serializeProperty(PROPERTY_AUTHENTICATION_METHOD, p, _authmethod);
                p = MQTT_Properties::serializeProperty(PROPERTY_AUTHENTICATION_DATA, p, _authdata);
            }
#if MQTT_CONNECT_REQUEST_PROBLEM_INFORMATION
            p = MQTT_Properties::serializeProperty(PROPERTY_REQUEST_PROBLEM_INFORMATION, p, MQTT_CONNECT_REQUEST_PROBLEM_INFORMATION);
#endif
#if MQTT_CONNECT_REQUEST_RESPONSE_INFORMATION
            p = MQTT_Properties::serializeProperty(PROPERTY_REQUEST_RESPONSE_INFORMATION, p, MQTT_CONNECT_REQUEST_RESPONSE_INFORMATION);
#endif
            p = _serializeUserProperties(p, connectProps);
            return p;
        };

#endif // MQTT5
    };

    _varHeader=[=](uint8_t* p){
        memcpy(p,&protocol,8);p+=8;
        p=_poke16(p,_parent->_getKeepAliveSeconds());
#if MQTT5
        p=_properties(p);
#endif
        return p;
    };

    _protocolPayload=[=](uint8_t* p_pos,uint8_t* base){
        // [ClientID] --> [Will properties] --> [willTopic -> willPayload -> username -> password]
        p_pos = _applyfront(p_pos); // ClientID
#if MQTT5
        p_pos = _willproperties(p_pos);
#endif
        while(!_blox.empty()) p_pos = _applyfront(p_pos);
#if H4AMC_DEBUG
        mqttTraits T(base,_bs);
#endif
    };
    _build();
}

PublishPacket::PublishPacket(H4AsyncMQTT* p,const char* topic, uint8_t qos, const uint8_t* payload, size_t length, H4AMC_PublishOptions opts_retain):
    _topic(topic),_qos(qos),_retain(opts_retain.getRetained()),_length(length),Packet(p,PUBLISH) {

    if(length < getMaxPayloadSize()){
#if MQTT5
        auto& props = opts_retain.getProperties();
#if H4AMC_ENABLE_CHECKS
        auto& svrOpts = _parent->_serverOptions;
        if (svrOpts->maximum_qos < _qos)
            _qos=_parent->_serverOptions->maximum_qos;
        if (!svrOpts->retain_available && _retain) {
            _notify(H4AMC_SERVER_RETAIN_UNAVAILABLE);
            _retain=false; // Otherwise abort the publish...
        }
#endif
#endif
        _begin=[&, this, topic]{ 

#if MQTT5
            if (!_parent->_isTXAliasAvailable(_topic)) { //Fresh topic OR Server Topic Alias MAX got exceeded.
                _stringblock(_topic);
            } else {
                _stringblock(std::string()); // Empty string.
            }
#else
            _stringblock(_topic);
#endif
            _bs+=_length;
            byte flags=_retain;
            // flags|=(_dup << 3);
        //
            if(_qos) {
                _id=++_parent->_nextId;
                flags|=(_qos << 1);
            }
            _controlcode|=flags;
#if MQTT5
        // [x] Add the properties to the length (property length) + store them
            // [ ] ProposeTXAlias() and save it - on PUBACK/PUBREC confirmTXAlias()
            // [ ] Make assigning topic aliases available only for QoS1 and QoS2 only. 
            if (_parent->_isTXAliasAvailable(topic)) {
                props.topic_alias = _parent->_getTXAlias(topic);
            } else if (_parent->_availableTXAliasSpace()) {
                props.topic_alias = _parent->_assignTXAlias(topic);
            }

            if (props.payload_format_indicator) _propertyLength+=2; // 1B + 1
            if (props.message_expiry_interval)  _propertyLength+=5; // 4B + 1
            if (props.response_topic.length())  _propertyLength+=3+props.response_topic.length(); // 2 for size, 1 PropID
            if (props.correlation_data.size())  _propertyLength+=3+props.correlation_data.size();
            if (props.topic_alias)              _propertyLength+=3; // 2B + 1
            if (props.content_type.length())    _propertyLength+=3+props.content_type.length();
            
            // No subscription ID from Client
            _propertyLength += _fetchUserProperties(props.user_properties);


            _bs += _propertyLength + H4AMC_Helpers::varBytesLength(_propertyLength);
            _properties = [this, &props](uint8_t* p) {
                p = H4AMC_Helpers::encodeVariableByteInteger(p, _propertyLength);
                if (props.payload_format_indicator)
                    p=MQTT_Properties::serializeProperty(PROPERTY_PAYLOAD_FORMAT_INDICATOR, p, props.payload_format_indicator);
                if (props.message_expiry_interval)
                    p=MQTT_Properties::serializeProperty(PROPERTY_MESSAGE_EXPIRY_INTERVAL, p, props.message_expiry_interval);
                if (props.response_topic.length())
                    p=MQTT_Properties::serializeProperty(PROPERTY_RESPONSE_TOPIC, p, props.response_topic);
                if (props.correlation_data.size())
                    p=MQTT_Properties::serializeProperty(PROPERTY_CORRELATION_DATA, p, props.correlation_data);
                if (props.topic_alias)
                    p=MQTT_Properties::serializeProperty(PROPERTY_TOPIC_ALIAS, p, props.topic_alias);
                if (props.content_type.length())
                    p=MQTT_Properties::serializeProperty(PROPERTY_CONTENT_TYPE, p, props.content_type);


                p=_serializeUserProperties(p, props.user_properties);
                return p;
            };
#endif
        };
        _varHeader=[this](uint8_t* p_pos){ // To embed the Topic name.
            p_pos = _applyfront(p_pos);
            if(_id) p_pos=_poke16(p_pos,_id);
            // Properties...
#if MQTT5
            p_pos=_properties(p_pos);
#endif
            return p_pos;
        };

        _protocolPayload=[=](uint8_t* p,uint8_t* base){ 
            memcpy(p,payload,_length);
            mqttTraits T(base,_bs);
            if(_qos) H4AsyncMQTT::_outbound[_id]=T;
        };
        _build();
    } else {
        H4AMC_PRINT1("PUB %d MPL=%d: NO CAN DO\n",length,getMaxPayloadSize());
        _notify(H4AMC_OUTBOUND_PUB_TOO_BIG,length);
    }
}

#if MQTT5
AuthenticationPacket::AuthenticationPacket(H4AsyncMQTT *p, const std::string &method, const H4AMC_BinaryData &data,
                                           AuthOptions& auth) : Packet(p, AUTH)
{

    _begin = [&]{
        // [x] Calculate size
        // [x] Define _properties
        if (!method.length()){
            H4AMC_PRINT1("Invalid Method!\n");
            _notify(H4AMC_INVALID_AUTH_METHOD);
            return;
        }
        _bs+=1;//reason

        _propertyLength += method.length() + 3;
        _propertyLength += data.size() + 3;
#if AUTHPACKET_REASONSTR_USERPROPS
        if (auth.reasonStr.length()) _propertyLength += auth.reasonStr.length() + 3;
        _propertyLength += _fetchUserProperties(auth.props);
#else
        _propertyLength += _fetchUserProperties(dummy);
#endif

        _bs += _propertyLength + H4AMC_Helpers::varBytesLength(_propertyLength);

        _properties = [&](uint8_t* p) {
            p = H4AMC_Helpers::encodeVariableByteInteger(p, _propertyLength);
            p = MQTT_Properties::serializeProperty(PROPERTY_AUTHENTICATION_METHOD, p, method);
            p = MQTT_Properties::serializeProperty(PROPERTY_AUTHENTICATION_DATA, p, data);
#if AUTHPACKET_REASONSTR_USERPROPS
            p = _serializeUserProperties(p, auth.props);
            if (auth.reasonStr.length()) 
                p = MQTT_Properties::serializeProperty(PROPERTY_REASON_STRING, p, auth.reasonStr);
#else
            p = _serializeUserProperties(p, dummy);
#endif
            return p;
        };
    };
    _varHeader = [&](uint8_t* p) {
        p = H4AMC_Helpers::poke8(p, auth.code);
        p = _properties(p);
        return p;
    };
#if H4AMC_DEBUG
    _protocolPayload = [&](uint8_t* p, uint8_t* base) { // No need, it holds no payload.
        mqttTraits T(base,_bs);
        return p;
    };
#endif
    _build();
}
#endif