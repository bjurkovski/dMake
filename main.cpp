#include <cstdlib>
#include <cstdio>
#include <iostream>

#include "makefile.h"
#include "distributedMake.h"

using namespace std;

int main(int argc, char* argv[]) {
	Makefile mkfile;

	if(argc > 1)
		mkfile.read(argv[1]);
	else
		mkfile.read();

	Make* make = new DistributedMake();
	make->run(mkfile);

	delete make;
	return 0;
}
