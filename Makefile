CC = g++
INCLUDE = -I dependencies/include
LIB = -L dependencies/lib
LIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer
CFLAGS = -Wall -g
LDFLAGS = -static-libgcc -static-libstdc++

SRC = main.cpp
TARGET = main

all:
	$(CC) $(CFLAGS) $(INCLUDE) $(LIB) -o $(TARGET) $(SRC) $(LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET)