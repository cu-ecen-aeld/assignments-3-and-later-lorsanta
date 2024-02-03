C_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c, %.o, $(wildcard *.c))
ifeq "$(CROSS_COMPILATION)" "aarch64-none-linux-gnu-"
	GCC = $(CROSS_COMPILATION)gcc
else
	GCC = gcc
endif

writer: $(OBJ_FILES)
	$(GCC) $< -o $@

$(OBJ_FILES) : $(C_FILES)
	$(GCC) $< -c -o $@

clean:
	rm $(OBJ_FILES) writer