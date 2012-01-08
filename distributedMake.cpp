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

void DistributedMake::run(Makefile makefile) {
	run(makefile, makefile.getFirstRule()->getName());
}

void DistributedMake::run(Makefile makefile, string startRule) {
	cout << makefile.toString();
	rules = makefile.getRules();
	if(rules.find(startRule) == rules.end()) {
		cout << "dmake: *** No rule to make target '" << startRule << "'.  Stop." << endl;
		exit(1);
	}

	createInitialSet(startRule);
	vector<Rule*> orderedList = topologicalSort();
	vector<string> commands;
	string lsCommand = "ls -l | sed 's/  */ /g' |cut -d ' ' -f 6-8 > tempFile";
	string rmCommand = "rm -f tempFile*";
	string tempFile1; 
	string tempFile2;

	for(unsigned int i=0; i<orderedList.size(); i++) {
		cout << i << ": " << orderedList[i]->getName() << endl;
		 commands = orderedList[i]->getCommands();
		 for(unsigned int j=0; j<commands.size(); j++){
		   cout << "\t" << j << " : " << commands[j] <<endl;

		   // Execution
		   // TODO : redirect the output flow in the shell where dmake is executed
		   system((lsCommand+"1").c_str());
		   system(commands[j].c_str());
		   system((lsCommand+"2").c_str());

		   ifstream tempFile1Stream("tempFile1", ios::in); 
		   ifstream tempFile2Stream("tempFile2", ios::in); 
		   string newFiles = ""; // Files produced by the execution of the command
		   if (tempFile1Stream && tempFile2Stream){
		     // first line is empty
		     getline(tempFile1Stream, tempFile1);
		     getline(tempFile2Stream, tempFile2);
		     getline(tempFile1Stream, tempFile1);
		     getline(tempFile2Stream, tempFile2);
		     bool goOn = true;		    
		     while(goOn){
		       if (tempFile1 == tempFile2){ // file not modified
			 goOn = getline(tempFile1Stream, tempFile1) 
			   && getline(tempFile2Stream, tempFile2);
		       }
		       else if (tempFile1.substr(17, tempFile1.size() - 17) 
				== tempFile2.substr(17, tempFile2.size() - 17)){ // file modified
			 newFiles += (tempFile1.substr(17, tempFile1.size() - 17) + "\n");
			 goOn = getline(tempFile1Stream, tempFile1) 
			   && getline(tempFile2Stream, tempFile2);
			 }
		       else { // new file
			 newFiles += tempFile2.substr(17, tempFile2.size() - 17);
			 newFiles += "\n";
			 goOn = getline(tempFile2Stream, tempFile2);
		       }
		     }
		     cout << "File produced : " << newFiles << endl;
		   }
		 }
	}
}
