#ifndef TCONFIG_H
#define TCONFIG_H

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <utility>

#include "const.h"
#include "line.h"
#include "math_eval.h"

using namespace std;

typedef struct { double low, high, burplevel; } VarCut;
typedef VarCut CompCut;
typedef VarCut CorCut;

class TConfig {

  // ClassDef(TConfig, 0)  // config file parser

public:
  TConfig();
  TConfig(const char *conf_file, const char *run_list = NULL);
  virtual ~TConfig();
  const char*	GetConfFile()	  {return fConfFile;}
  const char*	GetDir()	      {return dir;}
  const char* GetPattern()    {return pattern;}
  const char*	GetTreeName()	  {return tree;}
  const char* GetTreeCut()    {return tcut;}
  bool		    GetLogy()	      {return logy;}
  set<int>	  GetRuns()	      {return fRuns;}			// for ChectStat
  set<int>	  GetBoldRuns()	  {return fBoldRuns;}	// for ChectStat
  set<string> GetVars()	      {return fVars;}
  vector<pair<long, long>> GetEntryCuts() {return ecuts;}

  set<string>									GetSolos()	    {return fSolos;}
  vector<string>		          GetSoloPlots()  {return fSoloPlots;}
  map<string, VarCut>         GetSoloCuts()   {return fSoloCuts;}

	// custom variables: for CheckRun
  set<string>									GetCustoms()    {return fCustoms;}	
  vector<string>							GetCustomPlots(){return fCustomPlots;}
  map<string, VarCut>					GetCustomCuts() {return fCustomCuts;}
	map<string, Node *>					GetCustomDefs()	{return fCustomDefs;}

	// slope: for CheckStat
  set<pair<string, string>>					GetSlopes()			{return fSlopes;}	
  vector<pair<string, string>>			GetSlopePlots()	{return fSlopePlots;}
  map<pair<string, string>, VarCut> GetSlopeCuts()  {return fSlopeCuts;}

  set<pair<string, string>>						GetComps()			{return fComps;}
  map<pair<string, string>, VarCut>		GetCompCuts()   {return fCompCuts;}
  vector<pair<string, string>>				GetCompPlots()	{return fCompPlots;}

  set<pair<string, string>>					GetCors()			{return fCors;}
  vector<pair<string, string>>			GetCorPlots()	{return fCorPlots;}
  map<pair<string, string>, VarCut> GetCorCuts()  {return fCorCuts;}

  bool ParseRun(char *line);
  bool ParseSolo(char *line);
  bool ParseComp(char *line);
  bool ParseSlope(char *line);
  bool ParseCor(char *line);
  bool ParseCustom(char *line);
  bool ParseEntryCut(char *line);
	void add_variables(Node * node);	// used only by ParseCustom
  bool ParseOtherCommands(char *line);
  void ParseConfFile();
  void ParseRunFile();
  double ExtractValue(Node *node); // parse mathematical expression to extract value

private:
  const char *fConfFile;
  const char *fRunFile; // run list file
  const char *dir     = NULL;
  const char *pattern = NULL;
  const char *tree    = NULL;
  const char *tcut    = NULL; // tree cut
  vector<pair<long, long>> ecuts;  // cut on entry number
  map<string, const char *> ftrees;   // friend trees
  
  bool logy = false;
  set<int> fRuns;	      // all runs that are going to be checked
  // set<int> fSlugs;   // FIXME slugs? -- not now
  set<int> fBoldRuns;	  // runs that will be emphasized in plots
  set<string> fVars;    // all variables that are going to be checked.
			                  // this one diffs from fSolos because some variables 
			                  // appears in fComps or fCors but not in fSolos
  set<string>         fSolos;
  vector<string>      fSoloPlots;	  // solos that needed to be drawn, need to be in order
  map<string, VarCut>	fSoloCuts;

	set<string>					fCustoms;
	vector<string>			fCustomPlots;
  map<string, VarCut> fCustomCuts;
	map<string, Node *>	fCustomDefs;	// custom variables' def

  set<pair<string, string>>					fComps;
  vector<pair<string, string>>			fCompPlots;
  map<pair<string, string>, VarCut>	fCompCuts;

