OBJS = src/main.c

CC = gcc

COMPILER_FLAGS = -w -I/usr/include

LINKER_FLAGS = -L/usr/local/lib -lSDL2

OBJ_NAME = Waffle-16
OBJ_PATH = bin/$(OBJ_NAME)

build : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_PATH)

test : $(OBJ_PATH)
	./$(OBJ_PATH)
