#include <cstdlib>
#include <cstdio>
#include <iostream>

#include "makefile.h"
#include "distributedMake.h"

using namespace std;

int main() {
	Makefile mkfile;
	mkfile.read("Makefile.test");

	Make* make = new DistributedMake();
	make->run(mkfile);

	delete make;
	return 0;
}
