CC = gcc
CFLAGS = -Wall -pthread
TARGET = server
OBJS = main.o server.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c server.h
	$(CC) $(CFLAGS) -c main.c

server.o: server.c server.h
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f $(TARGET) $(OBJS)
