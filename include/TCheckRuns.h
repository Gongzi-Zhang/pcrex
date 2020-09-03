#ifndef TCHECKRUN_H
#define TCHECKRUN_H

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <glob.h>
#include <libgen.h>

#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TEntryList.h"
#include "TCut.h"
#include "TCollection.h"
#include "TGraphErrors.h"
#include "TH1F.h"
#include "TPaveStats.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TAxis.h"
#include "TText.h"
#include "TLine.h"

// #include "rcdb.h"
#include "const.h"
#include "line.h"
#include "TConfig.h"

enum Format {pdf, png};

using namespace std;

class TCheckRuns {

    // ClassDe (TCheckRuns, 0) // check statistics

  private:
    Format format = pdf;
    const char *out_name = "checkruns";
    const char *dir	  = "/chafs2/work1/apar/japanOutput/";
    string pattern    = "prexPrompt_pass2_xxxx.???.root";
    const char *tree  = "evt";	// evt, mul, pr
    TCut cut          = "ErrorFlag == 0";
    vector<pair<long, long>> ecuts;
		map<string, const char*> ftrees;
		map<string, const char*> real_trees;

    TConfig fConf;
    int   nRuns;
    int	  nVars;
    int	  nSolos;
		int		nCustoms;
    int	  nComps;
    int	  nCors;

		set<int>		fRuns;
    set<string>	fVars;	// native variables, no custom variable

    set<string>					fSolos;
    vector<string>	    fSoloPlots;
    map<string, VarCut>	fSoloCuts;

    set<string>					fCustoms;
    vector<string>	    fCustomPlots;
    map<string, VarCut>	fCustomCuts;
		map<string, Node *>	fCustomDefs;

    set< pair<string, string> >					fComps;
    vector< pair<string, string> >			fCompPlots;
    map< pair<string, string>, CompCut>	fCompCuts;

    set< pair<string, string> >					fCors;
    vector< pair<string, string> >			fCorPlots;
    map< pair<string, string>, CorCut>  fCorCuts;

    map<string, set<int>>									fSoloBadPoints;
    map<string, set<int>>									fCustomBadPoints;
    map<pair<string, string>, set<int>>	  fCompBadPoints;
    map<pair<string, string>, set<int>>	  fCorBadPoints;

    int nTotal = 0;
		int	nOk = 0;	// total number of ok events
    vector<long> fEntryNumber;
    map<int, vector<string>>  fRootFiles;
    map<int, int> fEntries;     // number of entries for each run
    map<string, pair<string, string>> fVarNames;
    map<string, TLeaf *>        fVarLeaves;
    map<string, double>         vars_buf;
    map<string, vector<double>> fVarValues;	// for all variables
    map<string, double>					fVarSum;		// all
    map<string, double>					fVarSum2;		// sum of square; all

    TCanvas * c;
  public:
     TCheckRuns(const char*);
     ~TCheckRuns();
		 void SetRuns(set<int> runs);
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutFormat(const char * f);
     void SetDir(const char * d);
     void CheckRuns();
     void CheckVars();
     bool CheckVar(string var);
		 bool CheckCustomVar(Node * node);
     void GetValues();
     bool CheckEntryCut(const long entry);
		 double get_custom_value(Node * node);
     void CheckValues();
     void Draw();
     void DrawSolos();
     // void DrawCustoms();
     void DrawComps();
     void DrawCors();

     // auxiliary funcitons
     const char * GetUnit(string var);
};

// ClassImp(TCheckRuns);