  set<pair<string, string>>					fSlopes;
  vector<pair<string, string>>			fSlopePlots;
  map<pair<string, string>, VarCut>	fSlopeCuts;

  set<pair<string, string>>					fCors;
  vector<pair<string, string>>			fCorPlots;
  map<pair<string, string>, VarCut>	fCorCuts;
};

// ClassImp(TConfig);

TConfig::TConfig() : fConfFile(0) {}
TConfig::TConfig(const char* conf_file, const char * run_list) :
  fConfFile(conf_file),
  fRunFile(run_list)
{}

TConfig::~TConfig() {
  fRuns.clear();
  fBoldRuns.clear();
  fVars.clear();
  fSolos.clear();
  fSoloPlots.clear();
  fSoloCuts.clear();
  fComps.clear();
  fCompPlots.clear();
  fCompCuts.clear();
  fSlopes.clear();
  fSlopePlots.clear();
  fSlopeCuts.clear();
  fCors.clear();
  fCorPlots.clear();
  fCorCuts.clear();
  cout << INFO << "End of TConfig" << ENDL;
}

void TConfig::ParseConfFile() {
  // conf file setness
  if (fConfFile == NULL) {
    cerr << FATAL << "no conf file specified" << ENDL;
    exit(1);
  }

  // conf file existance and readbility
  ifstream ifs (fConfFile);
  if (! ifs.is_open()) {
    cerr << FATAL << "conf file " << fConfFile << " doesn't exist or can't be read." << ENDL;
    exit(2);
  }

  // parse config file
  /*  session number:
   *  1: runs
   *  2: solos
   *  4: comparisons
   *  8: slopes
   * 16: correlations
   */
  char current_line[MAX];
  int  session = 0;
  int  nline = 0;
  while (ifs.getline(current_line, MAX)) {
    nline++;
    if (!StripComment(current_line)) {
      cerr << WARNING << "can't strip comment in line: " << nline << ", skip it." << ENDL;
      continue;
    }
    StripSpaces(current_line);

    if (current_line[0] == '\0') continue;  // blank line after removing comment and spaces

    if (current_line[0] == '@') {
      if (strcmp(current_line, "@runs") == 0)
        session = 1;
      else if (strcmp(current_line, "@solos") == 0)
        session = 2;
      else if (strcmp(current_line, "@comparisons") == 0)
        session = 4;
      else if (strcmp(current_line, "@slopes") == 0)
        session = 8;
      else if (strcmp(current_line, "@correlations") == 0)
        session = 16;
      else if (strcmp(current_line, "@customs") == 0)
        session = 32;
      else if (strcmp(current_line, "@entrycuts") == 0)
        session = 64; 
      else if (ParseOtherCommands(current_line)) 
	      ;
      else {
        cerr << WARNING << "Can't parse line " << nline << ". Ignore it" << ENDL;
        session = 0;
      }
      continue;
    }

    bool parsed = false;
    if (session & 1) {  // runs
      parsed = ParseRun(current_line);
    } else if (session & 2) {
      parsed = ParseSolo(current_line);
    } else if (session & 4) {
      parsed = ParseComp(current_line);
    } else if (session & 8) {
      parsed = ParseSlope(current_line);
    } else if (session & 16) {
      parsed = ParseCor(current_line);
    } else if (session & 32) {
      parsed = ParseCustom(current_line);
    } else if (session & 64) {
      parsed = ParseEntryCut(current_line);
    } else {
      cerr << WARNING << "unknown session, ignore line " << nline << ENDL;
      continue;
    }

    if (!parsed)
      cerr << ERROR << "line " << nline << " can't be parsed correclty." << ENDL;
  }
  ifs.close();

  if (fRunFile)
    ParseRunFile();

  cout << INFO << "Configuration in config file: " << fConfFile << ENDL;
  if (fRuns.size() > 0)     cout << "\t" << fRuns.size() << " Runs\n";
  if (fSolos.size() > 0)    cout << "\t" << fSolos.size() << " Solo variables\n";
  if (fCustoms.size() > 0)  cout << "\t" << fCustoms.size() << " Custom variables\n";
  if (fComps.size() > 0)    cout << "\t" << fComps.size() << " Comparison pairs\n";
  if (fSlopes.size() > 0)   cout << "\t" << fSlopes.size() << " Slopes\n";
  if (fCors.size() > 0)     cout << "\t" << fCors.size() << " Correlation pairs\n";
  if (dir)
    cout << "\t" << "root file dir: " << dir << endl;
  if (pattern)
    cout << "\t" << "file name pattern: " << pattern << endl;
  if (tree)
    cout << "\t" << "read tree: " << tree << endl;
  if (ecuts.size()) {
    cout << "\t" << "entry cut: (-1 means the end entry)" << endl;
    for (pair<long, long> cut : ecuts)
      cout << "\t\t" << cut.first << "\t" << cut.second << endl;
  }
}

