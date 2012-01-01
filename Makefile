CC = g++
CFLAGS = -Wall
LFLAGS =
OBJS = makefileParser.o makefile.o distributedMake.o
OUTPUT = dmake

$(OUTPUT): main.cpp $(OBJS)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

%.o: %.cpp %.h
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm $(OUTPUT)

