# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -no-pie

# Compile with -O3 optimization
OPTFLAGS = -O3

# Directories
SRC = src
OBJ = obj

SOURCES = $(wildcard $(SRC)/*.c)
OBJECTS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SOURCES))

main: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(DBGFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@ $(DBGFLAGS)

debug: DBGFLAGS = -DDEBUGL -DDEBUGH
debug: main

.PHONY: clean
# Cleans the executable and the object files
clean:
	$(RM) main $(OBJECTS)
