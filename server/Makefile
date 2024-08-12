C_FILES = $(wildcard *.c)
CFLAGS ?= -g -Wall
LDFLAGS ?= -pthread
OBJ_FILES = $(patsubst %.c, %.o, $(wildcard *.c))
ifeq "$(CROSS_COMPILE)" "aarch64-none-linux-gnu-"
	GCC ?= $(CROSS_COMPILE)gcc
else
	GCC ?= gcc
endif

TARGET ?= aesdsocket

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(GCC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(OBJ_FILES) : $(C_FILES)
	$(GCC) $(CFLAGS) $< -c -o $@

clean:
	rm -f $(OBJ_FILES) $(TARGET)
