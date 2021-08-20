#include <iostream>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <utility>

#include "line.h"
#include "math_eval.h"
#include "TConfig.h"

using namespace std;
extern map<string, const double> UNITS;
// map<string, const double> UNITS;

// ClassImp(TConfig);

TConfig::TConfig() : fConfFile(0) {}
TConfig::TConfig(const char *conf_file, const char *RSlist) :
  fConfFile(conf_file),
  fRSfile(RSlist)
{}

TConfig::~TConfig() {
  cerr << INFO << "End of TConfig" << ENDL;
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
   *  1: Runs/Slugs
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
      if (strcmp(current_line, "@runs") == 0 || strcmp(current_line, "@slugs") == 0)	
				// FIXME: detect collision between @runs and @slugs, they can't appear at the same time
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
        cerr << WARNING << "Can't parse line " << nline << "." << ENDL;
        exit(4);
      }
      continue;
    }

    bool parsed = false;
    if (session & 1) {  // runs/slugs
      for (int i : ParseRS(current_line)) {
				fRS.insert(i);
				parsed = true;
			}
    } 
		else if (session & 2) 
      parsed = ParseSolo(current_line);
    else if (session & 4) 
      parsed = ParseComp(current_line);
    else if (session & 8) 
      parsed = ParseSlope(current_line);
    else if (session & 16) 
      parsed = ParseCor(current_line);
    else if (session & 32) 
      parsed = ParseCustom(current_line);
    else if (session & 64)
      parsed = ParseEntryCut(current_line);
    else 
		{
      cerr << WARNING << "unknown session, ignore line " << nline << ENDL;
      continue;
    }

    if (!parsed)
		{
      cerr << ERROR << "line " << nline << " can't be parsed correclty." << ENDL;
			exit(44);
		}
  }
  ifs.close();

  if (fRSfile)
    ParseRSfile();

  cout << INFO << "Configuration in config file: " << fConfFile << ENDL;
  if (fRS.size() > 0)				cout << "\t" << fRS.size() << " Runs/Slugs\n";
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
    cout << "\t" << "read tree: " << tree << endl; if (ecuts.size()) {
    cout << "\t" << "entry cuts: (-1 means the end entry)" << endl;
    for (pair<long, long> cut : ecuts)
      cout << "\t\t" << cut.first << "\t" << cut.second << endl;
  }
	if (hcuts.size() > 0) {
		cout << "\t" << "highlight cuts:" << endl;
		for (const char *hcut : hcuts) {
			cout << "\t\t" << hcut << endl;
		}
	}
}

void TConfig::ParseRSfile() {
  if (!fRSfile) {
    cerr << WARNING << "No list file specified." << ENDL;
    return;
  }

  ifstream ifs(fRSfile);
  if (! ifs.is_open()) {
    cerr << FATAL << "list file " << fRSfile << " doesn't exist or can't be read." << ENDL;
    exit(2);
  }

  char line[MAX];
  int  nline = 0;
  while (ifs.getline(line, MAX)) {
    nline++;

    StripComment(line);
    StripSpaces(line);
    if (line[0] == '\0') continue;

    for (int i:ParseRS(line))
			fRS.insert(i);
  }
}

bool TConfig::ParseVar(VAR &var, const char * str)
{
	if (!str)
		return false;

	var = {NULL, NULL};
	int end_pos = strlen(str)-1;
	int i;
  if ((i=Index(str, "+-")) >= 0)
	{
		var.verr = StripSpaces(Sub(str, i+2, end_pos-i-1));
		end_pos = i-1;
	}
	var.vname = StripSpaces(Sub(str, 0, end_pos+1));
	return true;
}

