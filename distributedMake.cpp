#include "distributedMake.h"

#include <iostream>
using namespace std;

#define DEBUG 0

void DistributedMake::createInitialSet(string startRule) {
  Rule* rule = rules[startRule];

  vector<Rule*> deps = rule->getDependencies();
  int numDependencies = 0;
  for(unsigned int i = 0; i < deps.size(); i++) {
    if(!deps[i]->isFile()) numDependencies++;
  }

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

  while(!initialSet.empty()) {
    Rule* r = *(initialSet.begin());
    initialSet.erase(initialSet.begin());

    orderedList.push_back(r);

    vector<Rule*> usedBy = r->getRulesUsing();
    for(unsigned int i=0; i<usedBy.size(); i++) {
      vector<Rule*>::iterator it = find(dependencies[usedBy[i]].begin(), dependencies[usedBy[i]].end(), r);
      dependencies[usedBy[i]].erase(it);

      if(dependencies[usedBy[i]].size() == 0)
	initialSet.insert(usedBy[i]);
    }
  }

  return orderedList;
}

void DistributedMake::serializeFileInfo(int& size, int& type, int output[DMAKE_FILE_INFO_SIZE]) {
	output[0] = size;
	output[1] = type;
}

void DistributedMake::deserializeFileInfo(int input[DMAKE_FILE_INFO_SIZE], int& size, int& type) {
	size = input[0];
	type = input[1];
}

char* DistributedMake::serializeFile(string filename, int& size, string folder) {
  FILE* file = fopen((folder + filename).c_str(), "r");
  if(!file) {
    return NULL;
  }
  fseek(file, 0, SEEK_END); 

  int fileSize = ftell(file);
  int filenameSize = filename.size();
  // The message will contain one line with the file name and a '\n' and then the content of the file
  size = fileSize + filenameSize + 1;

  rewind(file);
  char* buffer = (char*) malloc(sizeof(char)*size);
  if(buffer == NULL) {
    cout << "dmake: *** serializeFile : Couldn't allocate buffer to read " << filename << "..." << endl;
    exit(1);
  }
  sprintf(buffer, "%s\n", filename.c_str());
  fread(&buffer[filenameSize+1], sizeof(char), fileSize, file);
  fclose(file);

  return buffer;
}

char* DistributedMake::deserializeFile(char* file, string& filename) {
  char* token = strtok(file, "\n");
  if (token==NULL){
    cout << "dmake: *** deserializeFile : null token"<< endl;
    exit(1);
  }
  filename = string(token);
  return &file[filename.size() + 1];
}

bool DistributedMake::canSendTask(Rule* rule) {
  vector<Rule*> dependencies = rule->getRuleDependencies();
  for(unsigned int i=0; i<dependencies.size(); i++) {
    if(!ruleIsFinished[dependencies[i]->getName()]) {
      return false;
    }
  }
  return true;
}

