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
		int		fRun;
    int		fSessions;
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

		int	nPoints = 0;	// total number of events
    map<string, set<int>>									fSoloBadPoints;
    map<string, set<int>>									fCustomBadPoints;
    map<pair<string, string>, set<int>>	  fCompBadPoints;
    map<pair<string, string>, set<int>>	  fCorBadPoints;

    map<string, DATASET>				vars_buf;		// for native variables
    map<string, vector<double>> fVarValues;	// for all variables
    map<string, double>					fVarSum;		// all
    map<string, double>					fVarSum2;		// sum of square; all

    TCanvas * c;
  public:
     TCheckRun(const char*);
     ~TCheckRun();
		 void SetRun(const int run) {fRun = run;}
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
  fVars	   = fConf.GetVars();

  fSolos      = fConf.GetSolos();
  fSoloCuts   = fConf.GetSoloCuts();
  fSoloPlots  = fConf.GetSoloPlots();

  fCustoms      = fConf.GetCustoms();
  fCustomDefs   = fConf.GetCustomDefs();
  fCustomCuts   = fConf.GetCustomCuts();
  fCustomPlots  = fConf.GetCustomPlots();

  fComps      = fConf.GetComps();
  fCompCuts   = fConf.GetCompCuts();
  fCompPlots  = fConf.GetCompPlots();

  fCors	      = fConf.GetCors();
  fCorPlots   = fConf.GetCorPlots();
  fCorCuts    = fConf.GetCorCuts();

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

void TCheckRun::CheckRun() {
  // check runs against database
  glob_t globbuf;
	const char * pattern  = Form("%s/%s_%d.???.root", dir, prefix, fRun);
	glob(pattern, 0, NULL, &globbuf);
	if (globbuf.gl_pathc == 0) {
		cout << __PRETTY_FUNCTION__ << ":FATAL\t no root file for specified run " << fRun << endl;
		exit(23);
	}
	fSessions = globbuf.gl_pathc;
  globfree(&globbuf);
  cout << __PRETTY_FUNCTION__ << "INFO\t run " << fRun << " has " << fSessions << " sessions" << endl;
}

