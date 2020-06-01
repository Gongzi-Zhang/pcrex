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

typedef struct { double low, high, stability; } VarCut;
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
  const char*	GetTreeName()	  {return tree;}
  const char*	GetFileType()	  {return ft;}
  const char*	GetJapanPass()	{return japan_pass;}
  const char*	GetDitBPM()	    {return dit_bpm;}
  bool		    GetLogy()	      {return logy;}
  set<int>	  GetRuns()	      {return fRuns;}			// for ChectStat
  set<int>	  GetBoldRuns()	  {return fBoldRuns;}	// for ChectStat
  set<string> GetVars()	      {return fVars;}

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
	void add_variables(Node * node);	// used only by ParseCustom
  bool ParseOtherCommands(char *line);
  void ParseConfFile();
  void ParseRunFile();
  double ExtractValue(Node *node); // parse mathematical expression to extract value

private:
  const char *fConfFile;
  const char *fRunFile; // run list file
  const char *dir     = NULL;
  const char *tree    = NULL;
  const char *ft      = NULL;
  const char *japan_pass  = NULL;
  const char *dit_bpm = NULL;
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
  cout << __PRETTY_FUNCTION__ << ":INFO\t End of TConfig\n";
}

void TConfig::ParseConfFile() {
  // conf file setness
  if (fConfFile == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no conf file specified" << endl;
    exit(1);
  }

  // conf file existance and readbility
  ifstream ifs (fConfFile);
  if (! ifs.is_open()) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t conf file " << fConfFile << " doesn't exist or can't be read.\n";
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
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t can't strip comment in line: " << nline << ", skip it.\n";
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
      else if (ParseOtherCommands(current_line)) 
	      ;
      else {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't parse line " << nline << ". Ignore it\n";
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
    } else {
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t unknown session, ignore line " << nline << endl;
      continue;
    }

    if (!parsed)
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t line " << nline << " can't be parsed correclty.\n";
  }
  ifs.close();

  if (fRunFile)
    ParseRunFile();

  cout << __PRETTY_FUNCTION__ << ":INFO\t Configuration in config file: " << fConfFile << endl;
  if (fRuns.size() > 0)     cout << "\t" << fRuns.size() << " Runs\n";
  if (fSolos.size() > 0)    cout << "\t" << fSolos.size() << " Solo variables\n";
  if (fCustoms.size() > 0)  cout << "\t" << fCustoms.size() << " Custom variables\n";
  if (fComps.size() > 0)    cout << "\t" << fComps.size() << " Comparison pairs\n";
  if (fSlopes.size() > 0)   cout << "\t" << fSlopes.size() << " Slopes\n";
  if (fCors.size() > 0)     cout << "\t" << fCors.size() << " Correlation pairs\n";
  if (ft)
    cout << "\t" << "root file type: " << ft << endl;
  if (dir)
    cout << "\t" << "root file dir: " << dir << endl;
  if (tree)
    cout << "\t" << "read tree: " << tree << endl;
  if (japan_pass)
    cout << "\t" << "japan pass: " << japan_pass << endl;
}

void TConfig::ParseRunFile() {
  if (!fRunFile) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t No run list file specified.\n";
    return;
  }

  ifstream ifs(fRunFile);
  if (! ifs.is_open()) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t run list file " << fRunFile << " doesn't exist or can't be read.\n";
    exit(2);
  }

  char line[20];
  int  nline = 0;
  while (ifs.getline(line, MAX)) {
    nline++;

    StripComment(line);
    StripSpaces(line);
    if (line[0] = '\0') continue;

    ParseRun(line);
  }
}

