CC = g++
MPICC = mpicxx
CFLAGS = -Wall -g
LFLAGS =
OBJS = makefile.o distributedMake.o
OUTPUT = dmake

all: $(OUTPUT) benchmark

$(OUTPUT): main.cpp $(OBJS)
	$(MPICC) $^ -o $@ $(CFLAGS) $(LFLAGS)

benchmark: benchmark.cpp
	$(CC) $^ -o $@ $(CFLAGS)

%.o: %.cpp %.h
	$(MPICC) -c $< -o $@ $(CFLAGS)

clean:
	rm $(OUTPUT)

