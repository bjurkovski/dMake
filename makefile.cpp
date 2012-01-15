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

vector<Rule*> Rule::getFileDependencies() {
	vector<Rule*> d;
	for(unsigned int i=0; i<dependencies.size(); i++) {
		if(dependencies[i]->isFile())
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
	string ret;
	ret += name + "\n";

	ret += dependencies.size() + "\n";
	for(unsigned int i=0; i<dependencies.size(); i++)
		ret += dependencies[i]->getName() + "\n";

	ret += commands.size() + "\n";
	for(unsigned int i=0; i<commands.size(); i++)
		ret += commands[i] + "\n";

	ret += usedBy.size() + "\n";
	for(unsigned int i=0; i<usedBy.size(); i++)
		ret += usedBy[i]->getName() + "\n";

	ret += isAFile + "\n";
	ret += timeModified.tm_sec + "\n";
	ret += timeModified.tm_min + "\n";
	ret += timeModified.tm_hour + "\n";
	ret += timeModified.tm_mday + "\n";
	ret += timeModified.tm_mon + "\n";
	ret += timeModified.tm_year + "\n";
	ret += timeModified.tm_wday + "\n";
	ret += timeModified.tm_yday + "\n";
	ret += timeModified.tm_isdst + "\n";

	return ret;
}

void Rule::deserialize(std::string serializedRule) {
  stringbuf isString(serializedRule);
  istream is(&isString);
  string line;
  unsigned int counter;

  // Read name
  getline(is, line);
  name = line;

  // Read dependencies
  getline(is, line);
  counter = atoi(line.c_str());
  for(unsigned int i=0; i<counter; i++){
    getline(is, line);
    Rule* rule = new Rule(line);
    addDependency(rule);
  }

  // Read commands
  getline(is, line);
  counter = atoi(line.c_str());
  for(unsigned int i=0; i<counter; i++){
    getline(is, line);
    addCommand(line);
  }

  // Read used_by
  getline(is, line);
  counter = atoi(line.c_str());
  for(unsigned int i=0; i<counter; i++){
    getline(is, line);
    Rule* rule = new Rule(line);
    addRuleUsing(rule);
  }

  // Read isAFile
  getline(is, line);
  isAFile = atoi(line.c_str());

  // Read timeModified
  getline(is, line);
  timeModified.tm_sec  = atoi(line.c_str());
  getline(is, line);
  timeModified.tm_min  = atoi(line.c_str());
  getline(is, line);
  timeModified.tm_hour  = atoi(line.c_str());
  getline(is, line);
  timeModified.tm_mday  = atoi(line.c_str());
  getline(is, line);
  timeModified.tm_mon  = atoi(line.c_str());
  getline(is, line);
  timeModified.tm_year  = atoi(line.c_str());
  getline(is, line);
  timeModified.tm_wday  = atoi(line.c_str());
  getline(is, line);
  timeModified.tm_yday  = atoi(line.c_str());
  getline(is, line);
  timeModified.tm_isdst  = atoi(line.c_str());
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

bool Makefile::isVariable(string depName, Rule* rule) {
	if(depName.size() >= 4) {
		if(depName[0]=='$' && depName[1]=='(' 
			&& depName[depName.size()-1]==')') {
			return true;
		}
	} else if((depName.size() == 2) && (depName[0] == '$') && rule != NULL) {
		switch(depName[1]) {
			case '@':
			case '^':
			case '<':
				return true;
		}
	}

	return false;
}

string Makefile::getVariableValue(string variable, Rule* rule) {
	string ret = "";
	vector<Rule*> dependencies;
	if((variable.size() == 2) && (variable[0] == '$') && rule != NULL) {
		switch(variable[1]) {
			case '@':
				return rule->getName();
			case '^':
				dependencies = rule->getDependencies();
				for(unsigned int i=0; i<dependencies.size(); i++)
					ret += dependencies[i]->getName() + " ";
				return ret;
			case '<':
				dependencies = rule->getDependencies();
				if(dependencies.size() > 0) return dependencies[0]->getName();
				else return "";
		}
	}

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
	Rule* rule = NULL;
	while(iss.getline(line, MAX_LINE_SIZE-1)) {
		if(strlen(line) > 1) {
			if(line[0]=='\t') {
				if(rule == NULL) {
					cout << "Error parsing. Command without rule..." << endl;
					cout << "\ton line: '" << line << "'" << endl;
					exit(1);
				}

				char* word;
				string command;
				word = strtok(line, " \t");
				while(word != NULL) {
					if(isVariable(word, rule)) command += getVariableValue(word, rule);
					else command += word;
					command += " ";
					word = strtok(NULL, " \t");
				}

				rule->addCommand(command);
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
					if(isVariable(name, rule)) ruleName = getVariableValue(name, rule);
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
						if(isVariable(dep, rule)) {
							string varValue = getVariableValue(dep, rule);
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
