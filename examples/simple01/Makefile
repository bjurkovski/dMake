all: main.cpp bib.o bib2.o bib3.o bib.h bib2.h bib3.h
	g++ $^ -o exec

bib.o: bib.cpp bib.h
	g++ -c bib.cpp -o bib.o
	echo "Finished rule bib.o"

bib2.o: bib2.cpp bib2.h bib3.o bib3.h
	g++ -c bib2.cpp -o bib2.o

bib3.o: bib3.cpp bib3.h
	g++ -c bib3.cpp -o bib3.o

clean:
	rm exec
	rm *.o
