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

typedef struct {
  double hw_sum;
  double block0, block1, block2, block3;
  double num_samples;
  double Device_Error_Code;
  double hw_sum_raw;
  double block0_raw, block1_raw, block2_raw, block3_raw;
  double sequence_number;
} DATASET;
enum Format {pdf, png};

using namespace std;

class TCheckRun {

    // ClassDe (TCheckRun, 0) // check statistics

  private:
    TConfig fConf;
    Format format = pdf;
    const char *out_name = "checkrun";
    const char *dir	= "/chafs2/work1/apar/japanOutput/";
    const char *prefix = "prexPrompt_pass2";
    const char *tree   = "evt";	// evt, mul, pr
		set<int>		fRuns;
    map<int, int> fSessions;
    map<int, int> fEntries;  
    int   nRuns;
    int	  nVars;
    int	  nSolos;
		int		nCustoms;
    int	  nComps;
    int	  nCors;
    set<string>	  fVars;	// native variables, no custom variable

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

    int nTotal = 0;
		int	nOk = 0;	// total number of ok events
    map<string, set<int>>									fSoloBadPoints;
    map<string, set<int>>									fCustomBadPoints;
    map<pair<string, string>, set<int>>	  fCompBadPoints;
    map<pair<string, string>, set<int>>	  fCorBadPoints;

    map<string, DATASET>				vars_buf;		// for native variables
    map<string, vector<double>> fVarValues;	// for all variables
    vector<double>              fErrorFlags;
    map<string, double>					fVarSum;		// all
    map<string, double>					fVarSum2;		// sum of square; all

    TCanvas * c;
  public:
     TCheckRun(const char*);
     ~TCheckRun();
		 void SetRuns(set<int> runs);
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutFormat(const char * f);
     void SetDir(const char * d);
     void CheckRun();
     void CheckVars();
     bool CheckVar(string exp);
		 bool CheckCustomVar(Node * node);
     void GetValues();
		 double get_custom_value(Node * node);
     void CheckValues();
     void Draw();
     void DrawSolos();
     void DrawCustoms();
     void DrawComps();
     void DrawCors();

     // auxiliary funcitons
     const char * GetUnit(string var);
};

// ClassImp(TCheckRun);

TCheckRun::TCheckRun(const char* config_file) :
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
  if (fConf.GetTreeName())  tree = fConf.GetTreeName();

  gROOT->SetBatch(1);
}

TCheckRun::~TCheckRun() {
  cerr << __PRETTY_FUNCTION__ << ":INFO\t Release TCheckRun\n";
}

void TCheckRun::SetDir(const char * d) {
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

void TCheckRun::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unknow output format: " << f << endl;
    exit(40);
  }
}

void TCheckRun::SetRuns(set<int> runs) {
  for(set<int>::const_iterator it=runs.cbegin(); it != runs.cend(); it++) {
    int run = *it;
    if (run < START_RUN || run > END_RUN) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << endl;
      continue;
    }
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
}

