# Author: Sampada Sakpal
# UserID: sakpals

## CC  = Compiler.
## CFLAGS = Compiler flags.
CC	= gcc
CFLAGS = -Wall -g -Wextra  -std=gnu99


## OBJ = Object files.
## SRC = Source files.
## EXE = Executable name.

SRC =		protocol_messages.c sha256.c server.c 
OBJ =		protocol_messages.o sha256.o server.o 
EXE = 		server

## Top level target is executable.
$(EXE):	$(OBJ)
		$(CC) $(CFLAGS) -o $(EXE) $(SRC) -pthread


## Clean: Remove object files and core dump files.
clean:
		/bin/rm $(OBJ)


## Clobber: Performs Clean and removes executable file.

clobber: clean
		/bin/rm $(EXE)
