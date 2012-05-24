CC = gcc

CFLAGS = -c -g -w -Wall

LDFLAGS =

LIBS    = 

SOURCES = cems.c 

OBJECTS=$(SOURCES:.c=.o)


EXECUTABLE =BB_watchdog 


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@


all: $(SOURCES) $(EXECUTABLE)

install: all
	./$(EXECUTABLE)
clean: 
	rm -rf $(EXECUTABLE) *.o