TCheckRuns::TCheckRuns(const char* config_file) :
  fConf(config_file)
{
  fConf.ParseConfFile();

  fRuns = fConf.GetRuns();
  nRuns = fRuns.size();
  fVars	= fConf.GetVars();
  nVars = fVars.size();

  fSolos      = fConf.GetSolos();
  fSoloCuts   = fConf.GetSoloCuts();
  fSoloPlots  = fConf.GetSoloPlots();
  nSolos      = fSolos.size();

  fCustoms      = fConf.GetCustoms();
  fCustomDefs   = fConf.GetCustomDefs();
  fCustomCuts   = fConf.GetCustomCuts();
  fCustomPlots  = fConf.GetCustomPlots();
  nCustoms      = fCustoms.size();

  fComps      = fConf.GetComps();
  fCompCuts   = fConf.GetCompCuts();
  fCompPlots  = fConf.GetCompPlots();
  nComps      = fComps.size();

  fCors	      = fConf.GetCors();
  fCorPlots   = fConf.GetCorPlots();
  fCorCuts    = fConf.GetCorCuts();
  nCors       = fCors.size();

  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetPattern())   pattern = fConf.GetPattern();
  if (fConf.GetTreeName())  tree    = fConf.GetTreeName();
  if (fConf.GetTreeCut())   cut     = fConf.GetTreeCut();
  ecuts = fConf.GetEntryCuts();
	ftrees = fConf.GetFriendTrees();

  gROOT->SetBatch(1);
}

TCheckRuns::~TCheckRuns() {
  cerr << INFO << "Release TCheckRuns" << ENDL;
}

void TCheckRuns::SetDir(const char * d) {
  struct stat info;
  if (stat( d, &info) != 0) {
    cerr << FATAL << "can't access specified dir: " << dir << ENDL;
    exit(30);
  } else if ( !(info.st_mode & S_IFDIR)) {
    cerr << FATAL << "not a dir: " << dir << ENDL;
    exit(31);
  }
  dir = d;
}

void TCheckRuns::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << FATAL << "Unknow output format: " << f << ENDL;
    exit(40);
  }
}

