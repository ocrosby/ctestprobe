CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g
CPPFLAGS := -Iinclude
AR       ?= ar

BUILD    := build
SRC      := src/ctestprobe.c
OBJ      := $(BUILD)/ctestprobe.o
LIB      := $(BUILD)/libctestprobe.a

SELF_TEST_SRC := tests/test_ctestprobe.c
SELF_TEST_BIN := $(BUILD)/test_ctestprobe

EXAMPLE_SRC := examples/example1.c
EXAMPLE_BIN := $(BUILD)/example1

.PHONY: all lib test example clean

all: $(LIB) $(SELF_TEST_BIN) $(EXAMPLE_BIN)

lib: $(LIB)

test: $(SELF_TEST_BIN)
	./$(SELF_TEST_BIN)

example: $(EXAMPLE_BIN)
	./$(EXAMPLE_BIN)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(OBJ): $(SRC) include/ctestprobe.h | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(SELF_TEST_BIN): $(SELF_TEST_SRC) $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB)

$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) ctestprobe.o libctestprobe.a
