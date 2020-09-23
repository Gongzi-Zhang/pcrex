#ifndef TBASE_H
#define TBASE_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <glob.h>
#include <libgen.h>

#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TCut.h"
#include "TEntryList.h"
#include "TCollection.h"
#include "TGraphErrors.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TPaveStats.h"
#include "TList.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TColor.h"

#include "const.h"
#include "line.h"
#include "rcdb.h"
#include "TConfig.h"


enum Format {pdf, png};
enum Program {mulplot, checkstatistics, checkruns};

using namespace std;

class TBase {

    // ClassDe (TBase, 0) // mul plots

  protected:
		Program program		= mulplot;
    Format format     = pdf; // default pdf output
    const char *out_name = "out";
    // root file: $dir/$pattern
    const char *dir   = "/adaqfs/home/apar/PREX/prompt/results/";
    string pattern    = "prexPrompt_xxxx_???_regress_postpan.root";
    const char *tree  = "reg";
		const char *mCut	= "";	// main cut
    // bool logy         = false;
		map<string, const char*> ftrees;	// friend trees: tree, file_name
		map<string, const char*> real_trees;	// used trees
    vector<pair<long, long>> ecuts;
    vector<TCut> allCuts;	// including highlight cuts, only for TCheckRuns

    TConfig fConf;
    int     nSlugs;
    int	    nRuns;
    int	    nVars;
    int	    nSolos;
		int		  nCustoms;
    int	    nComps;
    int	    nSlopes;
    int	    nCors;

    set<int>    fRuns;
    set<int>    fSlugs;
    set<string> fVars;

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

    // double slopes_buf[ROWS][COLS];
    // double slopes_err_buf[ROWS][COLS];
    map<pair<string, string>, pair<int, int>>	  fSlopeIndex;
    int      rows, cols;
    double * slopes_buf;
    double * slopes_err_buf;

    map<int, vector<string>> fRootFile;
    map<string, pair<string, string>> fVarName;
    map<string, TLeaf *> fVarLeaf;
		long nTotal;
		map<string, long> nOk;  // total number of ok events for each cut
		map<string, vector<long>> fEntryNumber;		// for each cut
		map<int, int> fRunEntries;     // number of entries for each run
    map<int, int> fRunSign;
		map<int, int> fRunArm;
		map<string, const char *> fVarUnit;
    map<string, double>  vars_buf;	// temp. value storage
		map<string, map<string, vector<double>>> fVarValue;	// real value storage; for each cut
    map<string, map<pair<string, string>, vector<double>>> fSlopeValue;
    map<string, map<pair<string, string>, vector<double>>> fSlopeErr;

		// some statistics: not for every cut, only for the main Cut
		map<string, double> fVarSum;
		map<string, double> fVarSum2;
		map<string, double> fVarMax;
		map<string, double> fVarMin;

    TCanvas * c;

  public:
     TBase(const char* config_file, const char* run_list = NULL);
     ~TBase();
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutFormat(const char * f);
     void SetDir(const char * d);
     // void SetLogy(bool log) {logy = log;}
     void SetSlugs(set<int> slugs);
     void SetRuns(set<int> runs);
     void CheckRuns();
     void CheckVars();
     bool CheckVar(string var);
     bool CheckCustomVar(Node * node);
     void GetValues();
     bool CheckEntryCut(const long entry);

		 double get_custom_value(Node * node);
		 virtual const char * GetUnit(string var);
};

// ClassImp(TBase);

TBase::TBase(const char* config_file, const char* run_list) :
  fConf(config_file, run_list)
{
  fConf.ParseConfFile();
  fRuns = fConf.GetRuns();
  nRuns = fRuns.size();
  fVars	= fConf.GetVars();
  nVars = fVars.size();

  fSolos    = fConf.GetSolos();
  fSoloCut = fConf.GetSoloCut();
  nSolos    = fSolos.size();

  fCustoms    = fConf.GetCustoms();
  fCustomDef = fConf.GetCustomDef();
  fCustomCut = fConf.GetCustomCut();
  nCustoms    = fCustoms.size();

  fComps    = fConf.GetComps();
  fCompCut = fConf.GetCompCut();
  nComps    = fComps.size();

  fSlopes = fConf.GetSlopes();
  fSlopeCut  = fConf.GetSlopeCut();
  nSlopes = fSlopes.size();

  fCors	    = fConf.GetCors();
  fCorCut  = fConf.GetCorCut();
	nCors			= fCors.size();

  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetPattern())   pattern = fConf.GetPattern();
  if (fConf.GetTreeName())  tree    = fConf.GetTreeName();
  if (fConf.GetTreeCut())   mCut		= fConf.GetTreeCut();
	allCuts.push_back(mCut);	// mCut should always be the first cut of allCuts
	ftrees = fConf.GetFriendTrees();
  // logy  = fConf.GetLogy();
  ecuts = fConf.GetEntryCut();
	for(const char* c : fConf.GetHighlightCut())
		allCuts.push_back(c);

  gROOT->SetBatch(1);
}

