CC = g++
MPICC = mpicxx
CFLAGS = -Wall
LFLAGS =
OBJS = makefile.o distributedMake.o
OUTPUT = dmake

$(OUTPUT): main.cpp $(OBJS)
	$(MPICC) $^ -o $@ $(CFLAGS) $(LFLAGS)

%.o: %.cpp %.h
	$(MPICC) -c $< -o $@ $(CFLAGS)

clean:
	rm $(OUTPUT)