void TCheckRuns::SetRuns(set<int> runs) {
  for(int run : runs) {
    if (run < START_RUN || run > END_RUN) {
      cerr << ERROR << "Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << ENDL;
      continue;
    }
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
}

void TCheckRuns::CheckRuns() {
  // check runs against database
  for (set<int>::const_iterator it = fRuns.cbegin(); it != fRuns.cend(); ) {
    int run = *it;
    string p_buf(pattern);
    p_buf.replace(p_buf.find("xxxx"), 4, to_string(run));
    const char * p = Form("%s/%s", dir, p_buf.c_str());

    glob_t globbuf;
    glob(p, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << ERROR << "no root file for run: " << run << ENDL;
      it = fRuns.erase(it);
      continue;
    }
    for (int i=0; i<globbuf.gl_pathc; i++)
      fRootFiles[run].push_back(globbuf.gl_pathv[i]);

    globfree(&globbuf);
    it++;
  }

	// GetValidRuns(fRuns);
  nRuns = fRuns.size();
  if (nRuns == 0) {
    cout << __PRETTY_FUNCTION__ << "FATAL\t no valid run specified" << ENDL;
    exit(4);
  }

  cout << INFO << "" << nRuns << " valid runs specified:" << ENDL;
  for(int run : fRuns) {
    cout << "\t" << run << endl;
  }
}

void TCheckRuns::CheckVars() {
  srand(time(NULL));
  int s = rand() % nRuns;
  set<int>::const_iterator it_r=fRuns.cbegin();
  for(int i=0; i<s; i++)
    it_r++;

  while (it_r != fRuns.cend()) {
		set<string> used_ftrees;
    int run = *it_r;
    const char * file_name = fRootFiles[run][0].c_str();
    TFile * f_rootfile = new TFile(file_name, "read");
    if (!f_rootfile->IsOpen()) {
      cerr << WARNING << "run-" << run << " ^^^^ can't read root file: " << file_name 
           << ", skip this run." << ENDL;
      goto next_run;
    }

    TTree * tin;
		tin = (TTree*) f_rootfile->Get(tree); // receive minitree
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

		for (string var : fVars) {
      size_t n = count(var.begin(), var.end(), '.');
      string branch, leaf;
      if (n==0) {
        branch = var;
      } else if (n==1) {
        size_t pos = var.find_first_of('.');
        if (real_trees.find(var.substr(0, pos)) != real_trees.end()) {
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

			TBranch * bbuf = (TBranch *) tin->FindObject(branch.c_str());
			if (!bbuf) {
				cerr << WARNING << "No such branch: " << branch << " in var: " << var << ENDL;
				tin->Delete();
				f_rootfile->Close();
				exit(24);
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

			fVarNames[var] = make_pair(branch, leaf);
			if (branch.find('.') != string::npos) {
				used_ftrees.insert(StripSpaces(Sub(branch.c_str(), 0, branch.find('.'))));
			} else {
				used_ftrees.insert(bbuf->GetTree()->GetName());
			}
		}

		if (used_ftrees.find(tree) == used_ftrees.end()) {
			cerr << WARNING << "unsed main tree: " << tree << ENDL;
		} else {
			used_ftrees.erase(used_ftrees.find(tree));
		}
		for (map<string, const char*>::const_iterator it=ftrees.cbegin(); it!=ftrees.cend();) {
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

  for (set<string>::const_iterator it=fSolos.cbegin(); it!=fSolos.cend();) {
    if (!CheckVar(*it)) {
			vector<string>::iterator it_p = find(fSoloPlots.begin(), fSoloPlots.end(), *it);
			if (it_p != fSoloPlots.cend())
				fSoloPlots.erase(it_p);

			map<string, VarCut>::const_iterator it_c = fSoloCuts.find(*it);
			if (it_c != fSoloCuts.cend())
				fSoloCuts.erase(it_c);

			cerr << WARNING << "Invalid solo variable: " << *it << ENDL;
      it = fSolos.erase(it);
		} else
      it++;
  }

	for (set<string>::const_iterator it=fCustoms.cbegin(); it!=fCustoms.cend(); ) { // this must come before check of comparison and correlation, which may use some variable defined here
		if (!CheckCustomVar(fCustomDefs[*it])) {
			vector<string>::iterator it_p = find(fCustomPlots.begin(), fCustomPlots.end(), *it);
			if (it_p != fCustomPlots.cend())
				fCustomPlots.erase(it_p);

			map<string, VarCut>::const_iterator it_c = fCustomCuts.find(*it);
			if (it_c != fCustomCuts.cend())
				fCustomCuts.erase(it_c);

			cerr << WARNING << "Invalid custom variable: " << *it << ENDL;
      it = fCustoms.erase(it);
		} else
      it++;
	}

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second) 
				|| strcmp(GetUnit(it->first), GetUnit(it->second)) != 0 ) {
			vector<pair<string, string>>::iterator it_p = find(fCompPlots.begin(), fCompPlots.end(), *it);
			if (it_p != fCompPlots.cend())
				fCompPlots.erase(it_p);

			map<pair<string, string>, VarCut>::const_iterator it_c = fCompCuts.find(*it);
			if (it_c != fCompCuts.cend())
				fCompCuts.erase(it_c);

			cerr << WARNING << "Invalid Comp variable: " << it->first << "\t" << it->second << ENDL;
      it = fComps.erase(it);
		} else 
			it++;
  }

  for (set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
			vector<pair<string, string>>::iterator it_p = find(fCorPlots.begin(), fCorPlots.end(), *it);
			if (it_p != fCorPlots.cend())
				fCorPlots.erase(it_p);

			map<pair<string, string>, VarCut>::const_iterator it_c = fCorCuts.find(*it);
			if (it_c != fCorCuts.cend())
				fCorCuts.erase(it_c);

			cerr << WARNING << "Invalid Cor variable: " << it->first << "\t" << it->second << ENDL;
      it = fCors.erase(it);
		} else
      it++;
  }

  nSolos = fSolos.size();
  nComps = fComps.size();
  nCors  = fCors.size();
	nCustoms = fCustoms.size();

	// initialization
  for(string var : fVars) {
		fVarSum[var] = 0;  
		fVarSum2[var] = 0;
		fVarValues[var] = vector<double>();
	}
  for(string custom : fCustoms) {
		fVarSum[custom] = 0;
		fVarSum2[custom] = 0;
		fVarValues[custom] = vector<double>();
	}

  cout << INFO << "" << nSolos << " valid solo variables specified:" << ENDL;
  for(string solo : fSolos) {
    cout << "\t" << solo << endl;
  }
  cout << INFO << "" << nComps << " valid comparisons specified:" << ENDL;
  for(pair<string, string> comp : fComps) {
    cout << "\t" << comp.first << " , " << comp.second << endl;
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

bool TCheckRuns::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend() && fCustoms.find(var) == fCustoms.cend()) {
    cerr << WARNING << "Unknown variable: " << var << ENDL;
    return false;
  }
  return true;
}

bool TCheckRuns::CheckCustomVar(Node * node) {
  if (node) {
		if (	 node->token.type == variable 
				&& fCustoms.find(node->token.value) == fCustoms.cend()  // FIXME can't solve loop now
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

void TCheckRuns::GetValues() {
  for (int run : fRuns) {
    const size_t sessions = fRootFiles[run].size();
    for (size_t session=0; session<sessions; session++) {
      const char * file_name = fRootFiles[run][session].c_str();
      TFile * f_rootfile = new TFile(file_name, "read");
      if (!f_rootfile->IsOpen()) {
        cerr << ERROR << "run-" << run << " ^^^^ Can't open root file: " << file_name << ENDL;
        f_rootfile->Close();
        continue;
      }

      cout << INFO << Form("Read run: %d, session: %03d\t", run, session)
           << file_name << ENDL;
      TTree * tin = (TTree*) f_rootfile->Get(tree); // receive minitree
      if (! tin) {
        cerr << WARNING << "No such tree: " << tree << " in root file: " << file_name << ENDL;
        f_rootfile->Close();
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

      bool error = false;
      for (string var : fVars) {
        string branch = fVarNames[var].first;
        string leaf   = fVarNames[var].second;
        TBranch * br = tin->GetBranch(branch.c_str());
        if (!br) {
          cerr << ERROR << "no branch: " << branch << " in tree: " << tree
            << " of file: " << file_name << ENDL;
          error = true;
          break;
        }
        TLeaf * l = br->GetLeaf(leaf.c_str());
        if (!l) {
          cerr << ERROR << "no leaf: " << leaf << " in branch: " << branch 
            << " in tree: " << tree << " of file: " << file_name << ENDL;
          error = true;
          break;
        }
        fVarLeaves[var] = l;
      }

      if (error)
        continue;

      const int N = tin->Draw(">>elist", cut, "entrylist");
      TEntryList *elist = (TEntryList*) gDirectory->Get("elist");
      for(int n=0; n<N; n++) { // loop through the events
        if (n % 50000 == 0)
          cout << INFO << "read " << n << " evnets!" << ENDL;

        const int en = elist->GetEntry(n);
        if (CheckEntryCut(nTotal+en))
          continue;

        nOk++;
        fEntryNumber.push_back(nTotal+en);
        for (string var : fVars) {
          double val;
          fVarLeaves[var]->GetBranch()->GetEntry(en);
          val = fVarLeaves[var]->GetValue();
          vars_buf[var] = val;
          fVarValues[var].push_back(val);
          fVarSum[var]	+= val;
          fVarSum2[var] += val * val;
        }
        for (string custom : fCustoms) {
          double val = get_custom_value(fCustomDefs[custom]);
          vars_buf[custom] = val;
          fVarValues[custom].push_back(val);
          fVarSum[custom]	+= val;
          fVarSum2[custom]  += val * val;
        }
      }
      nTotal += tin->GetEntries();
      fEntries[run] = nTotal;

      // delete elist;
      // elist = NULL;
      // for (string var : fVars) {
      //   delete fVarLeaves[var];
      //   fVarLeaves[var] = NULL;
      // }
      tin->Delete();
      f_rootfile->Close();
    }
  }
  cout << INFO << "read " << nOk << "/" << nTotal << " ok events." << ENDL;
  if (fEntryNumber.size() == 0) {
    cerr << FATAL << "No valid entries" << ENDL;
    exit(44);
  }
}

double TCheckRuns::get_custom_value(Node *node) {
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
					|| fCustoms.find(val) != fCustoms.cend())
				return vars_buf[val];
		default:
			cerr << ERROR << "unkonw token type: " << TypeName[node->token.type] << ENDL;
			return -999999;
	}
	return -999999;
}

bool TCheckRuns::CheckEntryCut(const long entry) {
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

void TCheckRuns::CheckValues() {
  const int n = fEntryNumber.size();

  set<string> vars;
  for (string var : fSolos)
    vars.insert(var);
  for (string var : fCustoms)
    vars.insert(var); // assumes (and should be) no same name between solos and customs
  for (string solo : vars) {
    double sum = 0, sum2 = 0;
    double mean = 0, sigma = 0;
    long discontinuity = 120;
    long pre_entry = 0;
    long entry = 0;
    double val = 0;

    // first loop: find out the mean value of largest consecutive events segments 
    // (with large 1s = 120 events discontinuity)
    long start_entry = fEntryNumber[0];
    pre_entry = start_entry;
    double length = 1;  // length of consecutive segments
    for (int i=0; i<n; i++) {
      entry = fEntryNumber[i];
      val = fVarValues[solo][i];

      if (entry - pre_entry > discontinuity || (sigma != 0 && abs(val - mean) > 10*sigma)) {  // end previous segment, start a new segment  
        if (pre_entry - start_entry > length) {
          length = pre_entry - start_entry + 1;
          mean = sum/length;  // length initial value can't be 0
          sigma = sqrt(sum2/length - mean*mean);
        }
        start_entry = entry;
        sum = 0;
        sum2 = 0;
      }

      sum += val;
      sum2 += val*val;
      pre_entry = entry;
    }
    if (pre_entry - start_entry > length) {
      length = pre_entry - start_entry + 1;
      mean = sum/length;  // length initial value can't be 0
      sigma = sqrt(sum2/length - mean*mean);
    }
    cout << INFO << "variable: " << solo << "\t start entry: " << start_entry 
         << "\t end_enry: " << pre_entry << "\t length: " << length 
         << "\tmean: " << mean << "\tsigma: " << sigma << ENDL;

    double low_cut  = fSoloCuts[solo].low;
    double high_cut = fSoloCuts[solo].high;
    double burp_cut = fSoloCuts[solo].burplevel;
    // if (low_cut == 1024 && high_cut != 1024)
    //   low_cut = high_cut;
    // else if (high_cut == 1024 && low_cut != 1024)
    //   high_cut = low_cut;

    double unit = UNITS[GetUnit(solo)];
    if (low_cut != 1024) {
      low_cut /= unit;
      low_cut = mean - low_cut;
    }
    if (high_cut != 1024) {
      high_cut /= unit;
      high_cut = mean + high_cut;
    }
    if (burp_cut != 1024)
      burp_cut /= unit;
    else 
      burp_cut = 10*sigma;

    pre_entry = fEntryNumber[0];
    const int burp_length = 120;  // compare with the average value of previous 120 events
    double burp_ring[burp_length] = {0};
    int burp_index = 0;
    int burp_events = 0;  // how many events in the burp ring now
    mean = fVarValues[solo][0];
    sum = 0;
    bool outlier = false;
    long start_outlier;
		bool has_outlier = false;
		bool has_glitch = false;
    for (int i=0; i<n; i++) {
      entry = fEntryNumber[i];
      val = fVarValues[solo][i];

      if (   (low_cut  != 1024 && val < low_cut)
          || (high_cut != 1024 && val > high_cut) ) {  // outlier
        if (!outlier) {
          start_outlier = entry;
					outlier = true;
					has_outlier = true;
				}
				fSoloBadPoints[solo].insert(fEntryNumber[i]);
      } else {
        if (outlier) {
          cerr << OUTLIER << "in variable: " << solo << "\tfrom entry: " << start_outlier << " to entry: " << fEntryNumber[i-1] << ENDL;
          // FIXME should I add run info in the OUTLIER output?
					outlier = false;
				}
      }

      if (entry - pre_entry > discontinuity) {  // start a new count
        burp_events = 0;
        sum = 0;
        mean = val;
      }

      if (abs(val - mean) > burp_cut) {
        cerr << GLITCH << "glitch in variable: " << solo << " in entry " << entry << "\tmean: " << mean << "\tvalue: " << val << ENDL;
				has_glitch = true;
				fSoloBadPoints[solo].insert(fEntryNumber[i]);
      }

      burp_ring[burp_index] = val;
      burp_index++;
      burp_index %= burp_length;
      if (burp_events == burp_length) {
        sum -= burp_ring[burp_index];
      } else {
        burp_events++;
      }
      sum += val;
      mean = sum/burp_events;
      pre_entry = entry;
    }
		if (outlier)
			cerr << OUTLIER << "in variable: " << solo << "\tfrom entry: " << start_outlier << " to entry: " << fEntryNumber[n-1] << ENDL;

		if (	(has_outlier || has_glitch) 
				&& find(fSoloPlots.cbegin(), fSoloPlots.cend(), solo) == fSoloPlots.cend())
			fSoloPlots.push_back(solo);
  }

  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
    double low_cut  = fCompCuts[comp].low;
    double high_cut = fCompCuts[comp].high;
    double burp_cut = fCompCuts[comp].burplevel;
    double unit = UNITS[GetUnit(var1)];
    if (low_cut != 1024)	low_cut /= unit;
    if (high_cut != 1024) high_cut /= unit;
    if (burp_cut != 1024) burp_cut /= unit;

    bool outlier = false;
    long start_outlier;
		bool has_outlier = false;
		for (int i=0; i<n; i++) {
      double val1, val2;
			val1 = fVarValues[var1][i];
			val2 = fVarValues[var2][i];
			double diff = abs(val1 - val2);

      if (  (low_cut  != 1024 && diff < low_cut)
         || (high_cut != 1024 && diff > high_cut) ) {
        // || (stat != 1024 && abs(diff-mean) > stat*sigma
        if (!outlier) {
          start_outlier = fEntryNumber[i];
					outlier = true;
					has_outlier = true;
				}
				fCompBadPoints[comp].insert(fEntryNumber[i]);
      } else {
        if (outlier) {
          cerr << OUTLIER << "in variable: " << var1 << " vs " << var2 << "\tfrom entry: " << start_outlier << " to entry: " << fEntryNumber[i-1] << ENDL;
          // FIXME should I add run info in the OUTLIER output?
					outlier = false;
				}
      }
    }
		if (outlier)
			cerr << OUTLIER << "in variable: " << var1 << " vs " << var2 << "\tfrom entry: " << start_outlier << " to entry: " << fEntryNumber[n-1] << ENDL;

		if (has_outlier && find(fCompPlots.cbegin(), fCompPlots.cend(), comp) == fCompPlots.cend())
			fCompPlots.push_back(comp);
	}

  /*
  for (pair<string, string> cor : fCors) {
    string yvar = cor.first;
    string xvar = cor.second;
    const double low_cut  = fCorCuts[cor].low;
    const double high_cut = fCorCuts[cor].high;
    const double burp_cut = fCompCuts[cor].burplevel;
    const int n = fEntryNumber.size();
		for (int i=0; i<n; i++) {
      double xval, yval;
			xval = fVarValues[xvar][i];
			yval = fVarValues[yvar][i];

      if ( 1 ) {	// FIXME
        cout << ALERT << "bad datapoint in Cor: " << yvar << " vs " << xvar << ENDL;
        if (find(fCorPlots.cbegin(), fCorPlots.cend(), *it) == fCorPlots.cend())
          fCorPlots.push_back(*it);
        fCorBadPoints[*it].insert(i);
      }
    }
	}
  */

  cout << INFO << "done with checking values" << ENDL;
}

void TCheckRuns::Draw() {
  c = new TCanvas("c", "c", 800, 600);
  c->SetGridy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(1110);
  gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  gStyle->SetBarWidth(1.05);

  if (format == pdf)
    c->Print(Form("%s.pdf[", out_name));

  DrawSolos();
  DrawComps();
  // DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << INFO << "done with drawing plots" << ENDL;
}

void TCheckRuns::DrawSolos() {
  vector<string> vars;
  for (string var : fSoloPlots)
    vars.push_back(var);
  for (string var : fCustomPlots)
    vars.push_back(var); // assumes (and should be) no same name between solos and customs
  for (string var : vars) {
    string unit = GetUnit(var);

    TGraphErrors * g = new TGraphErrors();      // all data points
    // TGraphErrors * g_err = new TGraphErrors();  // ErrorFlag != 0
    TGraphErrors * g_bad = new TGraphErrors();  // ok data points that don't pass check

    for(int i=0, ibad=0; i<nOk; i++) {
      double val;
      val = fVarValues[var][i];

      // g->SetPoint(i, i+1, val);

      // g_err->SetPoint(ierr, i+1, val);
      // ierr++;

      g->SetPoint(i, fEntryNumber[i], val);
      if (fSoloBadPoints[var].find(i) != fSoloBadPoints[var].cend()) {
        g_bad->SetPoint(ibad, fEntryNumber[i], val);
        ibad++;
      }
    }
    g->SetTitle((var + ";;" + unit).c_str());
    // g_err->SetMarkerStyle(1.2);
    // g_err->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(1.2);
    g_bad->SetMarkerSize(1.5);
    g_bad->SetMarkerColor(kRed);

    c->cd();
    g->Draw("AP");
    // g_err->Draw("P same");
    g_bad->Draw("P same");

    TAxis * ay = g->GetYaxis();
    double ymin = ay->GetXmin();
    double ymax = ay->GetXmax();

    if (nRuns > 1) {
      for (int run : fRuns) {
        TLine *l = new TLine(fEntries[run], ymin, fEntries[run], ymax);
        l->SetLineStyle(2);
        l->SetLineColor(kRed);
        l->Draw("same");
        TText *t = new TText(fEntries[run]-nTotal/(2*nRuns), ymin+(ymax-ymin)/30*(run%5 + 1), Form("%d", run));
        t->SetTextSize((t->GetTextSize())/(nRuns/7+1));
        t->SetTextColor(kRed);
        t->Draw("same");
      }
    }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, var.c_str()));

    c->Clear();
  }
  cout << INFO << "Done with drawing Solos and customized variables." << ENDL;
}

// it looks like not a good idea to draw diff plots with a few hundred thousands points
void TCheckRuns::DrawComps() {
  int MarkerStyles[] = {29, 33, 34, 31};
  for (pair<string, string> var : fCompPlots) {
    string var1 = var.first;
    string var2 = var.second;

    string unit = GetUnit(var1);

    TGraphErrors *g1 = new TGraphErrors();
    TGraphErrors *g2 = new TGraphErrors();
    TGraphErrors *g_err1 = new TGraphErrors();
    TGraphErrors *g_err2 = new TGraphErrors();
    TGraphErrors *g_bad1 = new TGraphErrors();
    TGraphErrors *g_bad2 = new TGraphErrors();
    TH1F * h_diff = new TH1F("diff", "", nOk, 0, nOk);

    double ymin, ymax;
    for(int i=0, ibad=0; i<nOk; i++) {
      double val1;
      double val2;
			val1 = fVarValues[var1][i];
			val2 = fVarValues[var2][i];
      if (i==0) 
        ymin = ymax = val1;

      if (val1 < ymin) ymin = val1;
      if (val1 > ymax) ymax = val1;
      if (val2 < ymin) ymin = val2;
      if (val2 > ymax) ymax = val2;

      g1->SetPoint(i, fEntryNumber[i], val1);
      g2->SetPoint(i, fEntryNumber[i], val2);
      h_diff->SetBinContent(i+1, val1-val2);
      
      //  g_err1->SetPoint(ierr, i+1, val1);
      //  g_err2->SetPoint(ierr, i+1, val2);
      //  ierr++;
      if (fCompBadPoints[var].find(i) != fCompBadPoints[var].cend()) {
        g_bad1->SetPoint(ibad, fEntryNumber[i], val1);
        g_bad2->SetPoint(ibad, fEntryNumber[i], val2);
        ibad++;
      }
    }
    double margin = (ymax-ymin)/10;
    ymin -= margin;
    ymax += margin;

		g1->SetTitle(Form("%s & %s;;%s", var1.c_str(), var2.c_str(), unit.c_str()));
		g1->SetMarkerColor(8);
		g2->SetMarkerColor(9);
    g_err1->SetMarkerStyle(1.2);
    g_err1->SetMarkerColor(kBlue);
    g_err2->SetMarkerStyle(1.2);
    g_err2->SetMarkerColor(kBlue);
    g_bad1->SetMarkerStyle(1.2);
    g_bad1->SetMarkerSize(1.5);
    g_bad1->SetMarkerColor(kRed);
    g_bad2->SetMarkerStyle(1.2);
    g_bad2->SetMarkerSize(1.5);
    g_bad2->SetMarkerColor(kRed);

    c->cd();
    TPad * p1 = new TPad("p1", "p1", 0.0, 0.35, 1.0, 1.0);
    p1->Draw();
    p1->SetGridy();
    p1->SetBottomMargin(0);
    p1->SetRightMargin(0.05);
    TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.35);
    p2->Draw();
    p2->SetGrid();
    p2->SetTopMargin(0);
    p2->SetBottomMargin(0.17);
    p2->SetRightMargin(0.05);

    p1->cd();
    g1->GetXaxis()->SetLabelSize(0);
    g1->Draw("AP");
    g2->Draw("P same");
    g1->GetYaxis()->SetRangeUser(ymin, ymax+(ymax-ymin)/5);

    g_bad1->Draw("P same");
    g_bad2->Draw("P same");
    g_err1->Draw("P same");
    g_err2->Draw("P same");
    
    p2->cd();
    h_diff->SetStats(kFALSE);
    h_diff->SetFillColor(kGreen);
    h_diff->SetBarOffset(0.5);
    h_diff->SetBarWidth(1);
    h_diff->Draw("B");
    p2->Update();

    if (nRuns > 1) {
      p1->cd();
      for (int run : fRuns) {
        TLine *l = new TLine(fEntries[run], ymin, fEntries[run], ymax);
        l->SetLineStyle(2);
        l->SetLineColor(kRed);
        l->Draw("same");
        TText *t = new TText(fEntries[run]-nTotal/(5*nRuns), ymin+(ymax-ymin)/30, Form("%d", run));
        t->Draw("same");
      }
    }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s-%s.png", out_name, var1.c_str(), var2.c_str()));
    c->Clear();
    h_diff->Delete();
    h_diff = NULL;
  }
  cout << INFO << "Done with drawing Comparisons." << ENDL;
}

void TCheckRuns::DrawCors() {
  for (pair<string, string> var : fCorPlots) {
    string xvar = var.second;
    string yvar = var.first;
    string xunit = GetUnit(xvar);
    string yunit = GetUnit(yvar);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    // TGraphErrors * g_good = new TGraphErrors();

    for(int i=0, ibad=0; i<nOk; i++) {
      double xval;
      double yval;
			xval = fVarValues[xvar][i];
			yval = fVarValues[yvar][i];

      g->SetPoint(i, xval, yval);
      
      // g_good->SetPoint(i, xval, yval);
      if (fCorBadPoints[var].find(i) != fCorBadPoints[var].cend()) {
        g_bad->SetPoint(ibad, xval, yval);
        ibad++;
      }
    }
		g->SetTitle((yvar + " vs " + xvar + ";" + xunit + ";" + yunit).c_str());
    g->SetMarkerStyle(1.2);
    g->SetMarkerColor(kBlue);
    // g_good->SetMarkerColor(kBlack);
    g_bad->SetMarkerStyle(1.2);
    g_bad->SetMarkerColor(kRed);

    // g_good->Fit("pol1");

    // TPaveStats * st;
    c->cd();
    gPad->SetRightMargin(0.05);
    g->Draw("AP");
    gPad->Update();
    // st = (TPaveStats *) g->FindObject("stats");
    // st->SetName("g_stats");
    // st->SetX2NDC(0.95); 
    // st->SetX1NDC(0.95-0.3); 
    // st->SetY2NDC(0.9);
    // st->SetY1NDC(0.75);

    // g_good->Draw("P same");
    g_bad->Draw("P same");
    
    // TAxis * ay = g->GetYaxis();
    // double min = ay->GetXmin();
    // double max = ay->GetXmax();
    // ay->SetRangeUser(min, max+(max-min)/9);
    // gPad->Update();

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s_vs_%s.png", out_name, xvar.c_str(), yvar.c_str()));
    c->Clear();
  }
  cout << INFO << "Done with drawing Correlations." << ENDL;
}

const char * TCheckRuns::GetUnit (string var) {	// must return const char * to distinguish between same value units (e.g. ppm, um)
  if (strcmp(tree, "evt") == 0) {  // event tree
    if (var.find("bpm") != string::npos)
      return "mm";
    else if (var.find("bcm") != string::npos)
      return "uA";
    else if (var.find("us") != string::npos || var.find("ds") != string::npos)
      return "mV";  // FIXME
    else 
      return "";
  } else if (strcmp(tree, "mul") == 0  || strcmp(tree, "dit") == 0) {
    if (var.find("asym") != string::npos)
      return "ppm";
    else if (var.find("diff") != string::npos)
      return "um";
    else
      return "";
  }
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
