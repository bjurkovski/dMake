#include <cstdlib>
#include <cstdio>
#include <iostream>

#include <mpi.h>
#include "makefile.h"
#include "distributedMake.h"

using namespace std;

int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	int myRank, nTasks;
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &nTasks);

	DistributedMake* make = new DistributedMake(nTasks, myRank);
	if(myRank == 0) {
		Makefile mkfile;

		if(argc > 1)
			mkfile.read(argv[1]);
		else
			mkfile.read();

		make->run(mkfile);
	}
	else {
		make->runSlave();
	}

	delete make;
	MPI_Finalize();
	return 0;
}

