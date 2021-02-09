# Sample Makefile

TARGET := mqttClientAWS
#TARGET1 := mjson
TARGET2 := base64

OBJ := $(TARGET).o $(TARGET2).o

CFLAGS = -L/usr/local/lib -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -lmongoc-1.0 -lbson-1.0 -L~/scripts/mqtt-C -lpaho-mqtt3c # -mcpu=xscale -O0 -Wall

CC := gcc

DEPS := mqttClientAWS.h base64.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


mqttClientAWS: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
