CC=gcc
CFLAGS= -Wall -Wextra -pedantic 

TARGET=dupfinder
SRC=main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean 
