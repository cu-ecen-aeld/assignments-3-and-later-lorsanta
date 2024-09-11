C_FILES = $(wildcard *.c)
CFLAGS ?= -g -Wall
LDFLAGS ?= -pthread
OBJ_FILES = $(patsubst %.c, %.o, $(wildcard *.c))
CC ?= $(CROSS_COMPILE)gcc
USE_AESD_CHAR_DEVICE ?= 1

TARGET ?= aesdsocket

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(OBJ_FILES) : $(C_FILES)
	$(CC) -DUSE_AESD_CHAR_DEVICE $(CFLAGS) $< -c -o $@

clean:
	rm -f $(OBJ_FILES) $(TARGET)
