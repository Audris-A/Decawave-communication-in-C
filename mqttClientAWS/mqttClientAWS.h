#include "base64.h"

#define CLIENTID_SUB "ClientSub"
#define TOPIC        "dwm/node/+/+/+"
#define QOS          0
#define TIMEOUT      10000L

#define LOCATION_COLLECTION "TagLocations"
#define CONFIG_COLLECTION   "config"
#define DISTANCE_COLLECTION "distance"
#define STATUS_COLLECTION   "status"

volatile MQTTClient_deliveryToken deliveredtoken;

MQTTClient_deliveryToken token;

MQTTClient client_sub;

mongoc_database_t *database;
mongoc_client_t *client;
mongoc_collection_t *collection;