TBase::~TBase() {
  cout << INFO << "End of TBase" << ENDL;
}

void TBase::SetDir(const char * d) {
  struct stat info;
  if (stat(d, &info) != 0) {
    cerr << FATAL << "can't access specified dir: " << dir << ENDL;
    exit(30);
  } else if ( !(info.st_mode & S_IFDIR)) {
    cerr << FATAL << "not a dir: " << dir << ENDL;
    exit(31);
  }
  dir = d;
}

void TBase::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << FATAL << "Unknow output format: " << f << ENDL;
    exit(40);
  }
}

void TBase::SetSlugs(set<int> slugs) {
  for(int slug : slugs) {
    if (   (CREX_AT_START_SLUG <= slug && slug <= CREX_AT_END_SLUG)
        || (PREX_AT_START_SLUG <= slug && slug <= PREX_AT_END_SLUG)
        || (        START_SLUG <= slug && slug <= END_SLUG) ) { 
      fSlugs.insert(slug);
    } else {
      cerr << ERROR << "Invalid slug number (" << START_SLUG << "-" << END_SLUG << "): " << slug << ENDL;
      continue;
    }
  }
  nSlugs = fSlugs.size();
}

void TBase::SetRuns(set<int> runs) {
  for(int run : runs) {
    if (run < START_RUN || run > END_RUN) {
      cerr << ERROR << "Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << ENDL;
      continue;
    }
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
}

void TBase::CheckRuns() {
  StartConnection();

  if (nSlugs > 0) {
    set<int> runs;
    for(int slug : fSlugs) {
      runs = GetRunsFromSlug(slug);
      for (int run : runs) {
        fRuns.insert(run);
      }
    }
  }
  nRuns = fRuns.size();

  // check runs against database
  GetValidRuns(fRuns);
  nRuns = fRuns.size();

  for (set<int>::const_iterator it = fRuns.cbegin(); it != fRuns.cend(); ) {
    int run = *it;
    string p_buf(pattern);
    p_buf.replace(p_buf.find("xxxx"), 4, to_string(run));
    const char * p = Form("%s/%s", dir, p_buf.c_str());

    glob_t globbuf;
    glob(p, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << ALERT << "no root file for run " << run << ". Ignore it." << ENDL;
      it = fRuns.erase(it);
      continue;
    }
    for (int i=0; i<globbuf.gl_pathc; i++)
      fRootFile[run].push_back(globbuf.gl_pathv[i]);

    globfree(&globbuf);
    it++;
  }

  nRuns = fRuns.size();
  if (nRuns == 0) {
    cerr << FATAL << "No valid runs specified!" << ENDL;
    EndConnection();
    exit(10);
  }

  cout << INFO << "" << nRuns << " valid runs specified:" << ENDL;
  for(int run : fRuns) {
    cout << "\t" << run << endl;
  }

	for (int run : fRuns) {
		fRunSign[run] = GetRunSign(run);
		fRunArm[run]	= GetRunArmFlag(run);
	}
  EndConnection();
}

void TBase::CheckVars() {
	// add necessary variables
	// for main avg/dd variables, if global armflag is all, there must be corresponding left/right arm data
	// FIXME: what if us_avg_ds_avg_dd
	if (garmflag == allarms) {
		for (string var : fVars) {
			if ( var.find("us_avg") != string::npos 
				|| var.find("ds_avg") != string::npos 
				|| var.find("us_dd") != string::npos 
			  || var.find("ds_dd")  != string::npos ) {
				string lvar = var;
				string rvar = var;
				if ( var.find("_avg") != string::npos ) {
					fVars.insert(lvar.replace(lvar.find("_avg"), 4, "l"));
					fVars.insert(rvar.replace(rvar.find("_avg"), 4, "r"));
				} else {
					fVars.insert(lvar.replace(lvar.find("_dd"), 3, "l"));
					fVars.insert(rvar.replace(rvar.find("_dd"), 3, "r"));
				}
			}
		}
	}

  srand(time(NULL));
  int s = rand() % nRuns;
  set<int>::const_iterator it_r=fRuns.cbegin();
  for(int i=0; i<s; i++)
    it_r++;

  while (it_r != fRuns.cend()) {
		set<string> tmp_vars;
		set<string> used_ftrees;

    int run = *it_r;
    const char * file_name = fRootFile[run][0].c_str();
    TFile * f_rootfile = new TFile(file_name, "read");
    if (!f_rootfile->IsOpen()) {
      cerr << WARNING << "run-" << run << " ^^^^ can't read root file: " << file_name 
           << ", skip this run." << ENDL;
      goto next_run;
    }
  
		// slope
    vector<TString> *l_iv, *l_dv;
		l_iv = (vector<TString>*) f_rootfile->Get("IVNames");
		l_dv = (vector<TString>*) f_rootfile->Get("DVNames");
    if (nSlopes > 0 && (l_iv == NULL || l_dv == NULL)){
      cerr << WARNING << "run-" << run << " ^^^^ can't read IVNames or DVNames in root file: " 
           << file_name << ", skip this run." << ENDL;
      goto next_run;
    }

    TTree * tin;
		tin = (TTree*) f_rootfile->Get(tree);
    if (tin == NULL) {
      cerr << WARNING << "run-" << run << " ^^^^ can't read tree: " << tree << " in root file: "
           << file_name << ", skip this run." << ENDL;
      goto next_run;
    }

    for (auto const ftree : ftrees) {
      const char *texp = ftree.first.c_str();
      string file_name = ftree.second;
			if (file_name.find("xxxx") != string::npos)
				file_name.replace(file_name.find("xxxx"), 4, to_string(run));
      if (file_name.size()) {
        glob_t globbuf;
        glob(file_name.c_str(), 0, NULL, &globbuf);
        if (globbuf.gl_pathc == 0) {
          cerr << WARNING << "run-" << run << " ^^^^ can't read friend tree: " << texp
							 << " in root file: " << file_name << ", skip this run." << ENDL; 
					goto next_run;
				}
				file_name = globbuf.gl_pathv[0];
				globfree(&globbuf);
      }
      tin->AddFriend(texp, file_name.c_str());	// FIXME: what if the friend tree doesn't exist
      int pos = Index(texp, '=');
      string alias = pos > 0 ? StripSpaces(Sub(texp, 0, pos)) : texp;
      // const char *old_name = pos > 0 ? StripSpaces(Sub(texp, pos+1)) : texp;
			if (real_trees.find(alias) != real_trees.end()) {
				cout << WARNING << "repeated tree definition\n"
						 << "\told: " << real_trees[alias] << "\n"
						 << "\tnew: " << texp << ENDL;
			}
      real_trees[alias] = texp;
      // it is user's responsibility to make sure each tree has an unique name
    }

    cout << INFO << "use the following root files to check vars: \n";
		cout << "\t" << file_name;
		for (auto const ftree : ftrees) {
			if (ftree.second[0])
				cout << "\n\t" << ftree.second;
		}
		cout << ENDL;

		// check variables in cut
		tmp_vars.clear();
		for (string var : fVars)
			tmp_vars.insert(var);

		{	// this brace is needed for compilation because I use goto
		 	// so that I can limit new defined node in this local scope
			for (const char * cut : allCuts) {	// check all cuts
				Node * node = ParseExpression(cut);
				for (string var : GetVariables(node))
					tmp_vars.insert(var);
				DeleteNode(node);
			}
		}

		for (string var : tmp_vars) {
      size_t n = count(var.begin(), var.end(), '.');
      string branch, leaf;
      if (n==0) {
        branch = var;
      } else if (n==1) {
        size_t pos = var.find_first_of('.');
        if (real_trees.find(var.substr(0, pos)) != real_trees.end()) {
					// FIXME: is it possible something like: tree.leaf ??? without branch name ???
					// ignore it right now
          branch = var;
        } else {
          branch = var.substr(0, pos);
          leaf = var.substr(pos+1);
        }
      } else if (n==2) {
        size_t pos = var.find_last_of('.');
        branch = var.substr(0, pos);
        leaf = var.substr(pos+1);
      } else {
        cerr << ERROR << "Invalid variable expression: " << var << endl;
        exit(24);
      }

      TBranch * bbuf = tin->GetBranch(branch.c_str());
			if (!bbuf) {
				if (Count(var.c_str(), '.') == 0) {	// try leaf directly
					TLeaf *l = tin->GetLeaf(branch.c_str());	
					if (l != NULL) {
						leaf = branch;
						bbuf = l->GetBranch();
						branch = bbuf->GetName();
					}
				}
				// special branches -- stupid
				if (branch.find("diff_bpm11X") != string::npos && run < 3390) {		// lagrange tree
					// no bpm11X in early runs, replace with bpm12X
					cout << WARNING << "run-" << run << " ^^^^ No branch diff_bpm11X, replace with 0.6*diff_bpm12X" << ENDL;
					string b = branch;
					bbuf = tin->GetBranch(b.replace(b.find("bpm11X"), 6, "bpm12X").c_str());
				} else if (branch.find("diff_bpmE") != string::npos && run < 3390) {		// reg tree
					cout << WARNING << "run-" << run << " ^^^^ No branch diff_bpmE, replace with diff_bpm12X" << ENDL;
					string b = branch;
					bbuf = tin->GetBranch(b.replace(b.find("bpmE"), 4, "bpm12X").c_str());
				} 

				if (!bbuf) {
					cerr << ERROR << "no branch (leaf): " << branch << " as in var: " << var << ENDL;
					tin->Delete();
					f_rootfile->Close();
					exit(24);
				}
			}

			TObjArray * l_leaf = bbuf->GetListOfLeaves();
			if (leaf.size() == 0) {
				leaf = l_leaf->At(0)->GetName();  // use the first leaf
			}
			TLeaf * lbuf = (TLeaf *) l_leaf->FindObject(leaf.c_str());
			if (!lbuf) {
				cerr << WARNING << "No such leaf: " << leaf << " in var: " << var << ENDL;
				cout << DEBUG << "List of valid leaves:" << ENDL;
				TIter next(l_leaf);
				TLeaf *l;
				while (l = (TLeaf*) next()) {
					cout << "\t" << l->GetName() << endl;
				}
				tin->Delete();
				f_rootfile->Close();
				exit(24);
			}
			// special var -- stupid
			if (branch.find("diff_bpm11X") != string::npos && run < 3390 && leaf.find("diff_bpm12X") != string::npos) {		// lagrange tree
				leaf = "diff_bpm11X";
			} else if (branch.find("diff_bpmE") != string::npos && run < 3390	&& leaf.find("diff_bpm12X") != string::npos) {	// reg tree		
				leaf = "diff_bpmE";
			} 

			fVarName[var] = make_pair(branch, leaf);
			if (branch.find('.') != string::npos) {
				used_ftrees.insert(StripSpaces(Sub(branch.c_str(), 0, branch.find('.'))));
			} else {
				used_ftrees.insert(bbuf->GetTree()->GetName());
			}
		}
		tmp_vars.clear();

		if (used_ftrees.find(tree) == used_ftrees.end()) {
			cerr << WARNING << "unsed main tree: " << tree << ENDL;
		} else {
			used_ftrees.erase(used_ftrees.find(tree));
		}
		for (map<string, const char*>::const_iterator it=ftrees.cbegin(); it!=ftrees.cend(); ) {
			bool used = false;
			for (auto const uftree : used_ftrees) {
				if (real_trees[uftree] == it->first) {
					used = true;
					break;
				}
			}
			if (used)
				it++;
			else {
				cerr << WARNING << "unused friend tree: " << it->first << ". remove it." << ENDL;
				it = ftrees.erase(it);
			}
		}

		if (nSlopes>0) {
			rows = l_dv->size();
			cols = l_iv->size();
			slopes_buf = new double[rows*cols];
			slopes_err_buf = new double[rows*cols];
			// if (rows != ROWS || cols != COLS) {
			//   cerr << FATAL << "Unmatched slope array size: " << rows << "x" << cols << " in run: " << run << ENDL;
			//   exit(20);
			// }
			bool error_dv_flag = false;
			bool error_iv_flag = false;
			for (vector<pair<string, string>>::iterator it=fSlopes.begin(); it != fSlopes.end(); ) {
				string dv = it->first;
				string iv = it->second;
				vector<TString>::const_iterator it_dv = find(l_dv->cbegin(), l_dv->cend(), dv);
				vector<TString>::const_iterator it_iv = find(l_iv->cbegin(), l_iv->cend(), iv);
				if (it_dv == l_dv->cend() || it_iv == l_iv->cend()) {
					if (it_dv == l_dv->cend()) {
						cerr << WARNING << "Invalid dv name for slope: " << dv << ENDL;
						error_dv_flag = true;
					}
					if (it_iv == l_iv->cend()) {
						cerr << WARNING << "Invalid iv name for slope: " << iv << ENDL;
						error_iv_flag = true;
					}
					map<pair<string, string>, VarCut>::const_iterator it_c = fSlopeCut.find(*it);
					if (it_c != fSlopeCut.cend())
						fSlopeCut.erase(it_c);
					it = fSlopes.erase(it);
					continue;
				}
				fSlopeIndex[*it] = make_pair(it_dv-l_dv->cbegin(), it_iv-l_iv->cbegin());
				it++;
			}
			if (error_dv_flag) {
				cout << DEBUG << "List of valid dv names:" << ENDL;
				for (vector<TString>::const_iterator it = l_dv->cbegin(); it != l_dv->cend(); it++) 
					cout << "\t" << (*it).Data() << endl;
			}
			if (error_iv_flag) {
				cout << DEBUG << "List of valid dv names:" << ENDL;
				for (vector<TString>::const_iterator it = l_iv->cbegin(); it != l_iv->cend(); it++) 
					cout << "\t" << (*it).Data() << endl;
			}
		}
		tin->Delete();
		f_rootfile->Close();
		break;
      
next_run:
		f_rootfile->Close();
		if (tin) {
			tin->Delete();
			tin = NULL;
		}
    it_r = fRuns.erase(it_r);

    if (it_r == fRuns.cend())
      it_r = fRuns.cbegin();
  }

  nRuns = fRuns.size();
  if (nRuns == 0) {
    cerr << FATAL << "no valid runs, aborting." << ENDL;
    exit(10);
  }

  nVars = fVars.size();
  nSlopes = fSlopes.size();
  if (nVars == 0 && nSlopes == 0) {
    cerr << FATAL << "no valid variables specified, aborting." << ENDL;
    exit(11);
  }

  for (vector<string>::iterator it=fSolos.begin(); it!=fSolos.end();) {
    if (fVars.find(*it) == fVars.cend()) {
			map<string, VarCut>::const_iterator it_c = fSoloCut.find(*it);
			if (it_c != fSoloCut.cend())
				fSoloCut.erase(it_c);

			cerr << WARNING << "Invalid solo variable: " << *it << ENDL;
      it = fSolos.erase(it);
		} else
      it++;
  }

	for (vector<string>::iterator it=fCustoms.begin(); it!=fCustoms.end(); ) {
		if (!CheckCustomVar(fCustomDef[*it])) {
			map<string, VarCut>::const_iterator it_c = fCustomCut.find(*it);
			if (it_c != fCustomCut.cend())
				fCustomCut.erase(it_c);

			cerr << WARNING << "Invalid custom variable: " << *it << ENDL;
      it = fCustoms.erase(it);
		} else
      it++;
	}

  for (vector<pair<string, string>>::iterator it=fComps.begin(); it!=fComps.end(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
				// || strcmp(GetUnit(it->first), GetUnit(it->second)) != 0 ) {
			map<pair<string, string>, VarCut>::const_iterator it_c = fCompCut.find(*it);
			if (it_c != fCompCut.cend())
				fCompCut.erase(it_c);

			cerr << WARNING << "Invalid Comp variable: " << it->first << "\t" << it->second << ENDL;
      it = fComps.erase(it);
		} else 
			it++;
  }

  for (vector<pair<string, string>>::iterator it=fCors.begin(); it!=fCors.end(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
			map<pair<string, string>, VarCut>::const_iterator it_c = fCorCut.find(*it);
			if (it_c != fCorCut.cend())
				fCorCut.erase(it_c);

			cerr << WARNING << "Invalid Cor variable: " << it->first << "\t" << it->second << ENDL;
      it = fCors.erase(it);
		} else
      it++;
  }

  nSolos = fSolos.size();
  nComps = fComps.size();
  nCors  = fCors.size();
  nCustoms = fCustoms.size();

  cout << INFO << "" << nSolos << " valid solo variables specified:" << ENDL;
  for(string solo : fSolos) {
    cout << "\t" << solo << endl;
  }
  cout << INFO << "" << nComps << " valid comparisons specified:" << ENDL;
  for(pair<string, string> comp : fComps) {
    cout << "\t" << comp.first << " , " << comp.second << endl;
  }
  cout << INFO << "" << nSlopes << " valid slopes specified:" << ENDL;
  for(pair<string, string> slope : fSlopes) {
    cout << "\t" << slope.first << " : " << slope.second << endl;
  }
  cout << INFO << "" << nCors << " valid correlations specified:" << ENDL;
  for(pair<string, string> cor : fCors) {
    cout << "\t" << cor.first << " : " << cor.second << endl;
  }
  cout << INFO << "" << nCustoms << " valid customs specified:" << ENDL;
  for(string custom : fCustoms) {
    cout << "\t" << custom << endl;
  }
}

bool TBase::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend() && find(fCustoms.cbegin(), fCustoms.cend(), var) == fCustoms.cend()) {
    cerr << WARNING << "Unknown variable: " << var << ENDL;
    return false;
  }
  return true;
}

