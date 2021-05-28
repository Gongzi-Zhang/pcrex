#ifndef TBASE_H
#define TBASE_H

#include <vector>
#include <set>
#include <map>
#include "TLeaf.h"
#include "TCut.h"
#include "TConfig.h"

using namespace std;

class TBase 
{

    // ClassDe (TBase, 0) // mul plots

  protected:
		// Program program		= mulplot;
    // root file: $dir/$pattern
    const char *dir   = "/adaqfs/home/apar/PREX/prompt/results/";
    string pattern    = "prexPrompt_xxxx_???_regress_postpan.root";
    const char *tree  = "reg";
		TCut tcut;	// tree cut
		map<string, const char*> ftrees;	// friend trees: tree, file_name
    vector<pair<long, long>> ecuts;

		map<string, const char*> real_trees;	// real tree expression for alias

    // TConfig fConf;
    int	    nGrans;	// granularity: can be run, slug or pit
    int	    nVars;
    int	    nSolos;
		int		  nCustoms;
    int	    nComps;
    int	    nSlopes;
    int	    nCors;

    const char *granularity = "run";
    set<int>    fGrans;	// FIXME how to use reference here
    set<string> fVars;
    map<string, string> fVarErrs;
    map<string, string> fVarTitles;
		set<string> fCutVars;

    vector<string>      fSolos;
    map<string, VarCut> fSoloCut;	// use the low and high cut as x range

    vector<string>			fCustoms;
    map<string, VarCut>	fCustomCut;
		map<string, Node *>	fCustomDef;

    vector<pair<string, string>>      fComps;
    map<pair<string, string>, VarCut>	fCompCut;

    vector<pair<string, string>>      fSlopes;
    map<pair<string, string>, VarCut> fSlopeCut;

    vector<pair<string, string>>      fCors;
    map<pair<string, string>, VarCut> fCorCut;

    map<int, vector<string>> fRootFile;
    map<string, pair<string, string>> fVarName;
    map<string, vector<int>> fVarUseAlt;
    map<string, TLeaf *> fVarLeaf;
		long nTotal = 0;
		long nOk = 0;  // total number of ok events 
		map<int, map<int, vector<long>>> fEntryNumber; // entry number of good entry
		map<int, map<int, int>> fEntries;  // number of entries for each session of each granularity
    map<string, double>  vars_buf;	// temp. value storage
		map<string, vector<double>> fVarValue;	// real value storage;

		map<string, double> fVarSum;
		map<string, double> fVarSum2;
		map<string, double> fVarMax;
		map<string, double> fVarMin;

  public:
     TBase();
     ~TBase();
     virtual void GetConfig(const TConfig &fConf);
     void SetDir(const char *d);
		 void SetTreeCut(TCut cut) {tcut = cut;}
     void CheckGrans();
     pair<string, string> ParseVar(const string var);
     virtual void CheckVars();
     bool CheckVar(string var);
     bool CheckCustomVar(Node * node);
     virtual int GetValues();
     void GetCustomValues();
     bool CheckEntryCut(const long entry);

		 double get_custom_value(Node *node);
		 virtual const char *GetUnit(string var);

		 void PrintStatistics();	// auxiliary functions
};
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