int DistributedMake::getTaskDestination(Rule* rule, vector<Rule*>& result, vector<Rule*>& commandFiles) {
	vector<Rule*> files = rule->getFileDependencies();
	vector<string> commands = rule->getCommands();

	for(unsigned int j=0; j<commands.size(); j++) {
		istringstream iss(commands[j]);
		string exec;
		iss >> exec;
		//char* exec = strtok((char*)commands[j].c_str(), " \t");
		if(exec.size() > 2) {
			if(exec[0] == '.' && exec[1] == '/') {
				Rule* commandFile = new Rule(exec);
				files.push_back(commandFile);
				commandFiles.push_back(commandFile);
			}
		}
	}

	vector<Rule*> filesToSend;
	int bestCandidate = -1;
	int maxNumFiles = 0;
	for(int i=0; i<numCores; i++) {
		int currentCore = (lastUsedCore + i + 1) % numCores;
		if(currentCore == coreId) continue;
		if(coreWorkingOn[currentCore] != "") continue;
//*
		map<int, VersionTable>::iterator it = filesSent.find(currentCore);
		VersionTable::iterator itFile;
		int numEqualFiles = 0;
		filesToSend.clear();
		if(it != filesSent.end()) {
			for(unsigned int j=0; j<files.size(); j++) {
				itFile = filesSent[currentCore].find(files[j]->getName());
				if(itFile != filesSent[currentCore].end()) {
					struct tm timeDependency = files[j]->getTimeModified();
					if(fileVersion[files[j]->getName()] <= filesSent[currentCore][files[j]->getName()])
					{
						//cout << "checking core " << currentCore << " - ";
						//cout << "file " << files[j]->getName() << " is already there!" << endl;
						numEqualFiles++;
					}
					else {
						//cout << "checking core " << currentCore << " - ";
						//cout << "file " << files[j]->getName() << " changed!" << endl;
						filesToSend.push_back(files[j]);
					}
				}
				else {
					//cout << "checking core " << currentCore << " - ";
					//cout << "file " << files[j]->getName() << " has not been sent yet..." << endl;
					filesToSend.push_back(files[j]);
				}
			}
		}
		else {
			//cout << "checking core " << currentCore << " - ";
			//cout << "first time communicating with this core" << endl;
			for(unsigned int j=0; j<files.size(); j++) {
				filesToSend.push_back(files[j]);
			}
		}

		if((bestCandidate == -1) || (numEqualFiles > maxNumFiles)) {
			maxNumFiles = numEqualFiles;
			bestCandidate = currentCore;
			result = filesToSend;
		}
//*/
//		result = files;
//		return currentCore;
	}

	return bestCandidate;
}

bool DistributedMake::sendTask(Rule* rule) {
	vector<Rule*> files;
	vector<Rule*> commandFiles;
	int currentCore = getTaskDestination(rule, files, commandFiles);
	if(currentCore!=-1) {
		if (DEBUG)
			cout << "DEBUG -- sendTask : Dispatching rule '" << rule->getName() << "' to core " << currentCore << endl;

//		vector<Rule*> files = rule->getFileDependencies();
//		vector<Rule*> commandFiles = vector<Rule*>();
		vector<string> commands = rule->getCommands();

		unsigned int numFiles = files.size();
		if (DEBUG)
			cout << "DEBUG -- sendTask : Will send " << numFiles << " files to core " << currentCore << endl;
		MPI_Send(&numFiles, 1, MPI_INT, currentCore, NUM_FILES_MESSAGE, MPI_COMM_WORLD);
		for(unsigned int currentFile=0; currentFile<numFiles; currentFile++) {
			int messageSize;
			if (DEBUG)
		  		cout << "DEBUG -- sendTask : Will send " << files[currentFile]->getName() << endl;
			char* buffer = serializeFile(files[currentFile]->getName(), messageSize);
			if(buffer == NULL) {
			  cout << "dmake: *** sendTask : serializeFile returned Null" << endl;
			  exit(1);
			}

			int fileInfo[DMAKE_FILE_INFO_SIZE], fileType;
			if(files[currentFile]->isExecutable()) fileType = EXECUTABLE_FILE;
			else fileType = ORDINARY_FILE;
			serializeFileInfo(messageSize, fileType, fileInfo);

			VersionTable::iterator it = fileVersion.find(files[currentFile]->getName());
			if(it == fileVersion.end())
				filesSent[currentCore][files[currentFile]->getName()] = 0;
			else
				filesSent[currentCore][files[currentFile]->getName()] = it->second;
			MPI_Send(fileInfo, DMAKE_FILE_INFO_SIZE, MPI_INT, currentCore, FILE_SIZE_AND_TYPE_MESSAGE, MPI_COMM_WORLD);
			MPI_Send(buffer, messageSize, MPI_CHAR, currentCore, FILE_MESSAGE, MPI_COMM_WORLD);
			free(buffer);
		}

		int numCommands = commands.size();
		if (DEBUG)
			cout << "DEBUG -- sendTask : Will send " << numCommands << " commands to core " << currentCore << endl;
		MPI_Send(&numCommands, 1, MPI_INT, currentCore, NUM_COMMANDS_MESSAGE, MPI_COMM_WORLD);

		string serializedCommands = "";
		for(int j=0; j<numCommands; j++) {
			if(j>0) serializedCommands += "\n";
			serializedCommands += commands[j];
		}
		if (numCommands != 0)
		  MPI_Send((void*) serializedCommands.c_str(), serializedCommands.size(), MPI_CHAR, currentCore, COMMANDS_MESSAGE, MPI_COMM_WORLD);

		MPI_Irecv(&resultCodes[currentCore], 1, MPI_INT, currentCore, NUM_FILES_MESSAGE, MPI_COMM_WORLD, &mpiRequests[currentCore]);
		coreWorkingOn[currentCore] = rule->getName();
		lastUsedCore = currentCore;
		return true;
	}

	for(unsigned int j=0; j<commandFiles.size(); j++) {
		delete commandFiles[j];
	}
	commandFiles.clear();

  return false;
}

