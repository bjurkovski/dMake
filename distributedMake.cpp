#include "distributedMake.h"

#include <iostream>
using namespace std;

void DistributedMake::createInitialSet(string startRule) {
	Rule* rule = rules[startRule];

	vector<Rule*> deps = rule->getDependencies();
	int numDependencies = 0;
	for(unsigned int i = 0; i < deps.size(); i++) {
		if(!deps[i]->isFile()) numDependencies++;
	}

	//cout << startRule << " has " << numDependencies << " dependencies... isFile = " << rule->isFile() << endl;
	if((numDependencies == 0) && !rule->isFile()) {
		initialSet.insert(rule);
	}
	else {
		for(unsigned int i = 0; i < deps.size(); i++) {
			createInitialSet(deps[i]->getName());		
		}
	}
}

vector<Rule*> DistributedMake::topologicalSort() {
	vector<Rule*> orderedList;

	map< Rule*, vector<Rule*> > dependencies;
	for(map<string, Rule*>::iterator i = rules.begin(); i != rules.end(); i++) {
		dependencies[i->second] = i->second->getRuleDependencies();
	}

	//cout << "InitialSet size: " << initialSet.size() << endl;
	while(!initialSet.empty()) {
		Rule* r = *(initialSet.begin());
		initialSet.erase(initialSet.begin());

		orderedList.push_back(r);

		//cout << "Checking " << r->getName() << endl;
		vector<Rule*> usedBy = r->getRulesUsing();
		for(unsigned int i=0; i<usedBy.size(); i++) {
			//cout << "   > " << usedBy[i]->getName() << endl;
			vector<Rule*>::iterator it = find(dependencies[usedBy[i]].begin(), dependencies[usedBy[i]].end(), r);
			dependencies[usedBy[i]].erase(it);

			if(dependencies[usedBy[i]].size() == 0)
				initialSet.insert(usedBy[i]);
		}
	}

	return orderedList;
}

bool DistributedMake::canSendTask(Rule* rule) {
	return true;
}

bool DistributedMake::sendTask(Rule* rule) {
	for(int i=0; i<numCores; i++) {
		int currentCore = (lastUsedCore + i + 1) % numCores;
		if(currentCore == coreId) continue;
		if(coresAvailable[currentCore]) {
			cout << "Dispatching rule '" << rule->getName() << "' to core " << currentCore << endl;
			MPI_Send((void*) rule->getName().c_str(), rule->getName().size(), MPI_CHAR, currentCore, TASK_MESSAGE, MPI_COMM_WORLD);
			MPI_Irecv(&resultCodes[currentCore], 1, MPI_INT, currentCore, RESPONSE_MESSAGE, MPI_COMM_WORLD, &mpiRequests[currentCore]);
			coresAvailable[currentCore] = false;
			return true;
		}
	}

	return false;
}

void DistributedMake::receiveResponse() {
	for(int i=0; i<numCores; i++) {
		if(coresAvailable[i]) continue;
		//cout << "trying to receive from core " << i << endl;

		int completed = 0;
		MPI_Status status;
		MPI_Test(&mpiRequests[i], &completed, &status);
		if(completed) {
			coresAvailable[i] = true;
			cout << "Master received return code " << resultCodes[i] << " from core " << i << endl;
		}
	}
}

void DistributedMake::receiveTask() {
	const int maxTaskNameSize = 256;
	int sizeReceived;
	char buf[maxTaskNameSize];
	MPI_Status status;
	MPI_Recv(buf, maxTaskNameSize, MPI_CHAR, 0, TASK_MESSAGE, MPI_COMM_WORLD, &status);
	MPI_Get_count(&status, MPI_CHAR, &sizeReceived);
	buf[sizeReceived] = '\0';
	cout << "Core " << coreId << " received task '" << buf << "'" << endl;
}

vector<string> DistributedMake::executeCommands(vector<string> commands) {
	const string tempFile = ".tempFile";
	string lsCommand = "ls -l | sed 's/  */ /g' |cut -d ' ' -f 6-8 > " + tempFile;
	string rmCommand = "rm -f " + tempFile + "*";

	vector<string> newFiles; // Files produced by the execution of the commands

	for(unsigned int j=0; j<commands.size(); j++){
		cout << "\t" << j << " : " << commands[j] <<endl;

		// Execution
		// TODO : redirect the output flow in the shell where dmake is executed
		system((lsCommand + "1").c_str());
		system(commands[j].c_str());
		system((lsCommand + "2").c_str());

		string tempFile1, tempFile2;
		ifstream tempFile1Stream((tempFile + "1").c_str(), ios::in); 
		ifstream tempFile2Stream((tempFile + "2").c_str(), ios::in); 
		if (tempFile1Stream && tempFile2Stream){
			// first line is empty
			getline(tempFile1Stream, tempFile1);
			getline(tempFile2Stream, tempFile2);
			getline(tempFile1Stream, tempFile1);
			getline(tempFile2Stream, tempFile2);
			bool goOn = true;		    
			while(goOn) {
				if (tempFile1 == tempFile2) { // file not modified
					goOn = getline(tempFile1Stream, tempFile1) 
							&& getline(tempFile2Stream, tempFile2);
				}
				else if (tempFile1.substr(17, tempFile1.size() - 17) 
							== tempFile2.substr(17, tempFile2.size() - 17)) { // file modified
					newFiles.push_back(tempFile1.substr(17, tempFile1.size() - 17));
					goOn = getline(tempFile1Stream, tempFile1) 
							&& getline(tempFile2Stream, tempFile2);
				}
				else { // new file
					newFiles.push_back(tempFile2.substr(17, tempFile2.size() - 17));
					goOn = getline(tempFile2Stream, tempFile2);
				}
			}
			//cout << "File produced : " << newFiles << endl;
		}
	}

	return newFiles;
}

void DistributedMake::sendResponse() {
	int resultCode = 1;
	MPI_Send(&resultCode, 1, MPI_INT, 0, RESPONSE_MESSAGE, MPI_COMM_WORLD);
	cout << "Core " << coreId << " is sending result back to master!" << endl;
}

DistributedMake::DistributedMake(int numCores, int coreId) : coresAvailable(numCores, true), resultCodes(numCores, 0) {
	this->numCores = numCores;
	this->coreId = coreId;
	this->lastUsedCore = 0;

	MPI_Request r;
	for(int i=0; i<numCores; i++) {
		mpiRequests.push_back(r);
	}
}

void DistributedMake::run(Makefile makefile) {
	run(makefile, makefile.getFirstRule()->getName());
}

void DistributedMake::run(Makefile makefile, string startRule) {
	//cout << makefile.toString();
	rules = makefile.getRules();
	if(rules.find(startRule) == rules.end()) {
		cout << "dmake: *** No rule to make target '" << startRule << "'.  Stop." << endl;
		exit(1);
	}

	unsigned int currentRule = 0;
	createInitialSet(startRule);
	vector<Rule*> orderedList = topologicalSort();

	while(1) {
		if(currentRule < orderedList.size()) {
			if(canSendTask(orderedList[currentRule])) {
				sendTask(orderedList[currentRule]);
				currentRule++;
			}
		}

		receiveResponse();
	}
}

void DistributedMake::runSlave() {
	while(1) {
		receiveTask();
		sendResponse();
	}
}
