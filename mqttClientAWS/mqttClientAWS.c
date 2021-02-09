/*
 *  README:
 *    TODO: Add logging and possible error identification
 *          Add config update funcionality
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <time.h>
#include <mongoc/mongoc.h>
#include "mqttClientAWS.h"
#include <time.h>

#define ADDRESS_SUB getenv("server")

struct messageContent_t {
    char message[1024];
    char timestamp[30];
    char testCaseName[30];
    char zoneId[30];
    char panId[5];
    char nodeId[5];
    char tagId[5];
    char anchorId[5];
    int  distance;
};

struct messageContent_t messageContent;

struct timeval time_now;

bool insert_data (mongoc_collection_t *collection, bson_t *payload)
{
   bson_error_t error;
   bson_oid_t oid;
   bson_t *doc;
   doc = bson_new();
   doc = payload;

   bson_oid_init(&oid, NULL);

   if (!mongoc_collection_insert_one(
           collection, doc, NULL, NULL, &error)) {
        fprintf(stderr, "%s\n", error.message);
   }
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

void sendDataToDatabase(bson_t *b, char * messageType)
{ 
    BSON_APPEND_UTF8(b, "zoneId", messageContent.zoneId);
    BSON_APPEND_UTF8(b, "panId", messageContent.panId);
    BSON_APPEND_UTF8(b, "testCaseName", messageContent.testCaseName);
    BSON_APPEND_UTF8(b, "timestamp", messageContent.timestamp);
 
    if (!strcmp(messageType, "data"))
    {
        BSON_APPEND_UTF8(b, "tagId", messageContent.nodeId);
        BSON_APPEND_UTF8(b, "anchorId", messageContent.anchorId);
        BSON_APPEND_INT32(b, "distance", messageContent.distance);
    }
    else
    {
        BSON_APPEND_UTF8(b, "nodeId", messageContent.nodeId);
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char dateTimeVal[25];
    
    sprintf(dateTimeVal, "%d.%02d.%02d %02d:%02d:%02d:%06li", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
             tm.tm_hour,
             tm.tm_min,
             tm.tm_sec,
             time_now.tv_usec);

    BSON_APPEND_UTF8(b, "created", dateTimeVal);   
    
    insert_data(collection, b);
};

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    char payloadText[message->payloadlen + 1];

    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;

    for (int i = 0; i < message->payloadlen; i++) 
    {
        payloadText[i] = *payloadptr++;
    }

    payloadText[message->payloadlen] = '\0';

    // Decode message
    char *str1, *str2, *token, *subtoken;
    char *saveptr1, *saveptr2;
    int j;
    
    for (j = 1, str1 = payloadText; ; j++, str1 = NULL) {
        token = strtok_r(str1, "^", &saveptr1);
        if (token == NULL)
       	    break;
        printf("%d: %s\n", j, token);
	
        switch(j){
            case 1: strcpy(messageContent.message, token);
	    case 2: strcpy(messageContent.timestamp, token);
	    case 3: strcpy(messageContent.zoneId, token);
	    case 4: strcpy(messageContent.panId, token);
	    case 5: strcpy(messageContent.testCaseName, token);
	    default: ; //
	}
    }
    
    bson_t *b;
    bson_error_t error;
    const uint8_t *data = messageContent.message;
    b = bson_new_from_json(data, -1, &error);
	
    if (!b)
    {
        printf("ERROR: %d.%d: %s\n", error.domain, error.code, error.message);
	//exit(1);
    }
    
    for (j = 1, str1 = topicName; ; j++, str1 = NULL) {
        token = strtok_r(str1, "/", &saveptr1);
        if (token == NULL)
       	    break;
        printf("%d: %s\n", j, token);
        
        if (j == 3) 
	{
	    strcpy(messageContent.nodeId, token);
    	}
        else if (j == 5)
	{
	    if (!strcmp(token, "location")) 
	    {
		collection = mongoc_database_get_collection (database, LOCATION_COLLECTION);
	    }
	    else if (!strcmp(token, "config")) 
	    {
		collection = mongoc_database_get_collection (database, CONFIG_COLLECTION);
	    }
	    else if (!strcmp(token, "data")) 
	    {
		collection = mongoc_database_get_collection (database, DISTANCE_COLLECTION);
		
		// Insert distance values after the formating
		char * base64String;
		size_t baseOutLen;
		bson_iter_t iter;
                const bson_value_t *value;
		
		if (bson_iter_init(&iter, b)) 
	        {
	            while (bson_iter_next(&iter)) {
			value = bson_iter_value (&iter);

		        if (!strcmp(bson_iter_key(&iter), "data"))
			{
			    base64String = value->value.v_utf8.str;
		            break;
			}	
		    }
		}
		
		size_t baseLen = strlen(base64String);
		
		unsigned char *decodedString = base64_decode((const unsigned char*) base64String, baseLen, baseOutLen);
		    
		int anchorCount = decodedString[0];

		int bytesCounter = 1;

		if (baseOutLen >= 34)
		{   
		    int i;
		    for (i = 0; i < anchorCount; i++)
		    {
		        int anchorHexId = (decodedString[bytesCounter + 0] & 0xff) | 
				          ((decodedString[bytesCounter + 1]& 0xff) & 0x000000FF) << 8;
	     	        char anchorId[5];
			sprintf(anchorId,"%x", anchorHexId);
			u_int16_t distance = (decodedString[bytesCounter + 2]& 0xff) | 
					    ((decodedString[bytesCounter + 3]& 0xff) & 0x000000FF) << 8 | 
					    ((decodedString[bytesCounter + 4]& 0xff) & 0x000000FF) << 16 | 
					    ((decodedString[bytesCounter + 5]& 0xff) & 0x000000FF) << 24;

			bytesCounter = bytesCounter + 4;
			    
			strcpy(messageContent.anchorId, anchorId);
			messageContent.distance = distance;
			    
			bson_t bsonObj;
			bson_init(&bsonObj);	    
			    
			sendDataToDatabase(&bsonObj, token);
			bson_destroy(&bsonObj);
		    }
		}
		
	    }
	    else if (!strcmp(token, "status")) 
	    {
		collection = mongoc_database_get_collection (database, STATUS_COLLECTION);
	    }

	    
	    if (!strcmp(token, "data"))
	    {
	        sendDataToDatabase(b, token);
	    }
	}	
    }
     
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);

    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int ssl_error(const char *str, size_t len, void *u)
{
    printf("%s\n", str);
}

int main()
{
    
    int rc_sub, rc_pub;
    int ch_sub;
    
    mongoc_init();
    
    client = mongoc_client_new(getenv("connectionString"));
    database = mongoc_client_get_database(client, "conTraDB");

    /***************** MQTT SUB INIT****************/
    MQTTClient_connectOptions conn_opts_sub = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    
    rc_sub = MQTTClient_create(&client_sub, ADDRESS_SUB, CLIENTID_SUB,
        MQTTCLIENT_PERSISTENCE_DEFAULT, NULL);
    
    if (!(rc_sub == MQTTCLIENT_SUCCESS))
    {
	return -1;	
    }
    
    conn_opts_sub.username = getenv("username");
    conn_opts_sub.password = getenv("password");
    
    //ssl_opts.ssl_error_cb = ssl_error;
    conn_opts_sub.ssl = &ssl_opts;
    conn_opts_sub.ssl->trustStore = getenv("certLocation");
    conn_opts_sub.keepAliveInterval = 20;
    conn_opts_sub.cleansession = 1;

    MQTTClient_setCallbacks(client_sub, NULL, connlost, msgarrvd, delivered);

    // šo ielikt connLost funkcijā?
    if ((rc_sub = MQTTClient_connect(client_sub, &conn_opts_sub)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc_sub);
	printf("URL = %s", ADDRESS_SUB);
        exit(EXIT_FAILURE);
    }

    /***************** MQTT SUB MAIN ****************/
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID_SUB, QOS);
    MQTTClient_subscribe(client_sub, TOPIC, QOS);
    do
    {
        ch_sub = getchar();
    } while(ch_sub!='Q' && ch_sub != 'q');
    MQTTClient_disconnect(client_sub, 10000);
    MQTTClient_destroy(&client_sub);
    printf("SUB MAIN 1\n");

    mongoc_database_destroy(database);
    mongoc_client_destroy(client);
    mongoc_cleanup();

    printf("mongo 2\n");
    return rc_sub;
}
