/*
MIT License

Copyright (c) 2026 H4Group

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Contact Email: TBD
*/
#define H4AMC_VERSION "1.0.0-rc12"
#define H4AMC_ERROR_BASE 100
/*
    Debug levels: 
    0 - No debug messages
    1 - connection / disconnection + paacket names TX/RX messages
    2 - level 1 + MQTT packet types
    3 - level 2 + MQTT packet info (excluding payload)
    4 - everything including full payload hex dump (and deep diagnostics!)
*/

#define H4AMC_DEBUG 0

// #define MQTT_VERSION       0x04 // MQTT v3.1.1
#define MQTT_VERSION       0x05 // MQTT v5.0

#if MQTT_VERSION >= 0x05
#define MQTT5 1

#define MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT           1		// Comment or Set to zero if desired, this will reduce binary size and enhance performance a bit.
#define MQTT5_RX_TOPIC_ALIAS_MAXIMUM                    50 		// Receive Topic Alias Maximum 
#define MQTT5_TX_TOPIC_ALIAS_MAXIMUM                    50		// Transmit Topic Alias Maximum, to limit the server's Topic Alias Maximum if received a higher value.

#define MQTT5_RX_MAX_PACKET_SIZE                        5200	// RX Max Packet Size limit
#define MQTT5_RECEIVE_MAXIMUM                           25		// Receive Maximum limit
#define MQTT5_SESSION_EXPIRY_INTERVAL                   3600	// MQTT Session Expiry In seconds.

#define H4AMC_MQTT5_INSERT_TOPIC_BY_ALIAS               1       // For retransmission of unacked publishes wherein publishes uses topic aliasa, if set to 0 the re-publish is discarded.

// CONNECT Properties / Options
#define MQTT_CONNECT_REQUEST_RESPONSE_INFORMATION       1       // Do request the response information from the server on CONNECT
#define MQTT_CONNECT_REQUEST_PROBLEM_INFORMATION        1       // Do request the problem information from the server on CONNECT

// SUBSCRIBE Default Options
#define MQTT5_SUBSCRIPTION_OPTION_NO_LOCAL               0 // Default behaviour for MQTT v3.3, and it's protocol error to set it to 1 for Shared Subscriptions in MQTT v5.0
#define MQTT5_SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED    1
#define MQTT5_SUBSCRIPTION_OPTION_RETAIN_HANDLING        1


// DONT CHANGE
#define MQTT_CONNECT_MAX_PACKET_SIZE            MQTT5_RX_MAX_PACKET_SIZE
#define MQTT_CONNECT_RECEIVE_MAXIMUM            MQTT5_RECEIVE_MAXIMUM
#define MQTT_CONNECT_TOPIC_ALIAS_MAX            MQTT5_RX_TOPIC_ALIAS_MAXIMUM
#define MQTT_CONNECT_SESSION_EXPRITY_INTERVAL   MQTT5_SESSION_EXPIRY_INTERVAL

#else
#define MQTT5 0
#define MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT           0
#endif

#define H4AMC_HEADROOM        2000
#define KEEP_ALIVE_INTERVAL   (H4AS_SCAVENGE_FREQ - H4AMC_HEADROOM)

#define H4AMC_MAX_RETRIES        2 // No need to specify this, as it should just retry publishes at successful CONNACK only???
#define H4AMC_ENABLE_CHECKS      1