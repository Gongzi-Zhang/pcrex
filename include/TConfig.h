#ifndef TCONFIG_H
#define TCONFIG_H

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>

#include "math_eval.h"

using namespace std;

typedef struct { double low, high, burplevel; } VarCut;
typedef VarCut CompCut;
typedef VarCut CorCut;

set<int> ParseRS(const char *line);
class TConfig {

  // ClassDef(TConfig, 0)  // config file parser

public:
  TConfig();
  TConfig(const char *conf_file, const char *RSlist=NULL);
  virtual ~TConfig();
  const char*	GetConfFile()	  const {return fConfFile;}
  const char*	GetDir()	      const {return dir;}
  const char* GetPattern()    const {return pattern;}
  const char*	GetTreeName()	  const {return tree;}
  const char* GetTreeCut()    const {return tcut;}
  bool		    GetLogy()	      const {return logy;}
  set<int>	  GetRS()					const {return fRS;}			// for ChectStat
  set<string> GetVars()	      const {return fVars;}
  map<string, string> GetVarAlts()  const {return fVarAlts;}
	map<string, const char*> GetFriendTrees()		const {return ftrees;}
  vector<pair<long, long>> GetEntryCut()			const {return ecuts;}
	vector<const char *>		 GetHighlightCut()	const {return hcuts;}

  vector<string>					 GetSolos()	    const {return fSolos;}
  map<string, VarCut>      GetSoloCut()		const {return fSoloCut;}

	// custom variables: 
  vector<string>					 GetCustoms()   const {return fCustoms;}	
  map<string, VarCut>			 GetCustomCut()	const {return fCustomCut;}
	map<string, Node *>			 GetCustomDef()	const {return fCustomDef;}

	// slope: for CheckStat
  vector<pair<string, string>>			GetSlopes()			const {return fSlopes;}	
  map<pair<string, string>, VarCut> GetSlopeCut()		const {return fSlopeCut;}

  vector<pair<string, string>>				GetComps()			const {return fComps;}
  map<pair<string, string>, VarCut>		GetCompCut()		const {return fCompCut;}

  vector<pair<string, string>>			GetCors()			const {return fCors;}
  map<pair<string, string>, VarCut> GetCorCut()		const {return fCorCut;}

	void SetRS(const char *rs_list) {fRSfile = rs_list;}

  bool ParseSolo(char *line);
  bool ParseComp(char *line);
  bool ParseSlope(char *line);
  bool ParseCor(char *line);
  bool ParseCustom(char *line);
  bool ParseEntryCut(char *line);
  bool ParseOtherCommands(char *line);
  void ParseConfFile();
  void ParseRSfile();
  double ExtractValue(Node *node); // parse mathematical expression to extract value

private:
  const char *fConfFile;
  const char *fRSfile;	// run/slug list file
  const char *dir     = NULL;
  const char *pattern = NULL;
  const char *tree    = NULL;
  const char *tcut    = NULL; // tree cut
  vector<pair<long, long>> ecuts;		// cut on entry number
	vector<const char *> hcuts;				// highlighted cuts
  map<string, const char *> ftrees; // friend trees
  
  bool logy = false;
  set<int> fRS;					// all runs/slugs that are going to be checked
  set<string> fVars;    // all variables that are going to be checked.
			                  // this one diffs from fSolos because some variables 
			                  // appears in fComps or fCors but not in fSolos
  map<string, string> fVarAlts;
  vector<string>      fSolos;
  map<string, VarCut>	fSoloCut;

	vector<string>			fCustoms;
  map<string, VarCut> fCustomCut;
	map<string, Node *>	fCustomDef;	// custom variables' def

  vector<pair<string, string>>			fComps;
  map<pair<string, string>, VarCut>	fCompCut;

  vector<pair<string, string>>			fSlopes;
  map<pair<string, string>, VarCut>	fSlopeCut;

  vector<pair<string, string>>			fCors;
  map<pair<string, string>, VarCut>	fCorCut;
};
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
