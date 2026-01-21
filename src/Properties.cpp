/* 
MIT License

Copyright (c) 2026 H4Group

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Contact Email: TBD
*/

#include "h4amc_common.h"
#if MQTT5
#include <Properties.h>
#include <H4AsyncMQTT.h>
void MQTT_Property_Numeric::print()
{
	if (is_malformed())
		H4AMC_PRINT2("MALFORMED PROPERTY id=%u v=%u\n", id, value);
	else
		H4AMC_PRINT2("\tProperty [%s] [%u]\n", mqttTraits::propnames[id], value);
}

uint8_t* MQTT_Property_Numeric::parse(uint8_t* data){
	value=(*data++)<<24;
	value+=(*data++)<<16;
	value+=(*data++)<<8;
	value+=(*data++);
	return data;
}
uint8_t* MQTT_Property_Numeric::serialize(uint8_t* data, uint32_t value){
	MQTT_Property_Numeric::value = value;
	return serialize(data);
}
uint8_t* MQTT_Property_Numeric::serialize(uint8_t* data){
	*data++=id;
	*data++ = (value & 0xff000000) >> 24;
	*data++ = (value & 0xff0000) >> 16;
	*data++ = (value & 0xff00) >> 8;
	*data++ = value & 0xff;
	return data;
}

uint8_t* MQTT_Property_Numeric_2B::parse (uint8_t* data){
	// value = (*(data + 1)) | (*data << 8);
	value=(*data++)<<8;
	value+=(*data++);
	return data;
}

uint8_t* MQTT_Property_Numeric_2B::serialize(uint8_t* data, uint32_t value){
	MQTT_Property_Numeric::value = value;
	return serialize(data);
}
uint8_t* MQTT_Property_Numeric_2B::serialize (uint8_t* data){
	*data++=id;

	*data++ = (value & 0xff00) >> 8;
	*data++ = value & 0xff;
	return data;
}

uint8_t* MQTT_Property_Numeric_1B::parse (uint8_t* data){
	value=*data++;
	return data;
}
uint8_t* MQTT_Property_Numeric_1B::serialize(uint8_t* data, uint32_t value){
	MQTT_Property_Numeric::value = value;
	return serialize(data);
}
uint8_t* MQTT_Property_Numeric_1B::serialize (uint8_t* data){
	*data++=id;
	*data++ = value & 0xff;
	return data;
}



void MQTT_Property_Bool::print()
{
	if (is_malformed())
		H4AMC_PRINT2("MALFORMED PROPERTY id=%u v=%u\n", id, value);
	else
		H4AMC_PRINT2("\tProperty [%s] [%s]\n", mqttTraits::propnames[id], value ? "true" : "false");
}

uint8_t* MQTT_Property_Numeric_VBI::parse (uint8_t* data){
	uint32_t multiplier = 1;
	uint8_t encodedByte;//,rl=0;
	uint8_t* pp=&data[0];
	do{
		encodedByte = *pp++;
		value += (encodedByte & 0x7f) * multiplier;
		multiplier <<= 7;//** multiplier *= 128;
		length++;
		if (multiplier > 128*128*128) //** if (offset>3)
		{
			H4AMC_PRINT1("Malformed Packet!!, value=%u\n",value);
			malformed_packet = true;
			return data;
		}
	} while ((encodedByte & 0x80) != 0);
	return pp;
}

uint8_t* MQTT_Property_Numeric_VBI::serialize(uint8_t* data, uint32_t value){
	if (value > 268435455UL) {
		H4AMC_PRINT1("Malformed Packet!!, value=%u\n",value);
		malformed_packet = true;
		return data;
	}
	this->value = value;
	return serialize(data);
}
uint8_t* MQTT_Property_Numeric_VBI::serialize(uint8_t* data){
	*data++=id;
	uint8_t encodedByte;
	do {
		encodedByte = value % 128;
		value /= 128;
		if (value > 0) {
			encodedByte |= 0x80;
		}
		*data++ = encodedByte;
	} while (value > 0);
	return data;
}

uint8_t* MQTT_Property_Binary::parse (uint8_t* data){
	value = H4AMC_Helpers::decodeBinary(&data);
	return data;
}
uint8_t* MQTT_Property_Binary::serialize(uint8_t* data, H4AMC_BinaryData value){
	this->value = value;
	return serialize(data);
}
uint8_t* MQTT_Property_Binary::serialize(uint8_t* data){
	*data++=id;
	return H4AMC_Helpers::encodeBinary(data,value);
}
void MQTT_Property_Binary::print(){
#if H4AMC_DEBUG
	H4AMC_PRINT2("\tProperty [%s] DUMP\n", mqttTraits::propnames[id]);
	dumphex(value.data(),value.size());
#endif
}

