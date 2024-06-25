#CC = gcc -g
CFLAGS = -Wall -g

TARGET = bleproxy

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) ${LDFLAGS} -o $@ $<

clean:
	rm -f $(TARGET)

