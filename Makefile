# vim: set noet
LIBS=-lpthread -lglog -lrabbitmq
INC=-I/usr/include/boost148
CC=g++
ODIR=.
EXECUTABLE=ssehub

override CFLAGS+=-Wall

DEPS = lib/cJSON/cJSON.h lib/picohttpparser/picohttpparser.h SSEClient.h SSEClientHandler.h SSEChannel.h HTTPRequest.h SSEServer.h SSEConfig.h AMQPConsumer.h SSEEvent.h
_OBJ = lib/cJSON/cJSON.o lib/picohttpparser/picohttpparser.o SSEClient.o SSEClientHandler.o SSEChannel.o HTTPRequest.o SSEServer.o SSEConfig.o AMQPConsumer.o SSEEvent.o main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC)

$(EXECUTABLE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJ) $(EXECUTABLE)