void TConfig::ParseRunFile() {
  if (!fRunFile) {
    cerr << WARNING << "No run list file specified." << ENDL;
    return;
  }

  ifstream ifs(fRunFile);
  if (! ifs.is_open()) {
    cerr << FATAL << "run list file " << fRunFile << " doesn't exist or can't be read." << ENDL;
    exit(2);
  }

  char line[20];
  int  nline = 0;
  while (ifs.getline(line, MAX)) {
    nline++;

    StripComment(line);
    StripSpaces(line);
    if (line[0] == '\0') continue;

    ParseRun(line);
  }
}

bool TConfig::ParseRun(char *line) {
  if (!line) {
    cerr << ERROR << "empty input" << ENDL;
    return false;
  }

  bool bold = false;
  if (Contain(line, "+") && Contain(line, "-")) {
    cerr << ":WARNING\t bold sign (+) can only be applied to single run, not run range" << ENDL;
    return false;
  }

  if (line[Size(line)-1] == '+') {  // bold runs
    bold = true;
    line[Size(line)-1] = '\0';
    StripSpaces(line);
  }

  int start=-1, end=-1;
  vector<char*> fields = Split(line, '-');
  switch (fields.size()) {
    case 2:
      if (!IsInteger(fields[1])) {
        cerr << WARNING << "invalid end run in range: " << line << ENDL;
        return false;
      }
      end = atoi(fields[1]);
    case 1:
      if (!IsInteger(fields[0])) {
        cerr << WARNING << "invalid (start) run in line: " << line << ENDL;
        return false;
      }
      start = atoi(fields[0]);
      if (end == -1)
        end = start;
  }

  for (int run=start; run<=end; run++) {
    fRuns.insert(run); 
    if (bold)
      fBoldRuns.insert(run);
  }

  return true;
}

bool TConfig::ParseSolo(char *line) {
  vector<char*> fields = Split(line, ';');
  VarCut cut = {1024, 1024, 1024};
  char* var;
  switch (fields.size()) {
    case 4:
      StripSpaces(fields[3]);
      if (!IsEmpty(fields[3])) {
        double val = ExtractValue(ParseExpression(fields[3]));
        if (val == -9999) {
          cerr << ERROR << "Invalid value for burplevel cut" << ENDL;
          return false;
        }
        cut.burplevel = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for upper limit cut" << ENDL;
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for lower limit cut" << ENDL;
          return false;
        }
        cut.low = val;
      }
    case 1:
      var = fields[0];
      break;
    default:
      cerr << ERROR << "At most 4 fields per solo line!" << ENDL;
      return false;
  }

  bool plot = false;
  if (Size(var) && var[Size(var)-1] == '+') { // field could be empty
    plot = true;
    var[Size(var)-1] = '\0';  // remove '+' 
    StripSpaces(var);	  // in case space before '+';
  }

  if (IsEmpty(var)) {
    cerr << ERROR << "Empty variable for solo. " << ENDL;
    return false;
  }
  if (fSolos.find(var) != fSolos.end()) {
    cerr << WARNING << "repeated solo variable, ignore it. " << ENDL;
    return false;
  }

  fSolos.insert(var);
  if (plot) fSoloPlots.push_back(var);
  fSoloCuts[var] = cut;
  fVars.insert(var);
  return true;
}