void DistributedMake::receiveResponse() {
	for(int i=0; i<numCores; i++) {
		if(coreWorkingOn[i] == "") continue;

		int completed = 0;
		MPI_Status status;
		int fileInfo[DMAKE_FILE_INFO_SIZE], fileSize, fileType, sizeReceived;
		char* buffer;

		MPI_Test(&mpiRequests[i], &completed, &status);
		if(completed) {
			if (DEBUG) {
				cout << "DEBUG -- receiveResponse : Will receive " << resultCodes[i] << " generated files." << endl;
			}

			for(int j=0; j<resultCodes[i]; j++) {
				MPI_Recv(fileInfo, DMAKE_FILE_INFO_SIZE, MPI_INT, i, FILE_SIZE_AND_TYPE_MESSAGE, MPI_COMM_WORLD, &status);
				deserializeFileInfo(fileInfo, fileSize, fileType);
				if (DEBUG)
					cout << "DEBUG -- receiveResponse : File size is "<< fileSize << endl;
				buffer = (char*) malloc(sizeof(char)*fileSize+1);
				if(buffer == NULL) {
					cout << "dmake: *** receiveResponse : Couldn't allocate buffer to read File" << endl;
					exit(1);
				}
				
				MPI_Recv(buffer, fileSize, MPI_CHAR, i, FILE_MESSAGE, MPI_COMM_WORLD, &status);
				MPI_Get_count(&status, MPI_CHAR, &sizeReceived);
				buffer[sizeReceived] = '\0';
				string filename;
				char* content = deserializeFile(buffer, filename);
				if (DEBUG)
					cout << "DEBUG -- receiveResponse : Master received file '" << filename << "'" << endl;

				int filenameSize = filename.size();
				FILE* newFile = fopen(filename.c_str(), "w");
				fwrite(content, sizeof(char), fileSize - filenameSize - 1, newFile);
				fclose(newFile);
				logicClock++;
				fileVersion[filename] = logicClock;
				filesSent[i][filename] = logicClock;

				if(fileType == EXECUTABLE_FILE) {
					system(("chmod +x " + filename).c_str());
					if (DEBUG)
						cout << "DEBUG -- receiveResponse : Master marking file '" << filename << "' as executable" << endl;
				}

				free(buffer);
			}

			rules[coreWorkingOn[i]]->update();
			ruleIsFinished[coreWorkingOn[i]] = true;
			coreWorkingOn[i] = "";
		}
	}
}

