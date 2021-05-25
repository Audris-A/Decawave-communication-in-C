#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <time.h>
#include <ctype.h>
#include "mqttClientRBPi.h"

struct timeval time_now;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    gettimeofday(&time_now, NULL);
    int res = 0;

    char dateTimeVal[62 + message->payloadlen];

    char payloadText[message->payloadlen + 1];

    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: \n");

    payloadptr = message->payload;

    for (int i = 0; i < message->payloadlen; i++) 
    {
        payloadText[i] = *payloadptr++;
    }

    payloadText[message->payloadlen] = '\0';

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char *str1, *tokenValue;
    char *saveptr1;
    char tagId[10];
    char messageType[100];
    int j = 0;

    for (j = 1, str1 = topicName; ; j++, str1 = NULL) {
	    tokenValue = strtok_r(str1, "/", &saveptr1);

	    if (tokenValue == NULL)
		    break;
	    
	    printf("%s\n", tokenValue);
	    
	    switch(j){
		    case 3: strcpy(tagId, tokenValue);
		    case 5: strcpy(messageType, tokenValue);
		    default: ; //
	    }
    }

    sprintf(dateTimeVal, "%s^%d.%02d.%02d %02d:%02d:%02d:%06li^%s^%s^%s^%s^%s", payloadText, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
             tm.tm_hour,
             tm.tm_min,
             tm.tm_sec,
             time_now.tv_usec, &tagId, &messageType, zoneId, panId, testCaseName);

    printf("%s\n", dateTimeVal);

    pubmsg.payload = dateTimeVal;
    pubmsg.payloadlen = strlen(dateTimeVal);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    deliveredtoken = 0;

    //Save the message in the backlog
    FILE *fp;
    fp = fopen("backlog.txt", "a+"); // think about dynamically asigning the file name accordingly
    fputs(dateTimeVal, fp);
    fputs("\n", fp);
    fclose(fp);

    //Send the message to the AWS cloud mqtt server
    printf("Topic = %s", topicName);
    MQTTClient_publishMessage(client_publisher, topicName, &pubmsg, &token);
    printf("Waiting for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            pubmsg.payload, topicName, CLIENTID_PUB);

    //res = MQTTClient_waitForCompletion(client_publisher, token, 20000L);
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);

    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
    int rc_sub, rc_pub;
    int ch_sub;
    
    zoneId = argv[1];
    panId = argv[2];
    testCaseName = argv[3];

    if (!argv[1])
    {
        printf("Nav padots zonas identifikators!\n");
        return 1;
    } 
    else if (!argv[2]) 
    {
        printf("Nav padots tīkla identifikators!\n");
        return 1;
    }
    else if (!argv[3])
    {
        printf("Nav padots testa nosaukums!\n");
        return -1;
    }

    /***************** MQTT SUB INIT****************/
    MQTTClient_connectOptions conn_opts_sub = MQTTClient_connectOptions_initializer;

    MQTTClient_create(&client_sub, ADDRESS_SUB, CLIENTID_SUB,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts_sub.keepAliveInterval = 20;
    conn_opts_sub.cleansession = 1;

    MQTTClient_setCallbacks(client_sub, NULL, connlost, msgarrvd, delivered);

    // šo ielikt connLost funkcijā?
    if ((rc_sub = MQTTClient_connect(client_sub, &conn_opts_sub)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc_sub);
        exit(EXIT_FAILURE);
    }

    /***************** MQTT PUBLISH INIT ****************/
    MQTTClient_connectOptions conn_opts_publisher = MQTTClient_connectOptions_initializer;
    
    MQTTClient_create(&client_publisher, ADDRESS_PUB, CLIENTID_PUB,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
    conn_opts_publisher.keepAliveInterval = 20;
    conn_opts_publisher.cleansession = 1;
    
    MQTTClient_setCallbacks(client_publisher, NULL, connlost, msgarrvd, delivered);
    
    // šo ielikt connLost funkcijā?
    if ((rc_pub = MQTTClient_connect(client_publisher, &conn_opts_publisher)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc_pub);
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

    MQTTClient_disconnect(client_publisher, 10000);
    MQTTClient_destroy(&client_publisher);

    return rc_sub;
}
