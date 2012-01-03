#include "makefile.h"

#define MAX_LINE_SIZE 256

using namespace std;

Rule::Rule(string name) {
	this->name = name;
	FILE* file = fopen(name.c_str(), "r");
	if(file != NULL) {
		fclose(file);
		struct stat attrib;
		stat(name.c_str(), &attrib);
		timeModified = *gmtime(&(attrib.st_mtime));
		isAFile = true;
	}
	else {
		isAFile = false;
	}
}

void Rule::addCommand(string command) {
	commands.push_back(command);
}

void Rule::addDependency(Rule* dependency) {
	if(dependency->isFile() && isFile() && (mktime(&(dependency->timeModified)) > mktime(&timeModified))) {
		isAFile = false;
	}
	else if(!dependency->isFile())
		isAFile = false;

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

vector<Rule*> Rule::getRuleDependencies() {
	vector<Rule*> d;
	for(unsigned int i=0; i<dependencies.size(); i++) {
		if(!dependencies[i]->isFile())
			d.push_back(dependencies[i]);
	}
	return d;
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
	//return ((getCommands().size()==0) && (getDependencies().size()==0));
	return isAFile;
}

void Makefile::addVariable(string varName, string varValue) {
	string keyValue[2] = {varName, varValue};
	for(int i=0; i<2; i++) {
		unsigned int beg=0, size=keyValue[i].size();
		while(beg<keyValue[i].size() && keyValue[i][beg]==' ') {
			beg++;
		}

		while(size>0 && keyValue[i][size-1]==' ') {
			size--;
		}

		size -= beg;
		keyValue[i] = keyValue[i].substr(beg, size);
	}

	variables[keyValue[0]] = keyValue[1];
}

bool Makefile::isVariable(string depName) {
	if(depName.size() >= 4) {
		if(depName[0]=='$' && depName[1]=='(' 
			&& depName[depName.size()-1]==')') {
			return true;
		}
	}
	return false;
}

string Makefile::getVariableValue(string variable) {
	return variables[variable.substr(2, variable.size()-3)];
}

void Makefile::addDependency(Rule* rule, string dependencyName) {
	map<string, Rule*>::iterator it = rules.find(dependencyName);
	Rule* depRule = NULL;

	if(it == rules.end()) {
		depRule = new Rule(dependencyName);
		rules[dependencyName] = depRule;
	}
	else {
		depRule = it->second;
	}

	rule->addDependency(depRule);
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
					cout << "\ton line: '" << line << "'" << endl;
					exit(1);
				}

				rule->addCommand(&line[1]);
			}
			else {
				// Test the type of line
				if(strchr(line, '=')) { // It's a variable definition
					char *var, *val;
					var = strtok(line, "=");
					val = strtok(NULL, "=");
					if(val == NULL) {
						addVariable(var, "");
					} else {
						addVariable(var, val);
					}
				}
				else { // It's a rule definition
					char *name, *dependencies, *dep;
					name = strtok(line, ":");

					string ruleName;
					if(isVariable(name)) ruleName = getVariableValue(name);
					else ruleName = name;

					map<string, Rule*>::iterator it = rules.find(ruleName);
					if(it == rules.end()) {
						rule = new Rule(ruleName);
						if(rules.size() == 0)
							firstRule = rule;
						rules[ruleName] = rule;
					}
					else {
						rule = it->second;
					}

					dependencies = strtok(NULL, ":");
					dep = strtok(dependencies, " ");
					while(dep != NULL) {
						if(isVariable(dep)) {
							string varValue = getVariableValue(dep);
							istringstream varValueStream(varValue);
							string varPart;
							while(varValueStream >> varPart) {
								addDependency(rule, varPart);
							}
						}
						else {
							addDependency(rule, dep);
						}

						dep = strtok(NULL, " ");
					}
				} // End of line type testing
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