vector<string> DistributedMake::receiveTask() {
  char* buffer;
  int fileInfo[DMAKE_FILE_INFO_SIZE], fileSize, fileType, numCommands, completed=0;
  const int maxCommandSize = 2048;
  int sizeReceived;
  MPI_Status status;
  vector<string> commands;

  MPI_Test(&mpiRequests[0], &completed, &status);
  if(completed) {
    if (DEBUG)
      cout << "DEBUG -- receiveTask : Core " << coreId << " will receive " << numFilesToReceive << " files." << endl;
    for(int i=0; i<numFilesToReceive; i++) {

		MPI_Recv(fileInfo, DMAKE_FILE_INFO_SIZE, MPI_INT, 0, FILE_SIZE_AND_TYPE_MESSAGE, MPI_COMM_WORLD, &status);
		deserializeFileInfo(fileInfo, fileSize, fileType);

      buffer = (char*) malloc(sizeof(char)*fileSize+1);
      if(buffer == NULL) {
	cout << "dmake: *** receiveTask : Couldn't allocate buffer to read File" << endl;
	exit(1);
      }

      MPI_Recv(buffer, fileSize, MPI_CHAR, 0, FILE_MESSAGE, MPI_COMM_WORLD, &status);

      MPI_Get_count(&status, MPI_CHAR, &sizeReceived);
      buffer[sizeReceived] = '\0';
      string filename;
      char* content = deserializeFile(buffer, filename);
      if(buffer == NULL) {
	cout << "dmake: *** receiveTask : deserializeFile returned Null" << endl;
	exit(1);
      }

		if (DEBUG)
	      cout << "DEBUG -- ReceiveTask : Core " << coreId << " received dependency '" << filename << "'" << endl;

      int filenameSize = filename.size();
      char newFilePath[50];
      sprintf(newFilePath, "%s%s", procFolder, filename.c_str());
      FILE* newFile = fopen(newFilePath, "w");
      fwrite(content, sizeof(char), fileSize - filenameSize - 1, newFile);
      fclose(newFile);

		if(fileType == EXECUTABLE_FILE) {
			string command = "chmod +x ";
			system((command + newFilePath).c_str());
			if (DEBUG)
				cout << "DEBUG -- ReceiveTask : Core " << coreId << " marking '" << filename << "' as executable" << endl;
		}

      free(buffer);
    }

    MPI_Recv(&numCommands, 1, MPI_INT, 0, NUM_COMMANDS_MESSAGE, MPI_COMM_WORLD, &status);
		
    if (numCommands > 0) {
      buffer = (char*) malloc(sizeof(char)*numCommands*(maxCommandSize+1)+1);
      if(buffer == NULL) {
	cout << "dmake: *** ReceiveTask : Couldn't allocate buffer to read commands" << endl;
	exit(1);
      }

      MPI_Recv(buffer, numCommands*(maxCommandSize+1), MPI_INT, 0, COMMANDS_MESSAGE, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_CHAR, &sizeReceived);
      buffer[sizeReceived] = '\0';
		  
      char* command = strtok(buffer, "\n");
      while(command) {
	commands.push_back(command);
	command = strtok(NULL, "\n");
      }

      free(buffer);
    }

    return commands;
  }
  else {
    return commands;
  }
}

