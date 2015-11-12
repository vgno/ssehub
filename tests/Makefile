# vim: set noet
LIBS=-lpthread -lboost_system -lboost_thread -lboost_program_options -lglog -lrabbitmq -lleveldb -lboost_unit_test_framework
INC=-I/usr/include/boost -I/usr/include/boost148 -I./../includes -I./../lib/ -I./../src/
CC=g++
ODIR=.
EXECUTABLE=tests

override CFLAGS+=-Wall

_OBJ = SSEEventTest.cpp SSEChannelTest.cpp
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS) $(INC)

$(EXECUTABLE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