bool TBase::CheckCustomVar(Node * node) {
  if (node) {
		if (	 node->token.type == variable 
				&& find(fCustoms.cbegin(), fCustoms.cend(), node->token.value) == fCustoms.cend()
				&& fVars.find(node->token.value) == fVars.cend() ) {
			cerr << WARNING << "Unknown variable: " << node->token.value << ENDL;
			return false;
		}

		bool l = CheckCustomVar(node->lchild);
		bool r = CheckCustomVar(node->rchild);
		bool s = CheckCustomVar(node->sibling);
		return l && r && s;
	}
	return true;
}

/* GetValues consideration:
 * 1. auxiliary cut: e.g. cut on number of entries (good patterns < 4500)
 * 4. unit
 * 5. special variables: bpm11X
 * 7. provide some statistical features of the values
 */
void TBase::GetValues() {

	map<string, double> unit;
	// initialization
  for (string var : fVars) {
    fVarMax[var] = 0;
		unit[var] = 1;
		if (var.find("diff") != string::npos)
			unit[var] = mm;
  }
  for (string custom : fCustoms)
		fVarMax[custom] = 0;

	nTotal = 0;
  for (int run : fRuns) {
    const size_t sessions = fRootFile[run].size();
    for (size_t session=0; session < sessions; session++) {
      const char *file_name = fRootFile[run][session].c_str();
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << ERROR << "run-" << run << " ^^^^ Can't open root file: " << file_name << ENDL;
        continue;
      }

      cout << INFO << Form("Read run: %d, session: %03d\t", run, session)
           << file_name << ENDL;
      TTree * tin = (TTree*) f_rootfile.Get(tree);
      if (! tin) {
        cerr << ERROR << "No such tree: " << tree << " in root file: " << file_name << ENDL;
				f_rootfile.Close();
        continue;
      }

			for (auto const ftree : ftrees) {
				string file_name = ftree.second;
				if (file_name.find("xxxx") != string::npos)
					file_name.replace(file_name.find("xxxx"), 4, to_string(run));
				if (file_name.size()) {
					glob_t globbuf;
					glob(file_name.c_str(), 0, NULL, &globbuf);
					if (globbuf.gl_pathc != sessions) {
						cerr << ERROR << "run-" << run << " ^^^^ unmatched friend tree root files: " << endl 
								 << "\t" << sessions << "main root files vs " 
								 << globbuf.gl_pathc << " friend tree root files" << ENDL; 
						continue;
					}
					file_name = globbuf.gl_pathv[session];
					globfree(&globbuf);
				}
				tin->AddFriend(ftree.first.c_str(), file_name.c_str());	// FIXME: what if the friend tree doesn't exist
			}

			if (Contain(mCut, "ok_cut") && tin->GetEntries("ok_cut") < 4500) {
				// FIXME; only for checkruns and mulplots
				cerr << WARNING << "run-" << run << " ^^^^ too short (< 4500 patterns), ignore it" <<ENDL;
				continue;
			}

      bool error = false;
      for (string var : fVars) {
        string branch = fVarName[var].first;
        string leaf   = fVarName[var].second;
        TBranch * br = tin->GetBranch(branch.c_str());
        if (!br) {
					// special branches -- stupid
					if (branch.find("diff_bpm11X") != string::npos && run < 3390) {	
						// no bpm11X in early runs, replace with bpm12X
						cout << WARNING << "run-" << run << " ^^^^ No branch diff_bpm11X, replace with 0.6*diff_bpm12X" << ENDL;
						br = tin->GetBranch(branch.replace(branch.find("bpm11X"), 6, "bpm12X").c_str());
					} else if (branch.find("diff_bpmE") != string::npos && run < 3390) {	
						cout << WARNING << "run-" << run << " ^^^^ No branch diff_bpmE, replace with diff_bpm12X" << ENDL;
						br = tin->GetBranch(branch.replace(branch.find("bpmE"), 4, "bpm12X").c_str());
					} 
					if (!br) {
						cerr << ERROR << "no branch: " << branch << " in tree: " << tree
							<< " of file: " << file_name << ENDL;
						error = true;
						break;
					}
        }
        TLeaf * l = br->GetLeaf(leaf.c_str());
        if (!l) {
					if (leaf.find("diff_bpm11X") != string::npos && run < 3390) {		// lagrange tree
						l = br->GetLeaf(leaf.replace(leaf.find("bpm11X"), 6, "bpm12X").c_str());
					} else if (leaf.find("diff_bpmE") != string::npos && run < 3390) {		// reg tree
						l = br->GetLeaf(leaf.replace(leaf.find("bpmE"), 4, "bpm12X").c_str());
					} 
					if (!l) {
						cerr << ERROR << "no leaf: " << leaf << " in branch: " << branch  << "in var: " << var
							<< " in tree: " << tree << " of file: " << file_name << ENDL;
						error = true;
						break;
					}
        }
        fVarLeaf[var] = l;
      }

      if (error)
        continue;

      if (nSlopes > 0) {
        tin->SetBranchAddress("coeff", slopes_buf);
        tin->SetBranchAddress("err_coeff", slopes_err_buf);
      }

			for (const char * cut : allCuts) {
				const int N = tin->Draw(">>elist", cut, "entrylist");
				TEntryList *elist = (TEntryList*) gDirectory->Get("elist");
				cout << INFO << "use cut: " << cut << ENDL;
				for(int n=0; n<N; n++) { // loop through the events
					if (n % 50000 == 49999) 
						cout << INFO << "read " << n+1 << " entries!" << ENDL;

					const int en = elist->GetEntry(n);
					if (CheckEntryCut(nTotal+en))
					 	continue;

					nOk[cut]++;
					fEntryNumber[cut].push_back(nTotal+en);
					for (string var : fVars) {
						double val;
						fVarLeaf[var]->GetBranch()->GetEntry(en);
						val = fVarLeaf[var]->GetValue();
						// leave sign correction to separated programs
						// if (program == mulplot || program == checkstatistics)	// FIXME: is there a better way to do sign correction
						// 	val *= (fRunSign[run] > 0 ? 1 : (fRunSign[run] < 0 ? -1 : 0)); 

						if (var.find("bpm11X") != string::npos && run < 3390)		// special treatment of bpmE = bpm11X + 0.4*bpm12X
							val *= 0.6;

						// avg/dd of single arm running
						if (	  garmflag == allarms 
								&& (fRunArm[run] == leftarm || fRunArm[run] == rightarm)
								&& (	 var.find("us_avg") != string::npos 
										|| var.find("us_dd")	!= string::npos 
										|| var.find("ds_avg") != string::npos 
										|| var.find("ds_dd")	!= string::npos)) 
						{
							string rvar = var;
							const char * replaced = "_avg";
							int l = 4;
							if (var.find("_dd") != string::npos) {
								replaced = "_dd";
								l = 3;
							}
							const char * replacement = (fRunArm[run] == leftarm ? "l" : "r");
							rvar.replace(rvar.find(replaced), l, replacement);
							fVarLeaf[rvar]->GetBranch()->GetEntry(en);
							val = fVarLeaf[rvar]->GetValue();
						}
						val *= unit[var];	// normalized to standard units
						vars_buf[var] = val;
						fVarValue[cut][var].push_back(val);
						fVarSum[var]	+= val;
						fVarSum2[var] += val * val;

						if (abs(val) > fVarMax[var])
							fVarMax[var] = abs(val);
					}
					for (string custom : fCustoms) {	// FIXME: this may be incorrect while doing calculation before sign and unit corrections
						double val = get_custom_value(fCustomDef[custom]);
						vars_buf[custom] = val;
						fVarValue[cut][custom].push_back(val);
						fVarSum[custom]	+= val;
						fVarSum2[custom]  += val * val;

						if (abs(val) > fVarMax[custom]) 
							fVarMax[custom] = abs(val);
					}
				}

				// slope
        for (pair<string, string> slope : fSlopes) {
          // double unit = ppm/(um/mm);
          fSlopeValue[cut][slope].push_back(slopes_buf[fSlopeIndex[slope].first*cols+fSlopeIndex[slope].second]);
          fSlopeErr[cut][slope].push_back(slopes_err_buf[fSlopeIndex[slope].first*cols+fSlopeIndex[slope].second]);
        }
			}
			nTotal += tin->GetEntries();
			fRunEntries[run] = nTotal;

      tin->Delete();
      f_rootfile.Close();
    }
  }

	for (int i=0; i<allCuts.size(); i++) {
		const char * cut = allCuts[i];
		if (nOk[cut] == 0) {
			if (i==0) {
				cerr << FATAL << "No valid entry for main cut: " << cut << ENDL;
				exit(44);
			} else {
				cerr << ERROR << "No valid entry for cut: " << cut << ENDL;
			}
		}
		cout << INFO << "with cut: " << cut <<  "--read " << nOk[cut] << "/" << nTotal << " good entries." << ENDL;
	}
}

