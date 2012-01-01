#include "makefile.h"

#define MAX_LINE_SIZE 256

using namespace std;

Rule::Rule(string name) {
	this->name = name;
}

void Rule::addCommand(string command) {
	commands.push_back(command);
}

void Rule::addDependency(Rule* dependency) {
	dependencies.push_back(dependency);
	dependency->addRuleUsing(this);
}

void Rule::addRuleUsing(Rule* rule) {
	usedBy.push_back(rule);
}

void Rule::removeRuleUsing(Rule* rule) {
	string ruleName = rule->getName();
	for(vector<Rule*>::iterator i = usedBy.begin(); i != usedBy.end(); i++) {
		Rule* r = *i;
		if(r->getName() == ruleName) {
			usedBy.erase(i);
			return;
		}
	}
}

string Rule::getName() {
	return name;
}

vector<Rule*> Rule::getDependencies() {
	return dependencies;
}

vector<string> Rule::getCommands() {
	return commands;
}

vector<Rule*> Rule::getRulesUsing() {
	return usedBy;
}

string Rule::serialize() {
	return "";
}

void Rule::deserialize(std::string serializedRule) {
}

bool Rule::isFile() {
	return ((getCommands().size()==0) && (getDependencies().size()==0));
}

void Makefile::addVariable(std::string varName, std::string varValue) {
	variables[varName] = varValue;
}

void Makefile::read() {
	read("Makefile");
}

void Makefile::read(const string filename) {
	FILE* file = fopen(filename.c_str(), "r");
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char* buffer = (char*) malloc(sizeof(char)*size);
	if(buffer == NULL) { cout << "Couldn't allocate buffer to read " << filename << "..." << endl; exit(1); }

	fread(buffer, sizeof(char), size, file);

	istringstream iss(buffer);
	char line[MAX_LINE_SIZE];
	//vector<Rule*> rules;
	Rule* rule = NULL;
	while(iss.getline(line, MAX_LINE_SIZE-1)) {
		if(strlen(line) > 1) {
			if(line[0]=='\t') {
				if(rule == NULL) {
					cout << "Error parsing. Command without rule..." << endl;
					exit(1);
				}

				rule->addCommand(&line[1]);
			}
			else {
				char *name, *dependencies, *dep;
				name = strtok(line, ":");
				map<string, Rule*>::iterator it = rules.find(name);

				if(it == rules.end()) {
					rule = new Rule(name);
					if(rules.size() == 0)
						firstRule = rule;
					rules[name] = rule;
				}
				else {
					rule = it->second;
				}

				Rule* depRule = NULL;
				dependencies = strtok(NULL, ":");
				dep = strtok(dependencies, " ");
				while(dep != NULL) {
					it = rules.find(dep);
					if(it == rules.end()) {
						depRule = new Rule(dep);
						rules[dep] = depRule;
					}
					else {
						depRule = it->second;
					}

					rule->addDependency(depRule);
					dep = strtok(NULL, " ");
				}
			}
		}
	}

	free(buffer);
	fclose(file);
}

Rule* Makefile::getFirstRule() {
	return firstRule;
}

map<string, Rule*>& Makefile::getRules() {
	return rules;
}

string Makefile::toString() {
	string ret = "";

	for(map<string, Rule*>::iterator i=rules.begin(); i != rules.end(); i++) {
		ret += "> Rule\n";
		ret += "Name: " + i->first + "\n";
		vector<Rule*> dependencies = i->second->getDependencies();
		vector<string> commands = i->second->getCommands();
		ret += "Dependencies: ";
		for(unsigned int j=0; j<dependencies.size(); j++) {
			ret += dependencies[j]->getName() + " ";
		}
		ret += "\n";
		ret += "Commands:\n";
		for(unsigned int j=0; j<commands.size(); j++) {
			ret += commands[j] + "\n";
		}
	}
	ret += "> First rule: " + firstRule->getName() + "\n";

	return ret;
}