bool TConfig::ParseRun(char *line) {
  if (!line) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t empty input" << endl;
    return false;
  }

  bool bold = false;
  if (Contain(line, "+") && Contain(line, "-")) {
    cerr << ":WARNING\t bold sign (+) can only be applied to single run, not run range" << endl;
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
	cerr << __PRETTY_FUNCTION__ << ":WARNING\t invalid end run in range: " << line << endl;
	return false;
      }
      end = atoi(fields[1]);
    case 1:
      if (!IsInteger(fields[0])) {
	cerr << __PRETTY_FUNCTION__ << ":WARNING\t invalid (start) run in line: " << line << endl;
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
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid value for stability cut\n";
          return false;
        }
        cut.stability = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for upper limit cut\n";
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for lower limit cut\n";
          return false;
        }
        cut.low = val;
      }
    case 1:
      var = fields[0];
      break;
    default:
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t At most 4 fields per solo line!\n";
      return false;
  }

  bool plot = false;
  if (Size(var) && var[Size(var)-1] == '+') { // field could be empty
    plot = true;
    var[Size(var)-1] = '\0';  // remove '+' 
    StripSpaces(var);	  // in case space before '+';
  }

  if (IsEmpty(var)) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t Empty variable for solo. \n";
    return false;
  }
  if (fSolos.find(var) != fSolos.end()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t repeated solo variable, ignore it. \n";
    return false;
  }

  fSolos.insert(var);
  if (plot) fSoloPlots.push_back(var);
  fSoloCuts[var] = cut;
  fVars.insert(Split(var, '.')[0]);	// for CheckStat, which need it to imply variable type: mean/rms
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
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid value for stability cut\n";
          return false;
        }
        cut.stability = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for upper limit cut\n";
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for lower limit cut\n";
          return false;
        }
        cut.low = val;
      }
    case 1:
			{
				vector<char *> vfields = Split(fields[0], ':');
				if (vfields.size() != 2) {
					cerr << __PRETTY_FUNCTION__ << ":ERROR\t Wrong format in defining custom variable (var: definition): " << fields[0] << endl;
					return false;
				}
				var = vfields[0];
				def = vfields[1];
			}
      break;
    default:
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t At most 4 fields per custom line!\n";
      return false;
  }

  bool plot = false;
  if (Size(var) && var[Size(var)-1] == '+') { // field could be empty
    plot = true;
    var[Size(var)-1] = '\0';  // remove '+' 
    StripSpaces(var);	  // in case space before '+';
  }

  if (IsEmpty(var)) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t Empty variable for custom. \n";
    return false;
  }
  if (fCustoms.find(var) != fCustoms.end()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t repeated custom variable, ignore it. \n";
    return false;
  }

  Node * node = ParseExpression(def);
  if (node == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t unable to parse definition. \n"
				 << "\t" << def << endl;
		return false;
	}

  fCustoms.insert(var);
	add_variables(node);
  if (plot) fCustomPlots.push_back(var);
  fCustomCuts[var] = cut;
	fCustomDefs[var] = node;
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
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for diff cut\n";
          return false;
        }
        cut.stability = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for upper limit cut\n";
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for lower limit cut\n";
          return false;
        }
        cut.low = val;
      }
    case 1:
      vars = Split(fields[0], ',');
      break;
    default:
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t At most 4 fields per comparison line\n";
      return false;
  }

  if (vars.size() != 2) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t wrong variables (should be: varA,varB) for comparison\n";
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
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t empty variable for comparison\n";
    return false;
  }
  if (fComps.find(make_pair(vars[0], vars[1])) != fComps.end()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t repeated comparison variables, ignore it. \n";
    return false;
  }

  fComps.insert(make_pair(vars[0], vars[1]));
  if (plot) fCompPlots.push_back(make_pair(vars[0], vars[1]));
  fCompCuts[make_pair(vars[0], vars[1])] = cut;
  fVars.insert(Split(vars[0], '.')[0]);
  fVars.insert(Split(vars[1], '.')[0]);
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
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid value for stability cut\n";
          return false;
        }
        cut.stability = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for upper limit cut\n";
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Non-number value for lower limit cut\n";
          return false;
        }
        cut.low = val;
      }
    case 1:
      vars = Split(fields[0], ':');
      break;
    default:
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t At most 4 fields per slope line\n";
      return false;
  }

  if (vars.size() != 2) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t wrong variables (should be: varA:varB) for slope\n";
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
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t empty variable for slope\n";
    return false;
  }
  if (fSlopes.find(make_pair(vars[0], vars[1])) != fSlopes.end()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t repeated Slope variables, ignore it. \n";
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
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid value for xlow cut\n";
          return false;
        }
        cut.stability = val;
      }
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid value for yhigh cut\n";
          return false;
        }
        cut.high = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid value for correlation ylow cut\n";
          return false;
        }
        cut.low = val;
      }
    case 1:
      vars = Split(fields[0], ':');
      break;
    default:
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t At most 2 fields per correlation line\n";
      return false;
  }

  if (vars.size() != 2) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t wrong variables (should be: varA:varB) for correlation\n";
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
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t empty variable for correlation\n";
    return false;
  }
  if (fCors.find(make_pair(vars[0], vars[1])) != fCors.end()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t repeated correlation variables, ignore it. \n";
    return false;
  }

  fCors.insert(make_pair(vars[0], vars[1]));
  if (plot) fCorPlots.push_back(make_pair(vars[0], vars[1]));
  fCorCuts[make_pair(vars[0], vars[1])] = cut;
  fVars.insert(Split(vars[0], '.')[0]);
  fVars.insert(Split(vars[1], '.')[0]);
  return true;
}

bool TConfig::ParseOtherCommands(char *line) {
  if (IsEmpty(line)) {
    return true;
  }
  vector<char *> fields = Split(line);
  if (fields.size() == 1) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t no value specified for command: " << fields[0] << endl;
    return false;
  }
  StripSpaces(fields[0]);
  StripSpaces(fields[1]);
  const char * command = fields[0];
  const char * value   = fields[1];
  if (strcmp(command, "@postpan") == 0) {
    if (strcmp(value, "true") == 0) 
      ft = "postpan";
  } else if (strcmp(command, "@japan") == 0) {
    if (strcmp(value, "true") == 0) 
      ft = "japan";
  } else if (strcmp(command, "@dithering") == 0) {
    if (strcmp(value, "true") == 0) 
      ft = "dithering";
  } else if (strcmp(command, "@dir") == 0)
    dir = value;
  else if (strcmp(command, "@tree") == 0)
    tree = value;
  else if (strcmp(command, "@japan_pass") == 0)
    japan_pass = value;
  else if (strcmp(command, "@dit_bpm") == 0)
    dit_bpm = value;
  else if (strcmp(command, "@logy") == 0) {
    if (strcmp(value, "true") == 0) 
      logy = true;
  } else {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t Unknow commands: " << command << endl;
    return false;
  }

  return true;
}

double TConfig::ExtractValue(Node * node) { 
  if (node == NULL) {
    cerr << __PRETTY_FUNCTION__ << ":ERROR\t NULL node\n";
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
        cerr << __PRETTY_FUNCTION__ << ":ERROR\t unknow unit: " << val << endl;
        return -9999;
      }
      if (Contain(val, "pp"))
        return UNITS[val] / ppm;  // normalized to ppm
      else 
        return UNITS[val] / um;   // normalized to um
    default:
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t unknow token type: " << TypeName[node->token.type] << endl;
      return -9999;
  }
	return -9999;
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