void TCheckRun::CheckRun() {
  // check runs against database
  for (set<int>::const_iterator it = fRuns.cbegin(); it != fRuns.cend(); ) {
    int run = *it;
    glob_t globbuf;
    const char * pattern  = Form("%s/%s_%d.???.root", dir, prefix, run);
    glob(pattern, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << __PRETTY_FUNCTION__ << ":ERROR\t no root file for run: " << run << endl;
      it = fRuns.erase(it);
      continue;
    }
    fSessions[run] = globbuf.gl_pathc;
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

void TCheckRun::CheckVars() {
	const char * file_name = Form("%s/%s_%d.000.root", dir, prefix, *(fRuns.cbegin()));
	TFile * f_rootfile = new TFile(file_name, "read");
	if (f_rootfile->IsOpen()) {
		TTree * tin = (TTree*) f_rootfile->Get(tree); // receive minitree
		if (tin != NULL) {
			TObjArray * l_var = tin->GetListOfBranches();
			bool error_var_flag = false;
			for (set<string>::const_iterator it=fVars.cbegin(); it != fVars.cend(); it++) {
				if (!l_var->FindObject(it->c_str())) {
					cerr << __PRETTY_FUNCTION__ << ":WARNING\t Variable not found: " << *it << endl;
					it = fVars.erase(it);
					error_var_flag = true;
          if (it == fVars.end())
            break;
				}
			}
			if (error_var_flag) {
				TIter next(l_var);
				TBranch *br;
				cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid variables:\n";
				while (br = (TBranch*) next()) {
					cout << "\t" << br->GetName() << endl;
				}
			}

      tin->Delete();
			f_rootfile->Close();
		}
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
	for(set<string>::const_iterator it=fVars.cbegin(); it!=fVars.cend(); it++) {
		vars_buf[*it] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		fVarSum[*it] = 0;  
		fVarSum2[*it] = 0;
		fVarValues[*it] = vector<double>();
	}
	for(set<string>::const_iterator it=fCustoms.cbegin(); it!=fCustoms.cend(); it++) {
		vars_buf[*it] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		fVarSum[*it] = 0;
		fVarSum2[*it] = 0;
		fVarValues[*it] = vector<double>();
	}

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSolos << " valid solo variables specified:\n";
  for(set<string>::const_iterator it=fSolos.cbegin(); it!=fSolos.cend(); it++) {
    cout << "\t" << *it << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nComps << " valid comparisons specified:\n";
  for(set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); it++) {
    cout << "\t" << it->first << " , " << it->second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nCors << " valid correlations specified:\n";
  for(set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); it++) {
    cout << "\t" << it->first << " : " << it->second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nCustoms << " valid customs specified:\n";
  for(set<string>::const_iterator it=fCustoms.cbegin(); it!=fCustoms.cend(); it++) {
    cout << "\t" << *it << endl;
  }
}

bool TCheckRun::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t Unknown variable: " << var << endl;
    return false;
  }
  return true;
}

bool TCheckRun::CheckCustomVar(Node * node) {
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

void TCheckRun::GetValues() {
  for (int run : fRuns) {
    for (int session=0; session<fSessions[run]; session++) {
      const char * file_name = Form("%s/%s_%d.%03d.root", dir, prefix, run, session);
      TFile * f_rootfile = new TFile(file_name, "read");
      if (!f_rootfile->IsOpen()) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
        f_rootfile->Close();
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d\t", run, session)
           << Form("%s_%d.%03d.root", prefix, run, session) << endl;
      TTree * tin = (TTree*) f_rootfile->Get(tree); // receive minitree
      if (! tin) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such tree: " << tree << " in root file: " << file_name << endl;
        f_rootfile->Close();
        continue;
      }

      double ErrorFlag;
      tin->SetBranchAddress("ErrorFlag", &ErrorFlag);
      for (set<string>::const_iterator it=fVars.cbegin(); it!=fVars.cend(); it++)
        tin->SetBranchAddress(it->c_str(), &(vars_buf[*it]));

      const int nentries = tin->GetEntries();  // number of events
      nTotal += nentries;
      fEntries[run] = nTotal;

      for(int n=0; n<nentries; n++) { // loop through the events
        if (n % 20000 == 0)
          cout << __PRETTY_FUNCTION__ << ":INFO\t processing " << n << " evnets!" << endl;

        tin->GetEntry(n);
        fErrorFlags.push_back(ErrorFlag);
        for (string var : fVars)
          fVarValues[var].push_back(vars_buf[var].hw_sum);
        for (string var : fCustoms)
          fVarValues[var].push_back(get_custom_value(fCustomDefs[var]));

        if (ErrorFlag == 0) {
          nOk++;
          for (set<string>::const_iterator it=fVars.cbegin(); it!=fVars.cend(); it++) {
            double val = vars_buf[*it].hw_sum;
            fVarSum[*it]	+= val;
            fVarSum2[*it] += val * val;
          }
          for (set<string>::const_iterator it=fCustoms.cbegin(); it!=fCustoms.cend(); it++) {
            double val = get_custom_value(fCustomDefs[*it]);
            fVarSum[*it]	+= val;
            fVarSum2[*it]	+= val * val;
          }
        }
      }
      tin->Delete();
      f_rootfile->Close();
    }
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t read " << nOk << " ok events in total" << endl;
}

double TCheckRun::get_custom_value(Node *node) {
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
				return vars_buf[val].hw_sum;
		default:
			cerr << __PRETTY_FUNCTION__ << ":ERROR\t unkonw token type: " << TypeName[node->token.type] << endl;
			return -999999;
	}
	return -999999;
}

void TCheckRun::CheckValues() {
  for (set<string>::const_iterator it=fSolos.begin(); it!=fSolos.end(); it++) {
    string var = *it;

    const double low  = fSoloCuts[*it].low;
    const double high = fSoloCuts[*it].high;
    const double stat = fSoloCuts[*it].stability;
    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean, sigma = 0;
    const int n = fErrorFlags.size();
		for (int i=0; i<n; i++) {
      double val;
			val = fVarValues[var][i];

      if (i == 0) {
        mean = val;
        sigma = 0;
      }

      if (  fErrorFlags[i] == 0 
         && (  (low  != 1024 && val < low)
            || (high != 1024 && val > high)
            || (stat != 1024 && abs(val-mean) > stat*sigma) ) ) {
        // cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in " << var << endl;
        if (find(fSoloPlots.cbegin(), fSoloPlots.cend(), *it) == fSoloPlots.cend())
          fSoloPlots.push_back(var);
        fSoloBadPoints[var].insert(i);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
	}

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); it++) {
    string var1 = it->first;
    string var2 = it->second;
    const double low  = fCompCuts[*it].low;
    const double high = fCompCuts[*it].high;
    const double stat = fCompCuts[*it].stability;
    const int n = fErrorFlags.size();
		for (int i=0; i<n; i++) {
      double val1, val2;
			val1 = fVarValues[var1][i];
			val2 = fVarValues[var2][i];
			double diff = abs(val1 - val2);

      if (  fErrorFlags[i] == 0 
         && (  (low  != 1024 && diff < low)
            || (high != 1024 && diff > high) )
        // || (stat != 1024 && abs(diff-mean) > stat*sigma
				) {
        // cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Comp: " << var1 << " vs " << var2 << endl;
        if (find(fCompPlots.cbegin(), fCompPlots.cend(), *it) == fCompPlots.cend())
          fCompPlots.push_back(*it);
        fCompBadPoints[*it].insert(i);
      }
    }
	}

  for (set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); it++) {
    string yvar = it->first;
    string xvar = it->second;
    const double low   = fCorCuts[*it].low;
    const double high  = fCorCuts[*it].high;
    const double stat = fCompCuts[*it].stability;
    const int n = fErrorFlags.size();
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

void TCheckRun::Draw() {
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

void TCheckRun::DrawSolos() {
  for (string var : fSoloPlots) {
    string unit = GetUnit(var);

    TGraphErrors * g = new TGraphErrors();      // all data points
    TGraphErrors * g_err = new TGraphErrors();  // ErrorFlag != 0
    TGraphErrors * g_bad = new TGraphErrors();  // ok data points that don't pass check

    for(int i=0, ibad=0, ierr=0; i<nTotal; i++) {
      double val;
			val = fVarValues[var][i];

      g->SetPoint(i, i+1, val);

      if (fErrorFlags[i] != 0) {
        g_err->SetPoint(ierr, i+1, val);
        ierr++;
      }
      if (fSoloBadPoints[var].find(i) != fSoloBadPoints[var].cend()) {
        g_bad->SetPoint(ibad, i+1, val);
        ibad++;
      }
    }
		g->SetTitle((var + ";;" + unit).c_str());
    g_err->SetMarkerStyle(1.2);
    g_err->SetMarkerColor(kBlue);
    g_bad->SetMarkerStyle(1.2);
    g_bad->SetMarkerColor(kRed);

    c->cd();
    g->Draw("AP");
    g_err->Draw("P same");
    g_bad->Draw("P same");

    TAxis * ay = g->GetYaxis();
    double ymin = ay->GetXmin();
    double ymax = ay->GetXmax();

    if (nRuns > 1) {
      for (int run : fRuns) {
        cout << "DEBUG: " << fEntries[run] << endl;
        cout << "DEBUG1: " << ymin << "\t" << ymax << endl;
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
      c->Print(Form("%s_%s.png", out_name, var.c_str()));

    c->Clear();
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Solos.\n";
}

// it looks like not a good idea to draw diff plots with a few hundred thousands points
void TCheckRun::DrawComps() {
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
    for(int i=0, ibad=0, ierr; i<nOk; i++) {
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

      g1->SetPoint(i, i+1, val1);
      g2->SetPoint(i, i+1, val2);
      h_diff->SetBinContent(i+1, val1-val2);
      
      if (fErrorFlags[i] != 0) {
        g_err1->SetPoint(ierr, i+1, val1);
        g_err2->SetPoint(ierr, i+1, val2);
        ierr++;
      }
      if (fCompBadPoints[var].find(i) != fCompBadPoints[var].cend()) {
        g_bad1->SetPoint(ibad, i+1, val1);
        g_bad2->SetPoint(ibad, i+1, val2);
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

void TCheckRun::DrawCors() {
  for (pair<string, string> var : fCorPlots) {
    string xvar = var.second;
    string yvar = var.first;
    string xunit = GetUnit(xvar);
    string yunit = GetUnit(yvar);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();
    TGraphErrors * g_good = new TGraphErrors();

    for(int i=0, ibad=0, igood=0; i<nOk; i++) {
      double xval;
      double yval;
			xval = fVarValues[xvar][i];
			yval = fVarValues[yvar][i];

      g->SetPoint(i, xval, yval);
      
      if (fErrorFlags[i] == 0) {
        g_good->SetPoint(igood, xval, yval);
        igood++;
      }
      if (fCorBadPoints[var].find(i) != fCorBadPoints[var].cend()) {
        g_bad->SetPoint(ibad, xval, yval);
        ibad++;
      }
    }
		g->SetTitle((yvar + " vs " + xvar + ";" + xunit + ";" + yunit).c_str());
    g->SetMarkerStyle(1.2);
    g->SetMarkerColor(kBlue);
    g_good->SetMarkerColor(kBlack);
    g_bad->SetMarkerStyle(1.2);
    g_bad->SetMarkerColor(kRed);

    g_good->Fit("pol1");

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

    g_good->Draw("P same");
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

const char * TCheckRun::GetUnit (string var) {
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