uint8_t* MQTT_Property_String::parse (uint8_t* data){
	value=H4AMC_Helpers::decodestring(&data);
	return data;
}
uint8_t* MQTT_Property_String::serialize(uint8_t* data, std::string value){
	this->value = value;
	return serialize(data);
}
uint8_t* MQTT_Property_String::serialize(uint8_t* data){
	*data++	= id;
	return H4AMC_Helpers::encodestring(data,value);
}
void MQTT_Property_String::print() { H4AMC_PRINT2("\tProperty [%s] [%s]\n", mqttTraits::propnames[id], value.c_str()); }

uint8_t* MQTT_Property_StringPair::parse (uint8_t* data){
	value.first = H4AMC_Helpers::decodestring(&data);
	value.second = H4AMC_Helpers::decodestring(&data);
	return data;
}
uint8_t* MQTT_Property_StringPair::serialize(uint8_t* data, MQTT_PROP_STRPAIR value){
	this->value = value;
	return serialize(data);
}
uint8_t* MQTT_Property_StringPair::serialize(uint8_t* data){
	*data++ = id;
	data=H4AMC_Helpers::encodestring(data,value.first);
	data=H4AMC_Helpers::encodestring(data,value.second);
	return data;
}

void MQTT_Property_StringPair::print() { H4AMC_PRINT2("\tProperty [%s] [%s=%s]\n", mqttTraits::propnames[id], value.first.c_str(), value.second.c_str()); }


bool MQTT_Properties::isAvailable(H4AMC_MQTT5_Property p) {
	if (p == PROPERTY_USER_PROPERTY)
		return user_properties.size();
	return std::find(available_properties.begin(), available_properties.end(), p) != available_properties.end();
}
uint8_t* MQTT_Properties::serializeProperty(H4AMC_MQTT5_Property p, uint8_t *data, uint32_t value)
{
	switch(p){
		//
		// BOOL
		//
		case PROPERTY_REQUEST_RESPONSE_INFORMATION:
		case PROPERTY_REQUEST_PROBLEM_INFORMATION:
		case PROPERTY_RETAIN_AVAILABLE:
		case PROPERTY_WILDCARD_SUBSCRIPTION_AVAILABLE:
		case PROPERTY_SUBSCRIPTION_IDENTIFIER_AVAILABLE:
		case PROPERTY_SHARED_SUBSCRIPTION_AVAILABLE:
		case PROPERTY_PAYLOAD_FORMAT_INDICATOR:
		{
		MQTT_Property_Bool prop(p);
		return prop.serialize(data, value);
		}
		//
		// 1 BYTE INT
		//
		case PROPERTY_MAXIMUM_QOS:
		{
			MQTT_Property_Numeric_1B prop(p);
			return prop.serialize(data, value);
		}
		//
		// 2 BYTE INT
		//
		case PROPERTY_SERVER_KEEP_ALIVE:
		case PROPERTY_RECEIVE_MAXIMUM:
		case PROPERTY_TOPIC_ALIAS_MAXIMUM:
		case PROPERTY_TOPIC_ALIAS:
		{
			MQTT_Property_Numeric_2B prop(p);
			return prop.serialize(data, value);
		}
		// 
		// VAR BYTE INT
		//
		case PROPERTY_SUBSCRIPTION_IDENTIFIER:
		{
			MQTT_Property_Numeric_VBI prop(p);
			return prop.serialize(data, value);
		}

		//
		// 4 BYTE 
		//
		case PROPERTY_SESSION_EXPIRY_INTERVAL:
		case PROPERTY_MESSAGE_EXPIRY_INTERVAL:
		case PROPERTY_WILL_DELAY_INTERVAL:
		case PROPERTY_MAXIMUM_PACKET_SIZE:
		{
			MQTT_Property_Numeric prop(p);
			return prop.serialize(data, value);
		}
		break;
		default:
		H4AMC_PRINT1("Invalid numeric property: %02X",p);
		break;
	}
	return data;
}
uint8_t* MQTT_Properties::serializeProperty(H4AMC_MQTT5_Property p, uint8_t *data, std::string value)
{
	switch (p)
	{
	case PROPERTY_CONTENT_TYPE:
	case PROPERTY_RESPONSE_TOPIC:
	case PROPERTY_ASSIGNED_CLIENT_IDENTIFIER:
	case PROPERTY_AUTHENTICATION_METHOD:
	case PROPERTY_RESPONSE_INFORMATION:
	case PROPERTY_SERVER_REFERENCE:
	case PROPERTY_REASON_STRING:
	{
		MQTT_Property_String prop(p);
		return prop.serialize(data, value);
	}
	default:
		H4AMC_PRINT1("Invalid string property: %02X", p);
		break;
	}
	return data;
}
uint8_t* MQTT_Properties::serializeProperty(H4AMC_MQTT5_Property p, uint8_t* data, H4AMC_BinaryData value)
{
	switch (p)
	{
	case PROPERTY_CORRELATION_DATA:
	case PROPERTY_AUTHENTICATION_DATA:
	{
		MQTT_Property_Binary prop(p);
		return prop.serialize(data, value);
	}
	default:
		H4AMC_PRINT1("Invalid binary property: %02X", p);
		break;
	}
	return data;
}
uint8_t* MQTT_Properties::serializeUserProperty(uint8_t *data, MQTT_PROP_STRPAIR& value)
{
	MQTT_Property_StringPair prop;
	return prop.serialize(data, value);
}

