/*
MIT License

Copyright (c) 2026 H4Group

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Contact Email: TBD
*/
#include <H4AsyncMQTT.h>

H4 h4(115200);

H4AsyncMQTT mqttClient;

//H4_TIMER PT1;
//
// if using TLS / https, edit h4amcconfig.h and #define ASYNC_TCP_SSL_ENABLED 1
// do the same in async_config.h of the PATCHED ESPAsyncTCP library!! 

//#define MQTT_URL "https://192.168.1.21:8883"
#define MQTT_URL "192.168.1.20:1883"
const uint8_t* cert=nullptr;

// if you provide a valid certificate when connecting, it will be checked and fail on no match
// if you do not provide one, H4AsyncMQTT will continue insecurely with a warning
// this one is MY local mosquitto server... it ain't gonna work, so either don't use one, or set your own!!!
//const uint8_t cert[20] = { 0x9a, 0xf1, 0x39, 0x79,0x95,0x26,0x78,0x61,0xad,0x1d,0xb1,0xa5,0x97,0xba,0x65,0x8c,0x20,0x5a,0x9c,0xfa };
//#define MQTT_URL "https://robot.local:8883"

// If using MQTT server authentication, fill in next two fields!
const char* mqAuth="example";
const char* mqPass="pangolin";
//
//  Some sketches will require you to set START_WITH_CLEAN_SESSION to false
//  For THIS sketch, leave it at false
//
#define START_WITH_CLEAN_SESSION   true

#define BIG_SIZE 5000

const char* pload0="multi-line payload hex dumper which should split over several lines, with some left over";
const char* pload1="PAYLOAD QOS1";
const char* pload2="Save the Pangolin!";
char big[BIG_SIZE];

void onMqttError(int e,int i){
  if(e < H4AMC_ERROR_BASE){
    Serial.printf("H4ASYNC ERROR %d [%s] info=%d[%p]\n",e,H4AsyncClient::errorstring(e).data(),i,i);
  }
  else {
    if(e < H4AMC_ERROR_MAX){
       Serial.printf("H4AsyncMQTT ERROR %d [%s] info=%d[%p]\n",e,H4AsyncMQTT::errorstring(e).data(),i,i);   
    }
    else Serial.printf("UNKNOWN ERROR: %u extra info %d[%p]\n",e,i,i);
  }
}
H4_TIMER sender;

void onMqttConnect(H4AMC_ConnackParam params) {
  Serial.printf("USER: Connected as %s MP=%d\n",mqttClient.getClientId().data(),getMaxPayloadSize());

  Serial.println("USER: Subscribing at QoS 2");
  if (!params.session) {
    mqttClient.subscribe({"test","multi2","fully/compliant"}, 2);
    mqttClient.unsubscribe({"multi2","fully/compliant"});
  }

  sender=h4.every(30000,[]{
    Serial.printf("T=%u Publish:\n",millis());
    //simple way to publish int types  as strings using printf format
    mqttClient.publish("test",_HAL_freeHeap(),"%u"); 
//    mqttClient.publish("test",-33);
//    mqttClient.publish("test",big,BIG_SIZE,0);
//    mqttClient.publish("test",pload0,strlen(pload0),0); 
//    mqttClient.publish("test",pload1,strlen(pload1),1); 
//    mqttClient.publish("test",pload2,strlen(pload2),2);    
  });
}

void onMqttMessage(const char* topic, const uint8_t* payload, size_t len, H4AMC_MessageOptions opts) {
	Serial.printf("Receive: H=%u Message %s qos%d dup=%d retain=%d len=%d\n",_HAL_freeHeap(),topic,opts.qos,opts.dup,opts.retain,len);
  //dumphex(payload,len);
  //Serial.println();
}

void onMqttDisconnect() { Serial.printf("USER: Disconnected from MQTT\n"); h4.cancel(sender); }

void h4setup(){
  for(auto i=0;i<BIG_SIZE;i++) big[i]=i;
  Serial.printf("\nH4AsyncMQTT v%s running @ debug level %d heap=%u\n",H4AMC_VERSION,H4AMC_DEBUG,_HAL_freeHeap()); 

  mqttClient.onError(onMqttError);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
//  mqttClient.setServer(MQTT_URL,mqAuth,mqPass,cert);
  mqttClient.setWill("DIED",2,"probably still some bugs",false);
//  mqttClient.setKeepAlive(RECONNECT_DELAY_M *3); // very rarely need to change this (if ever)
  WiFi.begin("XXXXXXXX","XXXXXXXX");
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(1000);
  }
  
  Serial.printf("WIFI CONNECTED IP=%s\n",WiFi.localIP().toString().c_str());

  mqttClient.connect(MQTT_URL,mqAuth,mqPass);
}