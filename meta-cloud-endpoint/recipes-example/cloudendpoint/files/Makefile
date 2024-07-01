#CC = gcc -g
INCLUDES = -I .
CFLAGS = -Wall -g
LIBS = -lcurl

TARGET = cloud

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) ${INCLUDES} ${LDFLAGS} -o $@ $< $(LIBS)

clean:
	rm -f $(TARGET)