bool TConfig::ParseCustom(char *line) {
  vector<char*> fields = Split(line, ';');
  VarCut cut = {1024, 1024, 1024};
  char *var;
  char *def;
  switch (fields.size()) {
    case 4:
      StripSpaces(fields[3]);
      if (!IsEmpty(fields[3])) {
        double val = ExtractValue(ParseExpression(fields[3]));
        if (val == -9999) {
          cerr << ERROR << "Invalid value for burplevel cut" << ENDL;
          return false;
        }
        cut.burplevel = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for upper limit cut" << ENDL;
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for lower limit cut" << ENDL;
          return false;
        }
        cut.low = val;
      }
    case 1:
			{
				vector<char *> vfields = Split(fields[0], ':');
				if (vfields.size() != 2) {
					cerr << ERROR << "Wrong format in defining custom variable (var: definition): " << fields[0] << ENDL;
					return false;
				}
				var = vfields[0];
				def = vfields[1];
			}
      break;
    default:
      cerr << ERROR << "At most 4 fields per custom line!" << ENDL;
      return false;
  }

  bool plot = false;
  if (Size(var) && var[Size(var)-1] == '+') { // field could be empty
    plot = true;
    var[Size(var)-1] = '\0';  // remove '+' 
    StripSpaces(var);	  // in case space before '+';
  }

  if (IsEmpty(var)) {
    cerr << ERROR << "Empty variable for custom. " << ENDL;
    return false;
  }
  if (fCustoms.find(var) != fCustoms.end()) {
    cerr << WARNING << "repeated custom variable, ignore it. " << ENDL;
    return false;
  }

  Node * node = ParseExpression(def);
  if (node == NULL) {
    cerr << ERROR << "unable to parse definition. \n"
				 << "\t" << def << ENDL;
		return false;
	}

  fCustoms.insert(var);
	add_variables(node);
  if (plot) fCustomPlots.push_back(var);
  fCustomCuts[var] = cut;
	fCustomDefs[var] = node;
  return true;
}

bool TConfig::ParseEntryCut(char *line) {
  long start, end;
  vector<char*> fields = Split(line, ':');
  switch (fields.size()) {
    case 2: // xxxx:xxxx or :xxxx
      if (!IsInteger(fields[1])) {
        cerr << WARNING << "invalid end entry number in range: " << line << ENDL;
        return false;
      }
      end = atoi(fields[1]);
      if (line[0] != ':' && !IsInteger(fields[0])) {
        cerr << WARNING << "invalid end entry number in range: " << line << ENDL;
        return false;
      }
      start = atoi(fields[0]);
      break;
    case 1: // xxxx:  or xxxx
      if (!IsInteger(fields[0])) {
        cerr << WARNING << "invalid end entry number in range: " << line << ENDL;
        return false;
      }
      start = atoi(fields[0]);
      if (line[Size(line) - 1] == ':')  // xxxx:
        end = -1;
      else 
        end = start;
      break;
    default:
      cerr << WARNING << "invalid entry cut in line: " << line << ENDL;
      return false;
  }
  if (end != -1 && start > end) {
    cerr << WARNING << "start value larger than end value in line: " << line << ENDL;
    return false;
  }
  ecuts.push_back(make_pair(start, end));
  return true;
}

void TConfig::add_variables(Node * node) {
	if (node) {
		add_variables(node->lchild);
		add_variables(node->rchild);
		add_variables(node->sibling);
		if (node->token.type == variable) {
			if (fCustoms.find(node->token.value) == fCustoms.cend())	// not one of previous customs
				fVars.insert(node->token.value);
		}
	}
}

