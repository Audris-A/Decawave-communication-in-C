#define ADDRESS_SUB  "127.0.0.1"
#define ADDRESS_PUB  "localhost:1888"
#define CLIENTID_SUB "ClientSend"
#define CLIENTID_PUB "ClientPub"
#define TOPIC        "dwm/node/+/+/+"
#define QOS          0
#define TIMEOUT      10000L

volatile MQTTClient_deliveryToken deliveredtoken;

MQTTClient client_publisher;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;

MQTTClient client_sub;

char *zoneId;
char *panId;
char *testCaseName;