uint8_t *MQTT_Properties::serializeUserProperties(uint8_t *data, H4AMC_USER_PROPERTIES &map)
{
	for (auto& item : map) {
		MQTT_PROP_STRPAIR pair{item.first,item.second};
		data=serializeUserProperty(data, pair);
	}
	return data;
}

MQTT_PROP_PARSERET MQTT_Properties::parseProperties(uint8_t* data) {
	uint32_t props_length = H4AMC_Helpers::decodeVariableByteInteger(&data);

	while (props_length) {
		auto prop_id = static_cast<H4AMC_MQTT5_Property>(*data++);
		props_length--;
		H4AMC_PRINT4("Parse PROP %02X len %d\n", prop_id, props_length);
		available_properties.push_back(prop_id);
		switch (prop_id)
		{
		//
		// UTF-8 STRING
		//
		case PROPERTY_CONTENT_TYPE:
		case PROPERTY_RESPONSE_TOPIC:
		case PROPERTY_ASSIGNED_CLIENT_IDENTIFIER:
		case PROPERTY_AUTHENTICATION_METHOD:
		case PROPERTY_RESPONSE_INFORMATION:
		case PROPERTY_SERVER_REFERENCE:
		case PROPERTY_REASON_STRING:
		{
			H4AMC_PRINT4("PARSE STRING PROP\n");
			MQTT_Property_String prop(prop_id);
			data = prop.parse(data);
			if (prop.is_malformed()) return {H4AMC_MQTT_ReasonCode::REASON_MALFORMED_PACKET, data};
			props_length-=(2 + prop.value.length());
			string_props.push_back(prop);
			break;
		}
		// 
		// BINARY DATA
		//
		case PROPERTY_CORRELATION_DATA:
		case PROPERTY_AUTHENTICATION_DATA:
		{
			H4AMC_PRINT4("PARSE BINARY PROP\n");
			MQTT_Property_Binary prop(prop_id);
			data = prop.parse(data);
			if (prop.is_malformed()) return {H4AMC_MQTT_ReasonCode::REASON_MALFORMED_PACKET, data};
			props_length-=(2 + prop.value.size());

			binary_props.push_back(prop);
			break;
		}
		case PROPERTY_USER_PROPERTY:
		{
			H4AMC_PRINT4("PARSE USER PROP\n");
			MQTT_Property_StringPair prop;
			data = prop.parse(data);
			if (prop.is_malformed()) return {H4AMC_MQTT_ReasonCode::REASON_MALFORMED_PACKET, data};
			props_length-=(4 + prop.value.first.length() + prop.value.second.length());
			user_properties[prop.value.first] = prop.value.second;
			available_properties.pop_back(); // Separate API for this.
			break;
		}
		//
		// 1B BOOL
		//
		case PROPERTY_REQUEST_PROBLEM_INFORMATION:
		case PROPERTY_REQUEST_RESPONSE_INFORMATION:
		case PROPERTY_RETAIN_AVAILABLE:
		case PROPERTY_WILDCARD_SUBSCRIPTION_AVAILABLE:
		case PROPERTY_SUBSCRIPTION_IDENTIFIER_AVAILABLE:
		case PROPERTY_SHARED_SUBSCRIPTION_AVAILABLE:
		case PROPERTY_PAYLOAD_FORMAT_INDICATOR:
		{
			H4AMC_PRINT4("PARSE BOOL PROP\n");
			MQTT_Property_Bool prop(prop_id);
			data = prop.parse(data);
			if (prop.is_malformed()) return {H4AMC_MQTT_ReasonCode::REASON_MALFORMED_PACKET, data};
			props_length--;
			numeric_props.push_back(prop);
			break;
		}
		//
		// 1B
		//
		case PROPERTY_MAXIMUM_QOS:
		{
			H4AMC_PRINT4("PARSE 1B PROP\n");
			MQTT_Property_Numeric_1B prop(prop_id);
			data = prop.parse(data);
			if (prop.is_malformed()) return {H4AMC_MQTT_ReasonCode::REASON_MALFORMED_PACKET, data};
			props_length--;
			numeric_props.push_back(prop);
			break;
		}
		//
		// 2B
		//
		case PROPERTY_SERVER_KEEP_ALIVE:
		case PROPERTY_RECEIVE_MAXIMUM:
		case PROPERTY_TOPIC_ALIAS_MAXIMUM:
		case PROPERTY_TOPIC_ALIAS:
		{
			H4AMC_PRINT4("PARSE 2B PROP\n");
			MQTT_Property_Numeric_2B prop(prop_id);
			data = prop.parse(data);
			if (prop.is_malformed()) return {H4AMC_MQTT_ReasonCode::REASON_MALFORMED_PACKET, data};
			props_length-=2;
			numeric_props.push_back(prop);
			break;
		}
		//
		// VARIABLE BYTE INTEGER
		//
		case PROPERTY_SUBSCRIPTION_IDENTIFIER:
		{
			H4AMC_PRINT4("PARSE VBI PROP\n");
			MQTT_Property_Numeric_VBI prop(prop_id);
			data = prop.parse(data);
			if (prop.is_malformed()) return {H4AMC_MQTT_ReasonCode::REASON_MALFORMED_PACKET, data};
			props_length-=prop.length;
			numeric_props.push_back(prop);
			break;
		}
		//
		// 4B
		//
		case PROPERTY_SESSION_EXPIRY_INTERVAL:
		case PROPERTY_MESSAGE_EXPIRY_INTERVAL:
		case PROPERTY_WILL_DELAY_INTERVAL:
		case PROPERTY_MAXIMUM_PACKET_SIZE:
		{
			H4AMC_PRINT4("PARSE 4B PROP\n");
			MQTT_Property_Numeric prop(prop_id);
			data = prop.parse(data);
			props_length-=4;
			numeric_props.push_back(prop);
			break;
		}
		default:
			H4AMC_PRINT1("Invalid property: %02X\n", prop_id);
			available_properties.pop_back();
			return std::make_pair(REASON_PROTOCOL_ERROR,data);
		}
	}
#if H4AMC_DEBUG
	dump();
#endif
	return std::make_pair(REASON_SUCCESS,data);
}

