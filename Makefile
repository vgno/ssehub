# vim: set noet
LIBS=-lpthread -lglog -lrabbitmq
INC=-I/usr/include/boost -I/usr/include/boost148 -I./includes
CC=g++
ODIR=.
EXECUTABLE=ssehub

override CFLAGS+=-Wall

DEPS = lib/picohttpparser/picohttpparser.h includes/SSEClient.h includes/SSEClientHandler.h includes/SSEChannel.h includes/HTTPRequest.h includes/HTTPResponse.h includes/SSEServer.h includes/SSEConfig.h includes/AMQPConsumer.h includes/SSEEvent.h includes/SSEStatsHandler.h
_OBJ = lib/picohttpparser/picohttpparser.o src/SSEClient.o src/SSEClientHandler.o src/SSEChannel.o src/HTTPRequest.o src/HTTPResponse.o src/SSEServer.o src/SSEConfig.o src/AMQPConsumer.o src/SSEEvent.o src/SSEStatsHandler.o src/main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC)

$(EXECUTABLE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

docker: clean
	docker build -t ssehub .

clean:
	rm -f $(OBJ) $(EXECUTABLE)
