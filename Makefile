CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -g
DEPS = # Add your header files here
OBJ = ctestprobe.o
TARGET = libctestprobe.a

all: $(TARGET)

%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(AR) rcs $@ $^
	ls -la $(TARGET)

.PHONY: clean

clean:
	rm -f $(OBJ) $(TARGET)