# vim: set noet
LIBS=-lpthread -lboost_system -lboost_thread -lboost_program_options -lglog -lrabbitmq -lleveldb
INC=-I/usr/include/boost -I/usr/include/boost148 -I./includes -I./lib/
CC=g++
ODIR=.
EXECUTABLE=ssehub

override CFLAGS+=-Wall -std=c++11

DEPS = lib/picohttpparser/picohttpparser.h includes/SSEInputSource.h includes/InputSources/amqp/AmqpInputSource.h includes/CacheAdapters/LevelDB.h includes/CacheAdapters/Redis.h includes/CacheAdapters/CacheInterface.h includes/CacheAdapters/Memory.h includes/SSEClient.h includes/SSEClientHandler.h includes/SSEChannel.h includes/HTTPRequest.h includes/HTTPResponse.h includes/SSEServer.h includes/SSEConfig.h includes/SSEEvent.h includes/SSEStatsHandler.h includes/StatsdClient.h
_OBJ = lib/picohttpparser/picohttpparser.o src/SSEInputSource.o src/InputSources/amqp/AmqpInputSource.o src/CacheAdapters/LevelDB.o src/CacheAdapters/Redis.o src/CacheAdapters/Memory.o src/SSEClient.o src/SSEClientHandler.o src/SSEChannel.o src/HTTPRequest.o src/HTTPResponse.o src/SSEServer.o src/SSEConfig.o src/SSEEvent.o src/SSEStatsHandler.o src/StatsdClient.o src/main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC)

$(EXECUTABLE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

docker: clean
	docker build -t ssehub .

clean:
	rm -f $(OBJ) $(EXECUTABLE)
