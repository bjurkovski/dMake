#ifndef DMAKE_DISTRIBUTED_MAKE_H
#define DMAKE_DISTRIBUTED_MAKE_H

#include "make.h"

#include <set>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

#include <mpi.h>

enum {
	TASK_MESSAGE = 123,
	RESPONSE_MESSAGE,
	NUM_FILES_MESSAGE,
	FILE_SIZE_MESSAGE,
	FILE_MESSAGE,
	NUM_COMMANDS_MESSAGE,
	COMMANDS_MESSAGE,
	FINISH_MESSAGE
};

class DistributedMake : public Make {
	protected:
		int numCores;
		int coreId;
		int lastUsedCore;
		std::map<std::string, Rule*> rules;
		std::map<std::string, bool> ruleIsFinished;
		std::set<Rule*> initialSet;
		std::vector<std::string> coreWorkingOn;
		std::vector<MPI_Request> mpiRequests;
		std::vector<int> resultCodes;
	private:
		char procFolder[20];
		int numFilesToReceive;

		void createInitialSet(std::string startRule);
		std::vector<Rule*> topologicalSort();

		char* serializeFile(std::string filename, int& size);
		char* deserializeFile(char* file, std::string& filename);

		bool canSendTask(Rule* rule);
		bool sendTask(Rule* rule);
		void receiveResponse();
		std::vector<std::string> receiveTask();
		std::vector<std::string> executeCommands(std::vector<std::string> commands);
		void sendResponse(std::vector<std::string> newFiles);
	public:
		DistributedMake(int numCores, int coreId);
		virtual ~DistributedMake() { }

		void run(Makefile makefile);
		void run(Makefile makefile, std::string startRule);
		void runSlave();
};

#endif