vector<string> DistributedMake::executeCommands(vector<string> commands) {
	/*
	TO DO: Since all the received files will be in temporary directories 
	(like .dmake_proc1, .dmake_proc2, etc) we should add a "cd .dmake_procX && "
	before each command. The name of the right folder is stored in the variable
	"procFolder".
	*/
	const string tempFile = ".tempFile";
	string folder = procFolder; // convert char[20] into string
	string lsCommand = "cd "+folder+" && ls -l | sed 's/  */ /g' |cut -d ' ' -f 6-8 > " + tempFile;
	string rmCommand = "cd "+folder+" && rm -f " + tempFile + "*";

	vector<string> newFiles; // Files produced by the execution of the commands

	for(unsigned int j=0; j<commands.size(); j++){
		if (DEBUG)
			cout << "DEBUG -- executeCommands : Core " << coreId << " :\t" << j << " : " << commands[j] <<endl;
		// Execution
		// TODO : redirect the output flow in the shell where dmake is executed
		system((lsCommand + "1").c_str());
		int ret = system(("cd "+folder+" &&"+commands[j]).c_str());
		if (DEBUG)
			cout << "DEBUG -- executeCommands : Core " << coreId << " executing command '" << commands[j] << "'. Return code: " << ret << endl;
		system((lsCommand + "2").c_str());

		string tempFile1, tempFile2;
		ifstream tempFile1Stream((folder+tempFile + "1").c_str(), ios::in); 
		ifstream tempFile2Stream((folder+tempFile + "2").c_str(), ios::in);
		if (tempFile1Stream && tempFile2Stream){
			// first line is empty
			getline(tempFile1Stream, tempFile1);
			getline(tempFile2Stream, tempFile2);
			getline(tempFile1Stream, tempFile1);
			getline(tempFile2Stream, tempFile2);
			bool goOn = true;
			bool hasFile1, hasFile2;	    
			while(goOn) {
				if (DEBUG)
					cout << "DEBUG -- executeCommands : Core " << coreId << " is comparing '" << tempFile1.substr(17, tempFile1.size() - 17) << "' and '" << tempFile2.substr(17, tempFile2.size() - 17) << "' : ";
				// file not modified
				if (tempFile1 == tempFile2) { 
					hasFile1 = getline(tempFile1Stream, tempFile1);
					hasFile2 = getline(tempFile2Stream, tempFile2);
					goOn =  hasFile1 && hasFile2;
					if (DEBUG)
						cout << "they're equal." << endl;
				}
				// file modified
				else if (tempFile1.substr(17, tempFile1.size() - 17) 
						== tempFile2.substr(17, tempFile2.size() - 17)) { 
					newFiles.push_back(tempFile1.substr(17, tempFile1.size() - 17));
					hasFile1 = getline(tempFile1Stream, tempFile1);
					hasFile2 = getline(tempFile2Stream, tempFile2);
					goOn =  hasFile1 && hasFile2;
					if (DEBUG)
						cout << "file changed." << endl;
				}
				// removed file
				else if (tempFile1.substr(17, tempFile1.size() - 17) 
						< tempFile2.substr(17, tempFile2.size() - 17)){ 
					hasFile1 = getline(tempFile1Stream, tempFile1);
					goOn =  hasFile1;
					if (DEBUG)
						cout << "first file was removed." << endl;
				}
				// new file
				else{
					newFiles.push_back(tempFile2.substr(17, tempFile2.size() - 17));
					hasFile2 = getline(tempFile2Stream, tempFile2);
					goOn =  hasFile2;
					if (DEBUG)
						cout << "second file is new." << endl;
				}
			}
			// Remaining new files
			while (hasFile2) {
				if (DEBUG)
					cout << "DEBUG -- executeCommands : Core " << coreId << " generated file '" << tempFile2.substr(17, tempFile2.size() - 17) << "'" << endl;
				newFiles.push_back(tempFile2.substr(17, tempFile2.size() - 17));
				hasFile2 = getline(tempFile2Stream, tempFile2);
			}
		}
		system(rmCommand.c_str());
	}
	return newFiles;
}

void DistributedMake::sendResponse(vector<string> newFiles) {
	int numFiles = newFiles.size();
	int fileInfo[DMAKE_FILE_INFO_SIZE], messageSize, fileType;
	if (DEBUG)
		cout << "DEBUG -- sendResponse : numFiles is " << numFiles << endl;
	MPI_Send(&numFiles, 1, MPI_INT, 0, NUM_FILES_MESSAGE, MPI_COMM_WORLD);
	for(unsigned int i=0; i<newFiles.size(); i++) {
		if (DEBUG)
			cout << "DEBUG -- sendResponse : Serializing " << newFiles[i] << "..." << endl;
		char* buffer = serializeFile(newFiles[i], messageSize, procFolder);
		if(buffer == NULL) {
			cout << "dmake: *** sendResponse : serializeFile returned Null" << endl;
			exit(1);
		}

		string folder = procFolder;
		Rule f(folder+newFiles[i]);
		if(f.isExecutable()) fileType = EXECUTABLE_FILE;
		else fileType = ORDINARY_FILE;
		serializeFileInfo(messageSize, fileType, fileInfo);

		MPI_Send(fileInfo, DMAKE_FILE_INFO_SIZE, MPI_INT, 0, FILE_SIZE_AND_TYPE_MESSAGE, MPI_COMM_WORLD);
		if (DEBUG)
			cout << "DEBUG -- sendResponse : message size is " << messageSize << endl;
		MPI_Send(buffer, messageSize, MPI_CHAR, 0, FILE_MESSAGE, MPI_COMM_WORLD);
		free(buffer);
	}
	if (DEBUG)
		cout << "DEBUG -- sendResponse : Core " << coreId << " is sending result back to master!" << endl;
	MPI_Irecv(&numFilesToReceive, 1, MPI_INT, 0, NUM_FILES_MESSAGE, MPI_COMM_WORLD, &mpiRequests[0]);
}