bool TConfig::SetCut(VarCut &cut, vector<char *> fields) 
{
	cut = {1024, 1024, 1024};
  switch (fields.size()) {
    case 3:
      StripSpaces(fields[2]);
      if (!IsEmpty(fields[2])) {
        double val = ExtractValue(ParseExpression(fields[2]));
        if (val == -9999) {
          cerr << ERROR << "Invalid value for burplevel cut" << ENDL;
          return false;
        }
        cut.burplevel = val;
      }
    case 2:
      StripSpaces(fields[1]);
      if (!IsEmpty(fields[1])) {
        double val = ExtractValue(ParseExpression(fields[1]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for upper limit cut" << ENDL;
          return false;
        }
        cut.high = val;
      }
    case 1:
      StripSpaces(fields[0]);
      if (!IsEmpty(fields[0])) {
        double val = ExtractValue(ParseExpression(fields[0]));
        if (val == -9999) {
          cerr << ERROR << "Non-number value for lower limit cut" << ENDL;
          return false;
        }
        cut.low = val;
      }
			break;
		case 0:
			return true;
    default:
      cerr << ERROR << "At most 3 fields for cut!" << ENDL;
      return false;
  }

	if (cut.high != 1024 && cut.low != 1024 && cut.low > cut.high)
	{
		cerr << ERROR << "Upper limit cut must be larger than low limit cut" << ENDL;
		return false;
	}

	return true;
}

bool TConfig::ParseSolo(char *line) {
  char *var=NULL;
  VarCut cut;

	VAR var_tmp;

  vector<char*> fields = Split(line, ';');
	if (fields.size() > nFIELDS)
	{
      cerr << ERROR << "At most 4 fields per solo line!" << ENDL;
      return false;
  }
	char * title = NULL;
	int i;
	if ((i=Index(fields[0], '|')) >= 0)
	{
		title = StripSpaces(Sub(fields[0], i+1));
		fields[0][i] = '\0';
	}
	if (!ParseVar(var_tmp, fields[0]))
	{
		cerr << ERROR << "Can't parse var declaration" << ENDL;
		return false;
	}
	fields.erase(fields.begin());
	if (!SetCut(cut, fields))
		return false;

	var = var_tmp.vname;
  if (!var || IsEmpty(var)) {
    cerr << ERROR << "Empty variable. " << ENDL;
    return false;
  }
  if (find(fSolos.cbegin(), fSolos.cend(), var) != fSolos.cend()) {
    cerr << WARNING << "repeated solo variable, ignore it. " << ENDL;
    return false;
  }

  fSolos.push_back(var);
	if (var_tmp.verr && !IsEmpty(var_tmp.verr))
	{
		fVarErrs[var] = var_tmp.verr;	// FIXME: what if variable has different error from other session
		fVars.insert(var_tmp.verr);
	}
	if (title && !IsEmpty(title))
		fVarTitles[var] = title;
  fSoloCut[var] = cut;
  fVars.insert(var);
  return true;
}

bool TConfig::ParseCustom(char *line) {
  char *var;
  char *def;
  VarCut cut;

	VAR var_tmp;

  vector<char*> fields = Split(line, ';');
	if (fields.size() > nFIELDS)
	{
      cerr << ERROR << "At most 4 fields per solo line!" << ENDL;
      return false;
  }
	char * title = NULL;
	int i;
	if ((i=Index(fields[0], '|')) >= 0)
	{
		title = StripSpaces(Sub(fields[0], i+1));
		fields[0][i] = '\0';
	}
	if (!ParseVar(var_tmp, fields[0]))
	{
		cerr << ERROR << "Can't parse var declaration" << ENDL;
		return false;
	}
	fields.erase(fields.begin());
	if (!SetCut(cut, fields))
		return false;

	vector<char *> vfields = Split(var_tmp.vname, ':');
	if (vfields.size() != 2) {
		cerr << ERROR << "Wrong format in defining custom variable (var: definition): " << fields[0] << ENDL;
		return false;
	}
	var = vfields[0];
	def = vfields[1];

  if (IsEmpty(var)) {
    cerr << ERROR << "Empty variable for custom. " << ENDL;
    return false;
  }
  if (find(fCustoms.cbegin(), fCustoms.cend(), var) != fCustoms.cend()) {
    cerr << WARNING << "repeated custom variable, ignore it. " << ENDL;
    return false;
  }

  Node *node = ParseExpression(def);
  if (node == NULL) {
    cerr << ERROR << "unable to parse definition. \n"
				 << "\t" << def << ENDL;
		return false;
	}

  fCustoms.push_back(var);
	for (string v : GetVariables(node)) 
		fVars.insert(v);
	if (var_tmp.verr && !IsEmpty(var_tmp.verr))
	{
		fVarErrs[var] = var_tmp.verr;
		fVars.insert(var_tmp.verr);
	}
	if (title && !IsEmpty(title))
		fVarTitles[var] = title;
  fCustomCut[var] = cut;
	fCustomDef[var] = node;
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
      if (line[strlen(line) - 1] == ':')  // xxxx:
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


bool TConfig::ParseComp(char *line) {
	char *var1, *var2;
  VarCut cut;

	VAR var_tmp1, var_tmp2;

  vector<char*> fields = Split(line, ';');
	if (fields.size() > nFIELDS)
	{
      cerr << ERROR << "At most 4 fields per solo line!" << ENDL;
      return false;
  }
	char * title = NULL;
	int i;
	if ((i=Index(fields[0], '|')) >= 0)
	{
		title = StripSpaces(Sub(fields[0], i+1));
		fields[0][i] = '\0';
	}
  vector<char*> vars = Split(fields[0], ',');
  if (vars.size() != 2) {
    cerr << ERROR << "wrong variables (should be: varA,varB) for comparison" << ENDL;
    return false;
  }
	fields.erase(fields.begin());
	if (!SetCut(cut, fields))
		return false;

	if (!ParseVar(var_tmp1, vars[0]) || !ParseVar(var_tmp2, vars[1]))
	{
		cerr << ERROR << "Can't parse var declaration" << ENDL;
		return false;
	}
	var1 = var_tmp1.vname;
	var2 = var_tmp2.vname;
  if (IsEmpty(var1) || IsEmpty(var2)) {
    cerr << ERROR << "empty variable for comparison" << ENDL;
    return false;
  }
  if (find(fComps.cbegin(), fComps.cend(), make_pair(string(var1), string(var2))) != fComps.cend()) {
    cerr << WARNING << "repeated comparison variables, ignore it. " << ENDL;
    return false;
  }

  fComps.push_back(make_pair(var1, var2));
  fCompCut[make_pair(var1, var2)] = cut;
  if (find(fCustoms.cbegin(), fCustoms.cend(), var1) == fCustoms.cend())
    fVars.insert(var1);
  if (find(fCustoms.cbegin(), fCustoms.cend(), var2) == fCustoms.cend())
    fVars.insert(var2);
	if (var_tmp1.verr && !IsEmpty(var_tmp1.verr))
	{
		fVarErrs[var1] = var_tmp1.verr;
		fVars.insert(var_tmp1.verr);
	}
	if (var_tmp2.verr && !IsEmpty(var_tmp2.verr))
	{
		fVarErrs[var2] = var_tmp2.verr;
		fVars.insert(var_tmp2.verr);
	}
	if (title && !IsEmpty(title))
		fVarTitles[string(var1) + string(var2)] = title;
  return true;
}

bool TConfig::ParseSlope(char *line) {
  vector<char*> fields = Split(line, ';');
  VarCut cut;
  vector<char*> vars;
	if (fields.size() > nFIELDS)
	{
      cerr << ERROR << "At most 4 fields per solo line!" << ENDL;
      return false;
  }
	vars = Split(fields[0], ':');
  if (vars.size() != 2) {
    cerr << ERROR << "wrong variables (should be: varA:varB) for slope" << ENDL;
    return false;
  }
	fields.erase(fields.begin());
	if (!SetCut(cut, fields))
		return false;

  StripSpaces(vars[0]);
  StripSpaces(vars[1]);
  if (IsEmpty(vars[0]) || IsEmpty(vars[1])) {
    cerr << ERROR << "empty variable for slope" << ENDL;
    return false;
  }
  if (find(fSlopes.cbegin(), fSlopes.cend(), make_pair(string(vars[0]), string(vars[1]))) != fSlopes.cend()) {
    cerr << WARNING << "repeated Slope variables, ignore it. " << ENDL;
    return false;
  }

  fSlopes.push_back(make_pair(vars[0], vars[1]));
  fSlopeCut[make_pair(vars[0], vars[1])] = cut;
  return true;
}

bool TConfig::ParseCor(char *line) {
	char *xvar, *yvar;
  VarCut cut;

	VAR xvar_tmp, yvar_tmp;

  vector<char*> fields = Split(line, ';');
	if (fields.size() > nFIELDS)
	{
      cerr << ERROR << "At most 4 fields per solo line!" << ENDL;
      return false;
  }
	char * title = NULL;
	int i;
	if ((i=Index(fields[0], '|')) >= 0)
	{
		title = StripSpaces(Sub(fields[0], i+1));
		fields[0][i] = '\0';
	}

  vector<char*> vars = Split(fields[0], ':');
  if (vars.size() != 2) {
    cerr << ERROR << "wrong variables (should be: varA:varB) for correlation" << ENDL;
    return false;
  }
	fields.erase(fields.begin());
	if (!SetCut(cut, fields))
		return false;

	if (!ParseVar(xvar_tmp, vars[0]) || !ParseVar(yvar_tmp, vars[1]))
	{
		cerr << ERROR << "Can't parse var declaration" << ENDL;
		return false;
	}
	xvar = xvar_tmp.vname;
	yvar = yvar_tmp.vname;
  if (IsEmpty(xvar) || IsEmpty(yvar)) {
    cerr << ERROR << "empty variable for correlation" << ENDL;
    return false;
  }
  if (find(fCors.cbegin(), fCors.cend(), make_pair(string(xvar), string(yvar))) != fCors.cend()) {
    cerr << WARNING << "repeated correlation variables, ignore it. " << ENDL;
    return false;
  }

  fCors.push_back(make_pair(xvar, yvar));
  fCorCut[make_pair(xvar, yvar)] = cut;
  if (find(fCustoms.cbegin(), fCustoms.cend(), xvar) == fCustoms.cend())
    fVars.insert(xvar);
  if (find(fCustoms.cbegin(), fCustoms.cend(), yvar) == fCustoms.cend())
    fVars.insert(yvar);
	if (xvar_tmp.verr && !IsEmpty(xvar_tmp.verr))
	{
		fVarErrs[xvar] = xvar_tmp.verr;
		fVars.insert(xvar_tmp.verr);
	}
	if (yvar_tmp.verr && !IsEmpty(yvar_tmp.verr))
	{
		fVarErrs[yvar] = yvar_tmp.verr;
		fVars.insert(yvar_tmp.verr);
	}
	if (title && !IsEmpty(title))
		fVarTitles[string(xvar) + string(yvar)] = title;
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

  const char * value = StripSpaces(Sub(line, i));
  if (value[0] == '\0') {
    cerr << WARNING << "no value specified for command: " << command << ENDL;
    return false;
  }

  if (strcmp(command, "@dir") == 0) 
    dir = value;
  else if (strcmp(command, "@pattern") == 0) 
    pattern = value;
  else if (strcmp(command, "@tree") == 0) 
    tree = value;
  else if (strcmp(command, "@cut") == 0) 
    tcut = value;
  else if (strcmp(command, "@logy") == 0) 
	{
    if (strcmp(value, "true") == 0) 
      logy = true;
  } 
	else if (strcmp(command, "@friendtree") == 0) 
	{
    int ind = Index(value, ';');
    const char * t = ind > 0 ? StripSpaces(Sub(value, 0, ind)) : value;
    const char * f = ind > 0 ? StripSpaces(Sub(value, ind+1)) : "";
    if (t[0] == '=') {
      cerr << WARNING << "Incorrect friend tree expression: " << t 
           << "Do you miss the alias before =" << ENDL;
      return false;
    }
    ftrees[t] = f;
	} 
	else if (strcmp(command, "@highlightcut") == 0) 
		hcuts.push_back(value);
  else 
	{
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
			return Getf1Value(val, v1);
    case function2:
			return Getf2Value(val, v1, ExtractValue(node->lchild->sibling));
    case number:
      return atof(val);
    case variable:
      if (UNITS.find(val) == UNITS.cend()) {
        cerr << ERROR << "unknow unit: " << val << ENDL;
        return -9999;
      }
      return UNITS[val];
    default:
      cerr << ERROR << "unknow token type: " << GetTypeName(node->token.type) << ENDL;
      return -9999;
  }
	return -9999;
}

set<int> ParseRS(const char *line) {
  if (!line) {
    cerr << ERROR << "empty input" << ENDL;
    return {};
  }

	set<int> vals;
  for (const char *seg : Split(line, ',')) {
    int start=-1, end=-1;
    vector<char*> fields = Split(seg, '-');
    switch (fields.size()) {
      case 2:
        if (!IsInteger(fields[1])) {
          cerr << WARNING << "invalid end value in range: " << line << ENDL;
          return {};
        }
        end = atoi(fields[1]);
      case 1:
        if (!IsInteger(fields[0])) {
          cerr << WARNING << "invalid (start) value in line: " << line << ENDL;
          return {};
        }
        start = atoi(fields[0]);
        if (end == -1)
          end = start;
    }

    for (int v=start; v<=end; v++) {
      vals.insert(v); 
    }
  }
  return vals;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
