#ifndef DMAKE_MAKEFILE_H
#define DMAKE_MAKEFILE_H

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

class Rule {
	protected:
		std::string name;
		std::vector<Rule*> dependencies;
		std::vector<std::string> commands;
		std::vector<Rule*> usedBy;
	private:

	public:
		Rule(std::string name);

		void addCommand(std::string command);
		void addDependency(Rule* dependency);
		void addRuleUsing(Rule* rule);
		void removeRuleUsing(Rule* rule);

		std::string getName();
		std::vector<Rule*> getDependencies();
		std::vector<std::string> getCommands();
		std::vector<Rule*> getRulesUsing();

		std::string serialize();
		void deserialize(std::string serializedRule);

		bool isFile();
};

class Makefile {
	protected:
		Rule* firstRule;
		//std::vector<Rule*> rules;
		std::map<std::string, Rule*> rules;
		std::map<std::string, std::string> variables;
	private:
		void addVariable(std::string varName, std::string varValue);
	public:
		void read();
		void read(const std::string filename);

		Rule* getFirstRule();
		std::map<std::string, Rule*>& getRules();

		std::string toString();
};

#endif