DistributedMake::DistributedMake(int numCores, int coreId) : coreWorkingOn(numCores, ""), resultCodes(numCores, 0) {
  this->numCores = numCores;
  this->coreId = coreId;
  this->lastUsedCore = 0;
  this->logicClock = 0;

  MPI_Request r;
  for(int i=0; i<numCores; i++) {
    mpiRequests.push_back(r);
  }
}

void DistributedMake::run(Makefile makefile) {
  run(makefile, makefile.getFirstRule()->getName());
}

void DistributedMake::run(Makefile makefile, string startRule) {
	sprintf(procFolder, "./");

	rules = makefile.getRules();
	if(rules.find(startRule) == rules.end()) {
		cout << "dmake: *** run : No rule to make target '" << startRule << "'.  Stop." << endl;
		exit(1);
	}

	unsigned int currentRule = 0;
	createInitialSet(startRule);
	vector<Rule*> orderedList = topologicalSort();
	for(unsigned int i=currentRule; i<orderedList.size(); i++)
		ruleExecuting.push_back(false);

	while(1) {
		if(currentRule < orderedList.size()) {
			while(ruleIsFinished[orderedList[currentRule]->getName()])
				currentRule++;

			bool canExecute = false;
			unsigned int toExecute = currentRule;
			for(unsigned int i=currentRule; i<orderedList.size(); i++) {
				if(!ruleExecuting[i] && canSendTask(orderedList[i])) {
					canExecute = true;
					toExecute = i;
				}
			}

			if(canExecute) {
				if(toExecute == orderedList.size()-1) {
					executeCommands(orderedList[toExecute]->getCommands());
					int sendValue = 1;
					//			TO DO: Try to use broadcast. Problem: we don't have a tag identifying
					//			the kind of message being passed.
					//			MPI_Bcast(&sendValue, 1, MPI_INT, coreId, MPI_COMM_WORLD);
					for(int i=0; i<numCores; i++) {
						if(i==coreId) continue;
						MPI_Send(&sendValue, 1, MPI_INT, i, FINISH_MESSAGE, MPI_COMM_WORLD);
					}
					if (DEBUG)
						cout << "DEBUG -- run : Master is finishing. Sending message to slaves." << endl;
					break;
				}
				else {
					if(sendTask(orderedList[toExecute])) {
						ruleExecuting[toExecute] = true;
					}
				}
			}
		}
		receiveResponse();
	}
}

void DistributedMake::runSlave() {
	int value = 0;
	int completed = 0;
	MPI_Request request;
	MPI_Status status;
	vector<string> commands;
	vector<string> newFiles;

	MPI_Irecv(&value, 1, MPI_INT, MPI_ANY_SOURCE, FINISH_MESSAGE, MPI_COMM_WORLD, &request);
	MPI_Irecv(&numFilesToReceive, 1, MPI_INT, 0, NUM_FILES_MESSAGE, MPI_COMM_WORLD, &mpiRequests[0]);

	char mkdirCommand[80];
	char rmdirCommand[80];
	sprintf(procFolder, ".dmake_proc%d/", coreId);
	sprintf(mkdirCommand, "mkdir %s 2> /dev/null", procFolder);
	system(mkdirCommand);

	while(1) {
		MPI_Test(&request, &completed, &status);
		if(completed) {
			if (DEBUG)
				cout << "DEBUG -- runSlave : Core " << coreId << " will finish its execution." << endl;
			break;
		}

		commands = receiveTask();
		if(commands.size() > 0) {
			newFiles = executeCommands(commands);
			sendResponse(newFiles);
		}
	}

  sprintf(rmdirCommand, "rm %s -rf 2> /dev/null", procFolder);
  system(rmdirCommand);
}
