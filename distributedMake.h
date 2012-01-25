#ifndef DMAKE_DISTRIBUTED_MAKE_H
#define DMAKE_DISTRIBUTED_MAKE_H

#include "make.h"

#include <set>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>	
#include <unistd.h>
#include <sys/types.h>
#include <utime.h>

#include <mpi.h>

enum {
	TASK_MESSAGE = 123,
	NUM_FILES_MESSAGE,
	FILE_SIZE_AND_TYPE_MESSAGE,
	FILE_MESSAGE,
	NUM_COMMANDS_MESSAGE,
	COMMANDS_MESSAGE,
	FINISH_MESSAGE
};

enum {
	ORDINARY_FILE = 456,
	EXECUTABLE_FILE
};

#define DMAKE_FILE_INFO_SIZE 2

typedef std::map< std::string, int > VersionTable;

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
		VersionTable fileVersion;
		std::map<int, VersionTable> filesSent;

		void createInitialSet(std::string startRule);
		std::vector<Rule*> topologicalSort();

		int logicClock;
		void serializeFileInfo(int& size, int& type, int output[DMAKE_FILE_INFO_SIZE]);
		void deserializeFileInfo(int input[DMAKE_FILE_INFO_SIZE], int& size, int& type);

		char* serializeFile(std::string filename, int& size, std::string folder="");
		char* deserializeFile(char* file, std::string& filename);

		bool canSendTask(Rule* rule);
		int getTaskDestination(Rule* rule, std::vector<Rule*>& result, std::vector<Rule*>& commandFiles);
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
