C_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c, %.o, $(wildcard *.c))
ifeq "$(CROSS_COMPILE)" "aarch64-none-linux-gnu-"
	GCC = $(CROSS_COMPILE)gcc
else
	GCC = gcc
endif

all: writer

writer: $(OBJ_FILES)
	$(GCC) $< -o $@

$(OBJ_FILES) : $(C_FILES)
	$(GCC) $< -c -o $@

clean:
	rm -f $(OBJ_FILES) writer
