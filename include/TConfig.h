#ifndef TCONFIG_H
#define TCONFIG_H

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>

#include "math_eval.h"

#define	 nFIELDS 4

using namespace std;

typedef struct { char *vname, *verr; } VAR;
typedef struct { double low, high, burplevel; } VarCut;
typedef VarCut CompCut;
typedef VarCut CorCut;

set<int> ParseRS(const char *line);
set<int> ParseRSfile(const char *rs_file);

class TConfig {

  // ClassDef(TConfig, 0)  // config file parser

public:
  TConfig();
  TConfig(const char *conf_file);
  virtual ~TConfig();
  const char*	GetConfFile()	  const {return fConfigFile;}
	const char* GetScalar(string var) const
	{
		map<string, const char *>::const_iterator it = fScalarConfig.find(var);
		if (it == fScalarConfig.cend())
		{
			cerr << ERROR << "no such configuration in the config file: " << var << ENDL;
			return NULL;
		}
    return it->second;
	}
	vector<string> GetVector(string var) const
	{
		map<string, vector<string>>::const_iterator it = fVectorConfig.find(var);
		if (it == fVectorConfig.end())
		{
			cerr << ERROR << "no such configuration in the config file: " << var << ENDL;
			return {};
		}
    return it->second;
	}
  set<int>	  GetRS()					const {return fRS;}			// for ChectStat
  set<string> GetVars()	      const {return fVars;}
  map<string, string>			 GetVarErrs()				const {return fVarErrs;}
  map<string, string>			 GetVarTitles()			const {return fVarTitles;}
	map<string, const char*> GetFriendTrees()		const {return ftrees;}
  vector<pair<long, long>> GetEntryCut()			const {return ecuts;}
	vector<const char *>		 GetHighlightCut()	const {return hcuts;}

  vector<string>					 GetSolos()	    const {return fSolos;}
  map<string, VarCut>      GetSoloCut()		const {return fSoloCut;}

	// custom variables: 
  vector<string>					 GetCustoms()   const {return fCustoms;}	
  map<string, VarCut>			 GetCustomCut()	const {return fCustomCut;}
	map<string, Node *>			 GetCustomDef()	const {return fCustomDef;}

  vector<pair<string, string>>				GetComps()			const {return fComps;}
  map<pair<string, string>, VarCut>		GetCompCut()		const {return fCompCut;}

  vector<pair<string, string>>			GetCors()			const {return fCors;}
  map<pair<string, string>, VarCut> GetCorCut()		const {return fCorCut;}

  bool ParseSolo(char *line);
  bool ParseComp(char *line);
  bool ParseCor(char *line);
  bool ParseCustom(char *line);
  bool ParseEntryCut(char *line);
  bool ParseFriendTree(char *line);
  void ParseConfFile();
  void ParseRSfile();

	static bool SetCut(VarCut &cut, vector<char *>fields);
	static bool ParseVar(VAR &var, const char* str);
  static double ExtractValue(Node *node); // parse mathematical expression to extract value

private:
  const char *fConfigFile;
	map<string, const char *> fScalarConfig;
	map<string, vector<string>> fVectorConfig;
	string fCurrentSession;

  vector<pair<long, long>> ecuts;		// cut on entry number
	vector<const char *> hcuts;				// highlighted cuts
  map<string, const char *> ftrees; // friend trees
  
  set<int> fRS;					// all runs/slugs that are going to be checked
  set<string> fVars;    // all variables that are going to be checked.
			                  // this one diffs from fSolos because some variables 
			                  // appears in fComps or fCors but not in fSolos

  map<string, string> fVarErrs;
  map<string, string> fVarTitles;
  vector<string>      fSolos;
  map<string, VarCut>	fSoloCut;

	vector<string>			fCustoms;
  map<string, VarCut> fCustomCut;
	map<string, Node *>	fCustomDef;	// custom variables' def

  vector<pair<string, string>>			fComps;
  map<pair<string, string>, VarCut>	fCompCut;

  vector<pair<string, string>>			fCors;
  map<pair<string, string>, VarCut>	fCorCut;
};
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
