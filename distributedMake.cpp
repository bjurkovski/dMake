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

	if((numDependencies == 0) && !rule->isFile()) {
		cout << "Inserting in the set... " << rule->getName() << endl;
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

	while(!initialSet.empty()) {
		Rule* r = *(initialSet.begin());
		initialSet.erase(initialSet.begin());

		orderedList.push_back(r);

		vector<Rule*> deps = r->getDependencies();
		for(unsigned int i=0; i<deps.size(); i++) {
			deps[i]->removeRuleUsing(r);
			if(deps[i]->getRulesUsing().size() == 0)
				initialSet.insert(deps[i]);
		}
	}

	return orderedList;
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

	createInitialSet(startRule);
	vector<Rule*> orderedList = topologicalSort();

	for(unsigned int i=0; i<orderedList.size(); i++) {
		cout << i << ": " << orderedList[i]->getName() << endl;
	}
}
