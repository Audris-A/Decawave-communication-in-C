#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <time.h>
#include <ctype.h>
#include "mqttClientRBPi.h"

int main(int argc, char* argv[])
{
    int rc_pub;
    char *filePath = argv[1];
    
    if (!argv[1])
    {
        printf("Nav padots ceļš līdz failam!\n");
        return 1;
    } 
    
    /***************** MQTT PUBLISH INIT ****************/
    MQTTClient_connectOptions conn_opts_publisher = MQTTClient_connectOptions_initializer;
    
    MQTTClient_create(&client_publisher, ADDRESS_PUB, CLIENTID_PUB,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
    conn_opts_publisher.keepAliveInterval = 20;
    conn_opts_publisher.cleansession = 1;
    
    MQTTClient_setCallbacks(client_publisher, NULL, NULL, NULL, NULL);
    
    // šo ielikt connLost funkcijā?
    if ((rc_pub = MQTTClient_connect(client_publisher, &conn_opts_publisher)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc_pub);
        exit(EXIT_FAILURE);
    }
    

    //Save the message in the backlog
    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    char *str1, *tokenValue; 
    char tagIdForTopic[10];
    char messageType[50];
    char *saveptr1;
    int j;

    char topicNameValue[100];
    
    fp = fopen(filePath, "r"); // think about dynamically asigning the file name accordingly
    
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        char lineValue[200];
	strcpy(lineValue, line);

        for (j = 1, str1 = lineValue; ; j++, str1 = NULL) {
            tokenValue = strtok_r(str1, "^", &saveptr1);
            if (tokenValue == NULL){
       	        break;
	    }

	    switch(j){
		case 3: strcpy(tagIdForTopic, tokenValue);
		case 4: strcpy(messageType, tokenValue);
	    }
	}
	
        sprintf(topicNameValue, "dwm/node/%s/uplink/%s", tagIdForTopic, messageType);
	printf("%s\n", topicNameValue);
        printf("%s\n", line);
    	pubmsg.payload = line;
    	pubmsg.payloadlen = strlen(line);
    	pubmsg.qos = QOS;
    	pubmsg.retained = 0;
    	deliveredtoken = 0;
    	MQTTClient_publishMessage(client_publisher, topicNameValue, &pubmsg, &token);
    }

    fclose(fp);

    MQTTClient_disconnect(client_publisher, 10000);
    MQTTClient_destroy(&client_publisher);

    return rc_pub;
}
