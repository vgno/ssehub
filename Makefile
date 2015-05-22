# vim: set noet
LIBS=-lpthread -lglog -lrabbitmq
CC=g++
ODIR=.
EXECUTABLE=ssehub

override CFLAGS+=-Wall
override CFLAGS+=-std=c++11

DEPS = lib/cJSON/cJSON.h SSEClient.h SSEChannel.h SSEServer.h SSEConfig.h AMQPConsumer.h
_OBJ = lib/cJSON/cJSON.o SSEClient.o SSEChannel.o SSEServer.o SSEConfig.o AMQPConsumer.o main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXECUTABLE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJ) $(EXECUTABLE)



