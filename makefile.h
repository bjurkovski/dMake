#ifndef DMAKE_MAKEFILE_H
#define DMAKE_MAKEFILE_H

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <sys/stat.h>	
#include <unistd.h>
#include <ctime>
#include <streambuf>

class Rule {
	protected:
		std::string name;
		std::vector<Rule*> dependencies;
		std::vector<std::string> commands;
		std::vector<Rule*> usedBy;
		bool isAFile;
		bool executableBit;
		struct tm timeModified;
	private:

	public:
		Rule(std::string name);

		void addCommand(std::string command);
		void addDependency(Rule* dependency);
		void addRuleUsing(Rule* rule);
		void removeRuleUsing(Rule* rule);

		std::string getName();
		std::vector<Rule*> getDependencies();
		std::vector<Rule*> getRuleDependencies();
		std::vector<Rule*> getFileDependencies();
		std::vector<std::string> getCommands();
		std::vector<Rule*> getRulesUsing();

		std::string serialize();
		void deserialize(std::string serializedRule);

		bool isFile();
		bool isExecutable();
		void update();
};

class Makefile {
	protected:
		Rule* firstRule;
		std::map<std::string, Rule*> rules;
		std::map<std::string, std::string> variables;
	private:
		void stripString(std::string& str);
		void addVariable(std::string varName, std::string varValue);
		bool isVariable(std::string depName, Rule* rule);
		std::string getVariableValue(std::string variable, Rule* rule);
		void addDependency(Rule* rule, std::string dependencyName);
	public:
		void read();
		void read(const std::string filename);

		Rule* getFirstRule();
		std::map<std::string, Rule*>& getRules();

		std::string toString();
};

#endif
