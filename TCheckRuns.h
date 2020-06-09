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
     bool CheckVar(string exp);
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

  gROOT->SetBatch(1);
}

TCheckRuns::~TCheckRuns() {
  cerr << __PRETTY_FUNCTION__ << ":INFO\t Release TCheckRuns\n";
}

void TCheckRuns::SetDir(const char * d) {
  struct stat info;
  if (stat( d, &info) != 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t can't access specified dir: " << dir << endl;
    exit(30);
  } else if ( !(info.st_mode & S_IFDIR)) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t not a dir: " << dir << endl;
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
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unknow output format: " << f << endl;
    exit(40);
  }
}

void TCheckRuns::SetRuns(set<int> runs) {
  for(int run : runs) {
    if (run < START_RUN || run > END_RUN) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << endl;
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
      cout << __PRETTY_FUNCTION__ << ":ERROR\t no root file for run: " << run << endl;
      it = fRuns.erase(it);
      continue;
    }
    for (int i=0; i<globbuf.gl_pathc; i++)
      fRootFiles[run].push_back(globbuf.gl_pathv[i]);

    globfree(&globbuf);
    it++;
  }

  nRuns = fRuns.size();
  if (nRuns == 0) {
    cout << __PRETTY_FUNCTION__ << "FATAL\t no valid run specified" << endl;
    exit(4);
  }

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nRuns << " valid runs specified:\n";
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
    int run = *it_r;
    const char *file_name = fRootFiles[run][0].c_str();
    TFile * f_rootfile = new TFile(file_name, "read");
    if (f_rootfile->IsOpen()) {
      if (!f_rootfile->GetListOfKeys()->Contains(tree)) {
        cerr << __PRETTY_FUNCTION__ << ":FATAL\t no tree: " << tree << " in root file: " << file_name;
        exit(22);
      }
      TTree * tin = (TTree*) f_rootfile->Get(tree);
      if (tin != NULL) {
        cout << __PRETTY_FUNCTION__ << ":INFO\t use file to check vars: " << file_name << endl;

        TObjArray * l_var = tin->GetListOfBranches();
        for (string var : fVars) {
          size_t dot_pos = var.find_first_of('.');
          string branch = (dot_pos == string::npos) ? var : var.substr(0, dot_pos);
          string leaf = (dot_pos == string::npos) ? "" : var.substr(dot_pos+1);

          TBranch * bbuf = (TBranch *) l_var->FindObject(branch.c_str());
          if (!bbuf) {
            cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such branch: " << branch << " in var: " << var << endl;
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid branches:\n";
            TIter next(l_var);
            TBranch *br;
            while (br = (TBranch*) next()) {
              cout << "\t" << br->GetName() << endl;
            }
            // FIXME: clear up before exit
            exit(24);
          }

          TObjArray * l_leaf = bbuf->GetListOfLeaves();
          if (leaf.size() == 0) {
            leaf = l_leaf->At(0)->GetName();  // use the first leaf
          }
          TLeaf * lbuf = (TLeaf *) l_leaf->FindObject(leaf.c_str());
          if (!lbuf) {
            cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such leaf: " << leaf << " in var: " << var << endl;
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid leaves:" << endl;
            TIter next(l_leaf);
            TLeaf *l;
            while (l = (TLeaf*) next()) {
              cout << "\t" << l->GetName() << endl;
            }
            // FIXME: clear up before exit
            exit(24);
          }

          fVarNames[var] = make_pair(branch, leaf);
        }
        tin->Delete();
        f_rootfile->Close();
        break;
      }
		}
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t root file of run: " << run << " is broken, ignore it.\n";
    it_r = fRuns.erase(it_r);

    if (it_r == fRuns.cend())
      it_r = fRuns.cbegin();
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

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid solo variable: " << *it << endl;
      it = fSolos.erase(it);
		} else
      it++;
  }

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); ) {
    if (!CheckVar(it->first) || !CheckVar(it->second)) {
			vector<pair<string, string>>::iterator it_p = find(fCompPlots.begin(), fCompPlots.end(), *it);
			if (it_p != fCompPlots.cend())
				fCompPlots.erase(it_p);

			map<pair<string, string>, VarCut>::const_iterator it_c = fCompCuts.find(*it);
			if (it_c != fCompCuts.cend())
				fCompCuts.erase(it_c);

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid Comp variable: " << it->first << "\t" << it->second << endl;
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

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid Cor variable: " << it->first << "\t" << it->second << endl;
      it = fCors.erase(it);
		} else
      it++;
  }

	for (set<string>::const_iterator it=fCustoms.cbegin(); it!=fCustoms.cend(); ) {
		if (!CheckCustomVar(fCustomDefs[*it])) {
			vector<string>::iterator it_p = find(fCustomPlots.begin(), fCustomPlots.end(), *it);
			if (it_p != fCustomPlots.cend())
				fCustomPlots.erase(it_p);

			map<string, VarCut>::const_iterator it_c = fCustomCuts.find(*it);
			if (it_c != fCustomCuts.cend())
				fCustomCuts.erase(it_c);

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid custom variable: " << *it << endl;
      it = fCustoms.erase(it);
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

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSolos << " valid solo variables specified:\n";
  for(string solo : fSolos) {
    cout << "\t" << solo << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nComps << " valid comparisons specified:\n";
  for(pair<string, string> comp : fComps) {
    cout << "\t" << comp.first << " , " << comp.second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nCors << " valid correlations specified:\n";
  for(pair<string, string> cor : fCors) {
    cout << "\t" << cor.first << " : " << cor.second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nCustoms << " valid customs specified:\n";
  for(string custom : fCustoms) {
    cout << "\t" << custom << endl;
  }
}

bool TCheckRuns::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t Unknown variable: " << var << endl;
    return false;
  }
  return true;
}

bool TCheckRuns::CheckCustomVar(Node * node) {
  if (node) {
		if (	 node->token.type == variable 
				&& fCustoms.find(node->token.value) == fCustoms.cend()
				&& fVars.find(node->token.value) == fVars.cend() ) {
			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Unknown variable: " << node->token.value << endl;
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
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
        f_rootfile->Close();
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d\t", run, session)
           << file_name << endl;
      TTree * tin = (TTree*) f_rootfile->Get(tree); // receive minitree
      if (! tin) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such tree: " << tree << " in root file: " << file_name << endl;
        f_rootfile->Close();
        continue;
      }

      bool error = false;
      for (string var : fVars) {
        string branch = fVarNames[var].first;
        string leaf   = fVarNames[var].second;
        TBranch * br = tin->GetBranch(branch.c_str());
        if (!br) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t no branch: " << branch << " in tree: " << tree
            << " of file: " << file_name << endl;
          error = true;
          break;
        }
        TLeaf * l = br->GetLeaf(leaf.c_str());
        if (!l) {
          cerr << __PRETTY_FUNCTION__ << ":ERROR\t no leaf: " << leaf << " in branch: " << branch 
            << " in tree: " << tree << " of file: " << file_name << endl;
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
        if (n % 20000 == 0)
          cout << __PRETTY_FUNCTION__ << ":INFO\t processing " << n << " evnets!" << endl;

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
  cout << __PRETTY_FUNCTION__ << ":INFO\t read " << nOk << "/" << nTotal << " ok events." << endl;
}

double TCheckRuns::get_custom_value(Node *node) {
	if (!node) {
		cerr << __PRETTY_FUNCTION__ << ":ERROR\t Null node\n";
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
			cerr << __PRETTY_FUNCTION__ << ":ERROR\t unkonw token type: " << TypeName[node->token.type] << endl;
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
  for (string solo : fSolos) {
    const double low  = fSoloCuts[solo].low;
    const double high = fSoloCuts[solo].high;
    const double stat = fSoloCuts[solo].stability;
    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean, sigma = 0;
    const int n = fEntryNumber.size();
		for (int i=0; i<n; i++) {
      double val;
			val = fVarValues[solo][i];

      if (i == 0) {
        mean = val;
        sigma = 0;
      }

      if (  (low  != 1024 && val < low)
            || (high != 1024 && val > high)
            || (stat != 1024 && abs(val-mean) > stat*sigma) ) {
        // cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in " << var << endl;
        if (find(fSoloPlots.cbegin(), fSoloPlots.cend(), solo) == fSoloPlots.cend())
          fSoloPlots.push_back(solo);
        fSoloBadPoints[solo].insert(i);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
	}

  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
    const double low  = fCompCuts[comp].low;
    const double high = fCompCuts[comp].high;
    const double stat = fCompCuts[comp].stability;
    const int n = fEntryNumber.size();
		for (int i=0; i<n; i++) {
      double val1, val2;
			val1 = fVarValues[var1][i];
			val2 = fVarValues[var2][i];
			double diff = abs(val1 - val2);

      if (  (low  != 1024 && diff < low)
            || (high != 1024 && diff > high) 
        // || (stat != 1024 && abs(diff-mean) > stat*sigma
				) {
        // cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Comp: " << var1 << " vs " << var2 << endl;
        if (find(fCompPlots.cbegin(), fCompPlots.cend(), comp) == fCompPlots.cend())
          fCompPlots.push_back(comp);
        fCompBadPoints[comp].insert(i);
      }
    }
	}

  for (pair<string, string> cor : fCors) {
    string yvar = cor.first;
    string xvar = cor.second;
    const double low   = fCorCuts[cor].low;
    const double high  = fCorCuts[cor].high;
    const double stat = fCompCuts[cor].stability;
    const int n = fEntryNumber.size();
		for (int i=0; i<n; i++) {
      double xval, yval;
			xval = fVarValues[xvar][i];
			yval = fVarValues[yvar][i];

      /*
      if ( 1 ) {	// FIXME
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Cor: " << yvar << " vs " << xvar << endl;
        if (find(fCorPlots.cbegin(), fCorPlots.cend(), *it) == fCorPlots.cend())
          fCorPlots.push_back(*it);
        fCorBadPoints[*it].insert(i);
      }
      */
    }
	}

  cout << __PRETTY_FUNCTION__ << ":INFO\t done with checking values\n";
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

  for (string var : fCustomPlots)
    fSoloPlots.push_back(var);
  DrawSolos();
  DrawComps();
  // DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << __PRETTY_FUNCTION__ << ":INFO\t done with drawing plots\n";
}

void TCheckRuns::DrawSolos() {
  for (string var : fSoloPlots) {
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
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Solos.\n";
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
    g_err1->SetMarkerStyle(1.2);
    g_err1->SetMarkerColor(kBlue);
    g_err2->SetMarkerStyle(1.2);
    g_err2->SetMarkerColor(kBlue);
    g_bad1->SetMarkerStyle(1.2);
    g_bad1->SetMarkerColor(kRed);
    g_bad2->SetMarkerStyle(1.2);
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
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Comparisons.\n";
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
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Correlations.\n";
}

const char * TCheckRuns::GetUnit (string var) {
  if (var.find("asym") != string::npos) {
		return "ppm";
  } else if (var.find("diff") != string::npos) {
		return "um";
  } else {
    return "";
  }
	return "";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
