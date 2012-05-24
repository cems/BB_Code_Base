CC = gcc

CFLAGS = -c -g -w -Wall

LDFLAGS =

LIBS    = 

SOURCES = new_serial.c 

OBJECTS=$(SOURCES:.c=.o)


EXECUTABLE = serial


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@


all: $(SOURCES) $(EXECUTABLE)

install: all
	./$(EXECUTABLE)
clean: 
	rm -rf $(EXECUTABLE) *.o