double TBase::get_custom_value(Node *node) {
	if (!node) {
		cerr << ERROR << "Null node" << ENDL;
		return -999999;
	}

	const char * val = node->token.value;
	double v=0, vl=0, vr=0;
	if (node->lchild) vl = get_custom_value(node->lchild);
	if (node->rchild) vr = get_custom_value(node->rchild);
	if (vl == -999999 || vr == -999999)
		return -999999;

	switch (node->token.type) {
		case opt:
			switch(val[0]) {
				case '+':
					return vl + vr;
				case '-':
					return vl - vr;
				case '*':
					return vl * vr;
				case '/':
					return vl / vr;
				case '%':
					return ((int)vl) % ((int)vr);
			}
		case function1:
			return f1[val](vl);
		case function2:
			return f2[val](vl, get_custom_value(node->lchild->sibling));
		case number:
			return atof(val);
		case variable:
			if (	 fVars.find(val) != fVars.cend()
					|| find(fCustoms.cbegin(), fCustoms.cend(), val) != fCustoms.cend())
				return vars_buf[val];
		default:
			cerr << ERROR << "unkonw token type: " << TypeName[node->token.type] << ENDL;
			return -999999;
	}
	return -999999;
}

bool TBase::CheckEntryCut(const long entry) {
  if (ecuts.size() == 0) return false;

  for (pair<long, long> cut : ecuts) {
    long start = cut.first;
    long end = cut.second;
    if (entry >= start) {
      if (end == -1)
        return true;
      else if (entry < end)
        return true;
    }
  }
  return false;
}

const char * TBase::GetUnit (string var) {	// must return const char * to distinguish between same value units (e.g. ppm, um)
	if (var.find("asym") != string::npos)
		return "ppm";
	else if (var.find("diff") != string::npos)
		return "um";
	if (var.find("bpm") != string::npos)
		return "mm";
	else if (var.find("bcm") != string::npos)
		return "uA";
	else if (var.find("us") != string::npos || var.find("ds") != string::npos)
		return "mV";  // FIXME
	else
		return "";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
