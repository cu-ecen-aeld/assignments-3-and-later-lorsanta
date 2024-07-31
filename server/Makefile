C_FILES = $(wildcard *.c)
CFLAGS = -lpthread -g -Wall
OBJ_FILES = $(patsubst %.c, %.o, $(wildcard *.c))
ifeq "$(CROSS_COMPILE)" "aarch64-none-linux-gnu-"
	GCC = $(CROSS_COMPILE)gcc
else
	GCC = gcc
endif

all: aesdsocket

aesdsocket: $(OBJ_FILES)
	$(GCC) $(CFLAGS) $< -o $@

$(OBJ_FILES) : $(C_FILES)
	$(GCC) $(CFLAGS) $< -c -o $@

clean:
	rm -f $(OBJ_FILES) aesdsocket
