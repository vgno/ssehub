# vim: set noet
LIBS=-lpthread
CC=g++
ODIR=.
EXECUTABLE=ssehub

override CFLAGS+=-Wall

DEPS = client.h channel.h server.h
_OBJ = client.o channel.o server.o main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXECUTABLE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJ) $(EXECUTABLE)



