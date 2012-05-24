CC = gcc

CFLAGS = -c -g -Wall

LDFLAGS =

LIBS    = -lpthread -lssl

SOURCES = init.c \
          session.c \
	  transport_l.c \
	  timer.c 

OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE = sender



$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@


all: $(SOURCES) $(EXECUTABLE)

install: all
	./sender
clean: 
	rm -rf sender *.o