void TCheckRun::CheckVars() {
	const char * file_name = Form("%s/%s_%d.000.root", dir, prefix, fRun);
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
	for (int session=0; session<fSessions; session++) {
		const char * file_name = Form("%s/%s_%d.%03d.root", dir, prefix, fRun, session);
		TFile * f_rootfile = new TFile(file_name, "read");
		if (!f_rootfile->IsOpen()) {
			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
      f_rootfile->Close();
			continue;
		}

		cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d\n", fRun, session)
				 << Form("%s_%d.%03d.root", prefix, fRun, session) << endl;
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

		for(int n=0; n<nentries; n++) { // loop through the events
      if (n % 20000 == 0)
        cout << __PRETTY_FUNCTION__ << ":INFO\t processing " << n << " evnets!" << endl;
      if (nPoints == 1000)
        break;

			tin->GetEntry(n);
			if (ErrorFlag == 0) {
        nPoints++;
				for (set<string>::const_iterator it=fVars.cbegin(); it!=fVars.cend(); it++) {
          double val = vars_buf[*it].hw_sum;
					fVarValues[*it].push_back(val);
					fVarSum[*it]	+= val;
					fVarSum2[*it] += val * val;
				}
				for (set<string>::const_iterator it=fCustoms.cbegin(); it!=fCustoms.cend(); it++) {
					double val = get_custom_value(fCustomDefs[*it]);
					fVarValues[*it].push_back(val);
					fVarSum[*it]	+= val;
					fVarSum2[*it]	+= val * val;
				}
			}
		}
		tin->Delete();
		f_rootfile->Close();
	}
  cout << __PRETTY_FUNCTION__ << ":INFO\t read " << nPoints << " ok events in total" << endl;
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
		for (int i=0; i<nPoints; i++) {
      double val;
			val = fVarValues[var][i];

      if (i == 0) {
        mean = val;
        sigma = 0;
      }

      if ( (low  != 1024 && val < low)
        || (high != 1024 && val > high)
        || (stat != 1024 && abs(val-mean) > stat*sigma)) {
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
		for (int i=0; i<nPoints; i++) {
      double val1, val2;
			val1 = fVarValues[var1][i];
			val2 = fVarValues[var2][i];
			double diff = abs(val1 - val2);

      if ( (low  != 1024 && diff < low)
        || (high != 1024 && diff > high)
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
		for (int i=0; i<nPoints; i++) {
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

  for (vector<string>::const_iterator it=fCustomPlots.cbegin(); it!=fCustomPlots.cend(); it++)
    fSoloPlots.push_back(*it);
  DrawSolos();
  DrawComps();
  // DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << __PRETTY_FUNCTION__ << ":INFO\t done with drawing plots\n";
}

void TCheckRun::DrawSolos() {
  for (vector<string>::const_iterator it=fSoloPlots.cbegin(); it!=fSoloPlots.cend(); it++) {
    string var = *it;
    string unit = GetUnit(var);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibad=0; i<nPoints; i++) {
      double val, err=0;
			val = fVarValues[var][i];

      g->SetPoint(i, i+1, val);
      g->SetPointError(i, 0, err);

      if (fSoloBadPoints[var].find(i) != fSoloBadPoints[var].cend()) {
        g_bad->SetPoint(ibad, i+1, val);
        g_bad->SetPointError(ibad, 0, err);
        ibad++;
      }
    }
		g->SetTitle((var + ";;" + unit).c_str());
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerSize(1.2);
    g_bad->SetMarkerColor(kRed);

    // g->Fit("pol0");
    // TF1 * fit= g->GetFunction("pol0");
    // double mean_value = fit->GetParameter(0);

    // TPaveStats * st;
    c->cd();
    g->Draw("AP");
    // gPad->Update();
    // st = (TPaveStats *) g->FindObject("stats");
    // st->SetName("g_stats");
    // st->SetX2NDC(0.95); 
    // st->SetX1NDC(0.95-0.3); 
    // st->SetY2NDC(0.9);
    // st->SetY1NDC(0.8);

    g_bad->Draw("P same");

    TAxis * ay = g->GetYaxis();

    double min = ay->GetXmin();
    double max = ay->GetXmax();
    ay->SetRangeUser(min, max+(max-min)/9);

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, var.c_str()));

    c->Clear();
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Solos.\n";
}

void TCheckRun::DrawComps() {
  int MarkerStyles[] = {29, 33, 34, 31};
  for (vector<pair<string, string>>::const_iterator it=fCompPlots.cbegin(); it!=fCompPlots.cend(); it++) {
    string var1 = it->first;
    string var2 = it->second;

    string unit = GetUnit(var1);

    TGraphErrors * g1 = new TGraphErrors();
    TGraphErrors * g2 = new TGraphErrors();
    TGraphErrors * g_bad1  = new TGraphErrors();
    TGraphErrors * g_bad2  = new TGraphErrors();
    TH1F * h_diff = new TH1F("diff", "", nPoints, 0, nPoints);

    double min, max;
    for(int i=0, ibad=0; i<nPoints; i++) {
      double val1, err1=0;
      double val2, err2=0;
			val1 = fVarValues[var1][i];
			val2 = fVarValues[var2][i];
      if (i==0) 
        min = max = val1;

      if (val1 < min) min = val1;
      if (val1 > max) max = val1;
      if (val2 < min) min = val2;
      if (val2 > max) max = val2;

      g1->SetPoint(i, i+1, val1);
      g1->SetPointError(i, 0, err1);
      g2->SetPoint(i, i+1, val2);
      g2->SetPointError(i, 0, err2);
      h_diff->SetBinContent(i+1, val1-val2);
      
      if (fCompBadPoints[*it].find(i) != fCompBadPoints[*it].cend()) {
        g_bad1->SetPoint(ibad, i+1, val1);
        g_bad1->SetPointError(ibad, 0, err1);
        g_bad2->SetPoint(ibad, i+1, val2);
        g_bad2->SetPointError(ibad, 0, err2);
        ibad++;
      }
    }
    double margin = (max-min)/10;
    min -= margin;
    max += margin;

		g1->SetTitle(Form("%s & %s;;%s", var1.c_str(), var2.c_str(), unit.c_str()));
    g_bad1->SetMarkerStyle(20);
    g_bad1->SetMarkerSize(1.2);
    g_bad1->SetMarkerColor(kRed);
    g_bad2->SetMarkerStyle(20);
    g_bad2->SetMarkerSize(1.2);
    g_bad2->SetMarkerColor(kRed);

    // g1->Fit("pol0");
    // g2->Fit("pol0");

    // TPaveStats *st1, *st2;
    // map<int, TPaveStats *> sts1, sts2;
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
    // p1->Update();
    // st1 = (TPaveStats *) g1->FindObject("stats");
    // st1->SetName("g1_stats");
    g2->Draw("P same");
    // p1->Update();
    // st2 = (TPaveStats *) g2->FindObject("stats");
    // st2->SetName("g2_stats");
    g1->GetYaxis()->SetRangeUser(min, max+(max-min)/5);

    // st1->SetX2NDC(0.95);
    // st1->SetX1NDC(0.95-0.3);
    // st1->SetY2NDC(0.9);
    // st1->SetY1NDC(0.8);
    // st2->SetX2NDC(0.95-0.3);
    // st2->SetX1NDC(0.95-2*0.3);
    // st2->SetY2NDC(0.9);
    // st2->SetY1NDC(0.8);

    g_bad1->Draw("P same");
    g_bad2->Draw("P same");
    
    p2->cd();
    h_diff->SetStats(kFALSE);
    h_diff->SetFillColor(kGreen);
    h_diff->SetBarOffset(0.5);
    h_diff->SetBarWidth(1);
    h_diff->Draw("B");
    p2->Update();

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
  for (vector<pair<string, string>>::const_iterator it=fCorPlots.cbegin(); it!=fCorPlots.cend(); it++) {
    string xvar = it->second;
    string yvar = it->first;
    string xunit = GetUnit(xvar);
    string yunit = GetUnit(yvar);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibad=0; i<nPoints; i++) {
      double xval, xerr;
      double yval, yerr;
			xval = fVarValues[xvar][i];
			xerr = fVarValues[xvar][i];
			yval = fVarValues[yvar][i];
			yerr = fVarValues[yvar][i];

      g->SetPoint(i, xval, yval);
      g->SetPointError(i, xerr, yerr);
      
      if (fCorBadPoints[*it].find(i) != fCorBadPoints[*it].cend()) {
        g_bad->SetPoint(ibad, xval, yval);
        g_bad->SetPointError(ibad, xerr, yerr);
        ibad++;
      }
    }
		g->SetTitle((it->first + " vs " + it->second + ";" + xunit + ";" + yunit).c_str());
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerSize(1.2);
    g_bad->SetMarkerColor(kRed);

    g->Fit("pol1");

    TPaveStats * st;
    c->cd();
    gPad->SetRightMargin(0.05);
    g->Draw("AP");
    gPad->Update();
    st = (TPaveStats *) g->FindObject("stats");
    st->SetName("g_stats");
    st->SetX2NDC(0.95); 
    st->SetX1NDC(0.95-0.3); 
    st->SetY2NDC(0.9);
    st->SetY1NDC(0.75);

    g_bad->Draw("P same");
    
    TAxis * ay = g->GetYaxis();
    double min = ay->GetXmin();
    double max = ay->GetXmax();
    ay->SetRangeUser(min, max+(max-min)/9);
    gPad->Update();

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
