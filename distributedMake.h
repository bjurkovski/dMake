#ifndef DMAKE_DISTRIBUTED_MAKE_H
#define DMAKE_DISTRIBUTED_MAKE_H

#include "make.h"

#include <set>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

class DistributedMake : public Make {
	protected:
		std::map<std::string, Rule*> rules;
		std::map<std::string, bool> ruleIsFinished;
		std::set<Rule*> initialSet;
	private:
		void createInitialSet(std::string startRule);
		std::vector<Rule*> topologicalSort();
	public:
		DistributedMake() { }
		virtual ~DistributedMake() { }

		void run(Makefile makefile);
		void run(Makefile makefile, std::string startRule);
};

#endif
