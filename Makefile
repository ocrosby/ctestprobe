CC = gcc
CFLAGS = -Wall -Wextra -g
DEPS = # Add your header files here
OBJ = # Add your object files here
TARGET = ctestprobe

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ) $(TARGET)