std::string MQTT_Properties::getStringProperty(H4AMC_MQTT5_Property p) {
	auto it = std::find_if(string_props.begin(),string_props.end(), [p](const MQTT_Property_String& sp){ return sp.id == p; });
	if (it == string_props.end())
		return "";
	return it->value;
}

uint32_t MQTT_Properties::getNumericProperty(H4AMC_MQTT5_Property p) { 
	auto it = std::find_if(numeric_props.begin(),numeric_props.end(), [p](const MQTT_Property_Numeric& np){ return np.id == p; });
	if (it == numeric_props.end())
		return 0;
	return it->is_malformed() ? 0 : it->value;
}

std::vector<uint32_t> MQTT_Properties::getNumericProperties(H4AMC_MQTT5_Property p)
{
	std::vector<uint32_t> values;
	for (auto& np : numeric_props) {
		if (np.id==p)
			values.push_back(np.value);
	}
	return values;
}

H4AMC_BinaryData MQTT_Properties::getBinaryProperty(H4AMC_MQTT5_Property p)
{
	auto it = std::find_if(binary_props.begin(),binary_props.end(), [p](const MQTT_Property_Binary& bp){ return bp.id == p; });
	if (it != binary_props.end())
		return it->value;
	return H4AMC_BinaryData();
}

#endif // MQTT5