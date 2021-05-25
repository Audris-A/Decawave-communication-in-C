# Sample Makefile

TARGET := mqttClientRBPi

OBJ := $(TARGET).o

# ASFLAGS = -mcpu=xscale -alh=$*.lis -L
CFLAGS = -L/usr/local/lib -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -lmongoc-1.0 -lbson-1.0 -L /home/audris/Downloads/dwm_data -l paho-mqtt3c # -mcpu=xscale -O0 -Wall

CC := gcc

DEPS := mqttClientRBPi.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


mqttClientRBPi: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