bool TConfig::ParseComp(char *line) {
  vector<char*> fields = Split(line, ';');
  VarCut cut = {1024, 1024, 1024};
  vector<char*> vars;
  switch (fields.size()) {
    case 4:
      StripSpaces(fields[3]);
      if (!IsEmpty(fields[3])) {
        double val = ExtractValue(ParseExpression(fields[3]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for diff cut" << ENDL;
          return false;
        }
        cut.burplevel = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for upper limit cut" << ENDL;
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for lower limit cut" << ENDL;
          return false;
        }
        cut.low = val;
      }
    case 1:
      vars = Split(fields[0], ',');
      break;
    default:
      cerr << ERROR << "At most 4 fields per comparison line" << ENDL;
      return false;
  }

  if (vars.size() != 2) {
    cerr << ERROR << "wrong variables (should be: varA,varB) for comparison" << ENDL;
    return false;
  }

  StripSpaces(vars[0]);
  StripSpaces(vars[1]);
  bool plot = false;
  if (Size(vars[1]) && vars[1][Size(vars[1])-1] == '+') {
    plot = true;
    vars[1][Size(vars[1])-1] = '\0';
    StripSpaces(vars[1]);
  }

  if (IsEmpty(vars[0]) || IsEmpty(vars[1])) {
    cerr << ERROR << "empty variable for comparison" << ENDL;
    return false;
  }
  if (fComps.find(make_pair(vars[0], vars[1])) != fComps.end()) {
    cerr << WARNING << "repeated comparison variables, ignore it. " << ENDL;
    return false;
  }

  fComps.insert(make_pair(vars[0], vars[1]));
  if (plot) fCompPlots.push_back(make_pair(vars[0], vars[1]));
  fCompCuts[make_pair(vars[0], vars[1])] = cut;
  if (fCustoms.find(vars[0]) == fCustoms.cend())
    fVars.insert(vars[0]);
  if (fCustoms.find(vars[1]) == fCustoms.cend())
    fVars.insert(vars[1]);
  return true;
}

bool TConfig::ParseSlope(char *line) {
  vector<char*> fields = Split(line, ';');
  VarCut cut = {1024, 1024, 1024};
  vector<char*> vars;
  switch (fields.size()) {
    case 4:
      StripSpaces(fields[3]);
      if (!IsEmpty(fields[3])) {
        double val = ExtractValue(ParseExpression(fields[3]));
        if (val == -9999) {
          cerr << ERROR << "Invalid value for burplevel cut" << ENDL;
          return false;
        }
        cut.burplevel = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for upper limit cut" << ENDL;
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for lower limit cut" << ENDL;
          return false;
        }
        cut.low = val;
      }
    case 1:
      vars = Split(fields[0], ':');
      break;
    default:
      cerr << ERROR << "At most 4 fields per slope line" << ENDL;
      return false;
  }

  if (vars.size() != 2) {
    cerr << ERROR << "wrong variables (should be: varA:varB) for slope" << ENDL;
    return false;
  }

  StripSpaces(vars[0]);
  StripSpaces(vars[1]);
  bool plot = false;
  if (Size(vars[1]) && vars[1][Size(vars[1])-1] == '+') {
    plot = true;
    vars[1][Size(vars[1])-1] = '\0';
    StripSpaces(vars[1]);
  }

  if (IsEmpty(vars[0]) || IsEmpty(vars[1])) {
    cerr << ERROR << "empty variable for slope" << ENDL;
    return false;
  }
  if (fSlopes.find(make_pair(vars[0], vars[1])) != fSlopes.end()) {
    cerr << WARNING << "repeated Slope variables, ignore it. " << ENDL;
    return false;
  }

  fSlopes.insert(make_pair(vars[0], vars[1]));
  if (plot) fSlopePlots.push_back(make_pair(vars[0], vars[1]));
  fSlopeCuts[make_pair(vars[0], vars[1])] = cut;
  return true;
}

