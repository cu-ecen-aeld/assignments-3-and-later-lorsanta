C_FILES = $(wildcard *.c)
CFLAGS ?= -g -Wall
LDFLAGS ?= -pthread
OBJ_FILES = $(patsubst %.c, %.o, $(wildcard *.c))
CC ?= $(CROSS_COMPILE)gcc

TARGET ?= aesdsocket

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(OBJ_FILES) : $(C_FILES)
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f $(OBJ_FILES) $(TARGET)