bool TConfig::ParseCor(char *line) {
  vector<char*> fields = Split(line, ';');
  VarCut cut = {1024, 1024, 1024};
  vector<char*> vars;
  switch (fields.size()) {
    case 4:
      StripSpaces(fields[3]);
      if (!IsEmpty(fields[3])) {
        double val = ExtractValue(ParseExpression(fields[3]));
        if (val == -9999) {
          cerr << ERROR << "Invalid value for burplevel cut" << ENDL;
          return false;
        }
        cut.burplevel = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << ERROR << "Invalid value for correlation slope high cut" << ENDL;
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << ERROR << "Invalid value for correlation slope low cut" << ENDL;
          return false;
        }
        cut.low = val;
      }
    case 1:
      vars = Split(fields[0], ':');
      break;
    default:
      cerr << ERROR << "At most 2 fields per correlation line" << ENDL;
      return false;
  }

  if (vars.size() != 2) {
    cerr << ERROR << "wrong variables (should be: varA:varB) for correlation" << ENDL;
    return false;
  }

  StripSpaces(vars[0]);
  StripSpaces(vars[1]);
  bool plot = false;
  if (Size(vars[1]) && vars[1][Size(vars[1])-1] == '+') {
    plot = true;
    vars[1][Size(vars[1])-1] = '\0';
    StripSpaces(vars[1]);
  }

  if (IsEmpty(vars[0]) || IsEmpty(vars[1])) {
    cerr << ERROR << "empty variable for correlation" << ENDL;
    return false;
  }
  if (fCors.find(make_pair(vars[0], vars[1])) != fCors.end()) {
    cerr << WARNING << "repeated correlation variables, ignore it. " << ENDL;
    return false;
  }

  fCors.insert(make_pair(vars[0], vars[1]));
  if (plot) fCorPlots.push_back(make_pair(vars[0], vars[1]));
  fCorCuts[make_pair(vars[0], vars[1])] = cut;
  if (fCustoms.find(vars[0]) == fCustoms.cend())
    fVars.insert(vars[0]);
  if (fCustoms.find(vars[1]) == fCustoms.cend())
    fVars.insert(vars[1]);
  return true;
}

bool TConfig::ParseOtherCommands(char *line) {
  if (IsEmpty(line)) {
    return true;
  }
  
  int i=0;
  while (line[i] != '\0') {
    if (line[i] != ' ' && line[i] != '\t') 
      i++;
    else 
      break;
  }
  const char * command = Sub(line, 0, i);
  while (line[i] != '\0' && (line[i] == ' ' || line[i] == '\t'))
    i++;

  const char * value = Sub(line, i);
  if (value[0] == '\0') {
    cerr << WARNING << "no value specified for command: " << command << ENDL;
    return false;
  }

  if (strcmp(command, "@dir") == 0) {
    dir = value;
  } else if (strcmp(command, "@pattern") == 0) {
    pattern = value;
  } else if (strcmp(command, "@tree") == 0) {
    tree = value;
  } else if (strcmp(command, "@cut") == 0) {
    tcut = value;
  } else if (strcmp(command, "@logy") == 0) {
    if (strcmp(value, "true") == 0) 
      logy = true;
  } else if (strcmp(command, "@friendtree") == 0) {
    int ind = Index(value, ';');
    const char * t = StripSpaces(Sub(value, 0, ind));
    const char * f = ind >= 0 ? StripSpaces(Sub(value, ind+1)) : "";
    if (t[0] == '=') {
      cerr << WARNING << "Incorrect friend tree expression: " << t 
           << "Do you miss the alias before =" << ENDL;
      return false;
    }
    ftrees[t] = f;
  } else {
    cerr << WARNING << "Unknow commands: " << command << ENDL;
    return false;
  }

  return true;
}

double TConfig::ExtractValue(Node * node) { 
  if (node == NULL) {
    cerr << ERROR << "NULL node" << ENDL;
    return -9999;
  }

  const char * val = node->token.value;
  double v=0,  v1=0, v2=0;

	if (node->lchild) v1 = ExtractValue(node->lchild);
	if (node->rchild) v2 = ExtractValue(node->rchild);
	if (v1 == -9999 || v2 == -9999)
		return -9999;

  switch (node->token.type) {
    case opt:
      switch (val[0]) {
        case '+':
          return v1 + v2;
        case '-':
          return v1 - v2;
        case '*':
          return v1 * v2;
        case '/':
          return v1 / v2;
        case '%':
          return ((int)v1) % ((int)v2);
      }
    case function1:
			return f1[val](v1);
    case function2:
			return f2[val](v1, ExtractValue(node->lchild->sibling));
    case number:
      return atof(val);
    case variable:
      if (UNITS.find(val) == UNITS.cend()) {
        cerr << ERROR << "unknow unit: " << val << ENDL;
        return -9999;
      }
      return UNITS[val];
    default:
      cerr << ERROR << "unknow token type: " << TypeName[node->token.type] << ENDL;
      return -9999;
  }
	return -9999;
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
