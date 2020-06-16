#ifndef TRUNWISE_H
#define TRUNWISE_H

#include <iostream>
#include <fstream>
#include <string>
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
#include "TCollection.h"
#include "TGraphErrors.h"
#include "TH1F.h"
#include "TPaveStats.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TAxis.h"
#include "TLegend.h"

#include "const.h"
#include "line.h"
#include "TConfig.h"


enum Format {pdf, png};

using namespace std;

class TRunWise {

    // ClassDe (TRunWise, 0) // check statistics

  private:
    TConfig fConf;
    Format format         = pdf;
    const char *out_name  = "runwise";
    const char *dir	      = "/chafs2/work1/apar/BMODextractor/";
    string pattern        = "dit_alldet_slopes_1X_slugxxxx.root";
    const char *tree      = "dit";
    vector<int> flips;
    int	  nRuns;
    int	  nSlugs;
    int	  nVars;
    int	  nSolos;
    int	  nComps;
    int	  nCors;
    set<int> fSlugs;
    set<string>	  fVars;
    set<string>	  fSolos;
    set< pair<string, string> >	fComps;
    set< pair<string, string> >	fCors;
    vector<string>	                fSoloPlots;
    vector< pair<string, string> >	fCompPlots;
    vector< pair<string, string> >	fCorPlots;
    map<string, VarCut>			            fSoloCuts;
    map< pair<string, string>, VarCut>	fCompCuts;
    map< pair<string, string>, VarCut>  fCorCuts;

    map<int, string> fRootFiles;
    map<string, string> fUnits;
    vector<int> fRuns;
    map<string, pair<string, string>> fVarNames;
    map<string, TLeaf *> fVarLeaves;

    map<string, set<int>>  fSoloBadRuns;
    map<pair<string, string>, set<int>>	  fCompBadRuns;
    map<pair<string, string>, set<int>>	  fCorBadRuns;
    map<string, vector<double>> fVarValues;

    TCanvas * c;
  public:
     TRunWise(const char*);
     ~TRunWise();
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutFormat(const char * f);
     // void SetFileType(const char * f);
     void SetDir(const char * d);
     void SetSlugs(set<int> slugs);
     void CheckSlugs();
     void CheckVars();
     bool CheckVar(string exp);
     void GetValues();
     void CheckValues();
     void Draw();
     void DrawSolos();
     void DrawComps();
     void DrawCors();

     // auxiliary funcitons
     const char * GetUnit(string var);
};

// ClassImp(TRunWise);

TRunWise::TRunWise(const char* config_file) :
  fConf(config_file)
{
  fConf.ParseConfFile();
  fVars	  = fConf.GetVars();
  fSolos  = fConf.GetSolos();
  fComps  = fConf.GetComps();
  fCors	  = fConf.GetCors();

  fSoloPlots  = fConf.GetSoloPlots();
  fCompPlots  = fConf.GetCompPlots();
  fCorPlots   = fConf.GetCorPlots();
  fSoloCuts   = fConf.GetSoloCuts();
  fCompCuts   = fConf.GetCompCuts();
  fCorCuts    = fConf.GetCorCuts();

  // if (fConf.GetFileType())  SetFileType(fConf.GetFileType()); // must precede the following statements
  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetPattern())   pattern = fConf.GetPattern();
  if (fConf.GetTreeName())  tree  = fConf.GetTreeName();

  gROOT->SetBatch(1);
}

TRunWise::~TRunWise() {
  cerr << __PRETTY_FUNCTION__ << ":INFO\t Release TRunWise\n";
}

void TRunWise::SetDir(const char * d) {
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

void TRunWise::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unknow output format: " << f << endl;
    exit(40);
  }
}

void TRunWise::SetSlugs(set<int> slugs) {
  for(int slug : slugs) {
    if (   (slug >= AT_START_SLUG && slug <= AT_END_SLUG)
        || (slug >= START_SLUG && slug <= END_SLUG) ) { 
      fSlugs.insert(slug);
    } else {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid slug number (" << START_SLUG << "-" << END_SLUG << "): " << slug << endl;
      continue;
    }
  }
  nSlugs = fSlugs.size();
}

void TRunWise::CheckSlugs() {
  for (set<int>::const_iterator it = fSlugs.cbegin(); it != fSlugs.cend(); ) {
    int slug = *it;
    string p_buf(pattern);
    p_buf.replace(p_buf.find("xxxx"), 4, to_string(slug));
    const char * p = Form("%s/%s", dir, p_buf.c_str());
    glob_t globbuf;
    glob(p, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << __PRETTY_FUNCTION__ << ":WARNING\t no root file for slug " << slug << ". Ignore it.\n";
      it = fSlugs.erase(it);
      continue;
    }
    fRootFiles[slug] = globbuf.gl_pathv[0];

    globfree(&globbuf);
    it++;
  }

  nSlugs = fSlugs.size();
  if (nSlugs == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t No valid slugs specified!\n";
    exit(10);
  }

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSlugs << " valid slugs specified:\n";
  for(int slug : fSlugs) {
    cout << "\t" << slug << endl;
  }
}

void TRunWise::CheckVars() {
  srand(time(NULL));
  int s = rand() % nSlugs;
  set<int>::const_iterator it_s=fSlugs.cbegin();
  for(int i=0; i<s; i++)
    it_s++;

  while (it_s != fSlugs.cend()) {
    int slug = *it_s;
    const char * file_name = fRootFiles[slug].c_str();
    TFile * f_rootfile = new TFile(file_name, "read");
    if (f_rootfile->IsOpen()) {
      TTree * tin = (TTree*) f_rootfile->Get(tree); // receive minitree

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
      
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t root file of slug: " << slug << " is broken, ignore it.\n";
    it_s = fSlugs.erase(it_s);

    if (it_s == fSlugs.cend())
      it_s = fSlugs.cbegin();
  }

  nSlugs = fSlugs.size();
  if (nSlugs == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no valid runs, aborting.\n";
    exit(10);
  }

  nVars = fVars.size();
  if (nVars == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no valid variables specified, aborting.\n";
    exit(11);
  }

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
		if (CheckVar(it->first) && CheckVar(it->second)) {
			//	&& fVarNames[it->first].second == fVarNames[it->second].second) {
			it++;
			continue;
		} 
			
    // if (fVarNames[it->first].second != fVarNames[it->second].second)
    //   cerr << __PRETTY_FUNCTION__ << ":WARNING\t different statistical types for comparison in: " << it->first << " , " << it->second << endl;
    // else 
		// 	cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid Comp variable: " << it->first << "\t" << it->second << endl;

		vector<pair<string, string>>::iterator it_p = find(fCompPlots.begin(), fCompPlots.end(), *it);
		if (it_p != fCompPlots.cend())
			fCompPlots.erase(it_p);

		map<pair<string, string>, VarCut>::const_iterator it_c = fCompCuts.find(*it);
		if (it_c != fCompCuts.cend())
			fCompCuts.erase(it_c);

		it = fComps.erase(it);
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

  nSolos = fSolos.size();
  nComps = fComps.size();
  nCors  = fCors.size();

  // cout << __PRETTY_FUNCTION__ << ":DEBUG\t " << " internal variables:\n";
  // for (string var : fVars) {
  //   cout << "\t" << var << endl;
  // }
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
}

bool TRunWise::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend()) {
    cerr << __PRETTY_FUNCTION__ << ":WARNING\t Unknown variable in: " << var << endl;
    return false;
  }

  return true;
}

void TRunWise::GetValues() {
  for (int slug : fSlugs) {
    const char * file_name = fRootFiles[slug].c_str();
    TFile f_rootfile(file_name, "read");
    if (!f_rootfile.IsOpen()) {
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
      continue;
    }

    cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read slug: %d", slug) << file_name << endl;
    TTree * tin = (TTree*) f_rootfile.Get(tree); // receive minitree
    if (! tin) {
      cerr << __PRETTY_FUNCTION__ << ":WARNING\t No such tree: " << tree << " in root file: " << file_name << endl;
      continue;
    }

    bool error = false;
    // run
    vector<const char *> run_names = {"run"};
    TBranch * b_run = NULL;
    for (const char * run_name : run_names) {
      if (!b_run)
        b_run = tin->GetBranch(run_name);
      if (b_run) 
        break;
    }
    if (!b_run) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t no run branch in tree: " << tree 
        << " of file: " << file_name << endl;
      continue;
    }
    TLeaf *l_run = (TLeaf *)b_run->GetListOfLeaves()->At(0);

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

    const int nentries = tin->GetEntries();  // number of miniruns
    for(int n=0; n<nentries; n++) { // loop through the miniruns
      tin->GetEntry(n);

      l_run->GetBranch()->GetEntry(n);
      fRuns.push_back(l_run->GetValue());
      for (string var : fVars) {
        double unit = 1;
        double value;
        string leaf = fVarNames[var].second;
        // if (var.find("asym") != string::npos) {
        //   if (leaf == "mean" || leaf == "err")
        //     unit = ppb;
        //   else if (leaf == "rms")
        //     unit = ppm;
        // }
        // else if (var.find("diff")) {
        //   if (leaf == "mean" || leaf == "err")
        //     unit = um/mm;
        //   else if (leaf == "rms")
        //     unit = mm/mm;
        // }

        fVarLeaves[var]->GetBranch()->GetEntry(n);
        value = fVarLeaves[var]->GetValue() / unit;
        fVarValues[var].push_back(value);
      }
    }

    tin->Delete();
    f_rootfile.Close();
  }
  nRuns = fRuns.size();
}

void TRunWise::CheckValues() {
  for (string solo : fSolos) {
    const double low_cut  = fSoloCuts[solo].low;
    const double high_cut = fSoloCuts[solo].high;
    const double stat_cut = fSoloCuts[solo].stability;
    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean, sigma = 0;
    for (int i=0; i<nRuns; i++) {
      double val = fVarValues[solo][i];

      if (i == 0) {
        mean = val;
        sigma = 0;
      }

      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (stat_cut != 1024 && abs(val-mean) > stat_cut*sigma)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in " << solo
             << " in run: " << fRuns[i] << endl;
        if (find(fSoloPlots.cbegin(), fSoloPlots.cend(), solo) == fSoloPlots.cend())
          fSoloPlots.push_back(solo);
        fSoloBadRuns[solo].insert(fRuns[i]);
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
    const double low_cut  = fCompCuts[comp].low;
    const double high_cut = fCompCuts[comp].high;
    for (int i=0; i<nRuns; i++) {
      double val1 = fVarValues[var1][i];
      double val2 = fVarValues[var1][i];
			double diff = abs(val1 - val2);

      if ( (low_cut  != 1024 && diff < low_cut)
        || (high_cut != 1024 && diff > high_cut)) {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Comp: " << var1 << " vs " << var2 
             << " in run: " << fRuns[i] << endl;
        if (find(fCompPlots.cbegin(), fCompPlots.cend(), comp) == fCompPlots.cend())
          fCompPlots.push_back(comp);
        fCompBadRuns[comp].insert(fRuns[i]);
      }
    }
  }

  for (pair<string, string> cor : fCors) {
    string yvar = cor.first;
    string xvar = cor.second;
    const double low_cut   = fCorCuts[cor].low;
    const double high_cut  = fCorCuts[cor].high;
    // const double 
    for (int i=0; i<nRuns; i++) {
      double xval = fVarValues[xvar][i];
      double yval = fVarValues[yvar][i];

			/*
      if () {
        cout << __PRETTY_FUNCTION__ << ":ALERT\t bad datapoint in Cor: " << yvar << " vs " << xvar 
             << " in run: " << fRuns[i] << endl;
        if (find(fCorPlots.cbegin(), fCorPlots.cend(), *it) == fCorPlots.cend())
          fCorPlots.push_back(*it);
        fCorBadRuns[*it].insert(fRuns[i]);
      }
			*/
    }
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t done with checking values\n";
}

void TRunWise::Draw() {
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
  DrawCors();

  if (format == pdf)
    c->Print(Form("%s.pdf]", out_name));

  cout << __PRETTY_FUNCTION__ << ":INFO\t done with drawing plots\n";
}

void TRunWise::DrawSolos() {
  for (string solo : fSoloPlots) {
    string unit = GetUnit(solo);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * gc = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibad=0; i<nRuns; i++) {
      double val, err=0;
      val = fVarValues[solo][i];
      g->SetPoint(i, i+1, val);
      g->SetPointError(i, 0, err);
      gc->SetPoint(i, i+1, val);
      gc->SetPointError(i, 0, err);

      if (fSoloBadRuns[solo].find(fRuns[i]) != fSoloBadRuns[solo].cend()) {
        g_bad->SetPoint(ibad, i+1, val);
        g_bad->SetPointError(ibad, 0, err);
        ibad++;
      }
    }
    g->GetXaxis()->SetRangeUser(0, nRuns+1);
    g->SetTitle((solo + ";;" + unit).c_str());
    gc->SetMarkerStyle(20);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerSize(1.2);
    g_bad->SetMarkerColor(kRed);

    g->Fit("pol0");
    TF1 * fit= g->GetFunction("pol0");

    c->cd();
    g->Draw("AP");

    gc->Draw("P same");
    g_bad->Draw("P same");

    TAxis * ax = g->GetXaxis();

    ax->SetNdivisions(-(nRuns+1));
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    // ax->SetLabelOffset(0.02);
    for (int i=0; i<=nRuns; i++) {
      ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d", fRuns[i]));
    }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, solo.c_str()));

    c->Clear();
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Solos.\n";
}

void TRunWise::DrawComps() {
  int MarkerStyles[] = {29, 33, 34, 31};
  int color1 = 48, color2 = 38;
  for (pair<string, string> comp : fCompPlots) {
    string var1 = comp.first;
    string var2 = comp.second;
    string branch1 = fVarNames[var1].first;
    string branch2 = fVarNames[var2].first;
    string name1 = branch1.substr(branch1.find_last_of('_')+1);
    string name2 = branch2.substr(branch2.find_last_of('_')+1);

    string unit = GetUnit(var1);

    TGraphErrors * g1 = new TGraphErrors();
    TGraphErrors * g2 = new TGraphErrors();
    TGraphErrors * gc1 = new TGraphErrors();
    TGraphErrors * gc2 = new TGraphErrors();
    TGraphErrors * g_bad1  = new TGraphErrors();
    TGraphErrors * g_bad2  = new TGraphErrors();
    map<int, TGraphErrors *> g_flips1;
    map<int, TGraphErrors *> g_flips2;
    vector<string> lnames1;
    vector<string> lnames2;
    TH1F * h_diff = new TH1F("diff", "", nRuns, 0, nRuns);

    double min, max;
    for(int i=0, ibad=0; i<nRuns; i++) {
      double val1, err1=0;
      double val2, err2=0;
      val1 = fVarValues[var1][i];
      val2 = fVarValues[var2][i];
      if (i==0) 
        min = max = val1;

      if ((val1-err1) < min) min = val1-err1;
      if ((val1+err1) > max) max = val1+err1;
      if ((val2-err2) < min) min = val2-err2;
      if ((val2+err2) > max) max = val2+err2;

      g1->SetPoint(i, i+1, val1);
      g1->SetPointError(i, 0, err1);
      g2->SetPoint(i, i+1, val2);
      g2->SetPointError(i, 0, err2);
      gc1->SetPoint(i, i+1, val1);
      gc1->SetPointError(i, 0, err1);
      gc2->SetPoint(i, i+1, val2);
      gc2->SetPointError(i, 0, err2);
      h_diff->SetBinContent(i+1, val1-val2);
      
      if (fCompBadRuns[comp].find(fRuns[i]) != fCompBadRuns[comp].cend()) {
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

    g1->GetXaxis()->SetRangeUser(0, nRuns+1);
    h_diff->GetXaxis()->SetRangeUser(0, nRuns+1);

    g1->SetTitle(Form("#color[%d]{%s} & #color[%d]{%s};;%s", color1, var1.c_str(), color2, var2.c_str(), unit.c_str()));
    gc1->SetMarkerStyle(23);
    gc2->SetMarkerStyle(22);
    gc1->SetMarkerColor(color1);
    gc2->SetMarkerColor(color2);
    g_bad1->SetMarkerStyle(20);
    g_bad1->SetMarkerSize(1.2);
    g_bad1->SetMarkerColor(kRed);
    g_bad2->SetMarkerStyle(20);
    g_bad2->SetMarkerSize(1.2);
    g_bad2->SetMarkerColor(kRed);

    g1->Fit("pol0");
    g2->Fit("pol0");

    TPaveStats *st1, *st2;
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
    g1->GetXaxis()->SetNdivisions(-(nRuns+1));
    g1->Draw("AP");
    p1->Update();
    st1 = (TPaveStats *) g1->FindObject("stats");
    st1->SetName("g1_stats");
    g2->Draw("P same");
    p1->Update();
    st2 = (TPaveStats *) g2->FindObject("stats");
    st2->SetName("g2_stats");
    g1->GetYaxis()->SetRangeUser(min, max+(max-min)/5);

    st1->SetX2NDC(0.45);
    st1->SetX1NDC(0.1);
    st1->SetTextColor(color1);
    st2->SetTextColor(color2);

    gc1->Draw("P same");
    gc2->Draw("P same");
    g_bad1->Draw("P same");
    g_bad2->Draw("P same");
    
    p2->cd();
    h_diff->SetStats(kFALSE);
    h_diff->SetFillColor(kGreen);
    h_diff->SetBarOffset(0.5);
    h_diff->SetBarWidth(1);
    h_diff->Draw("B");
    TAxis * ax = h_diff->GetXaxis();
    ax->SetNdivisions(-(nRuns+1));
    ax->ChangeLabel(1, -1, 0);  // erase first label
    ax->ChangeLabel(-1, -1, 0); // erase last label
    // ax->SetLabelOffset(0.02);
    for (int i=0; i<=nRuns; i++) {
      ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d", fRuns[i]));
    }
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

void TRunWise::DrawCors() {
  for (pair<string, string> cor : fCorPlots) {
    string xvar = cor.second;
    string yvar = cor.first;
    string xbranch = fVarNames[xvar].first;
    string ybranch = fVarNames[yvar].first;
    string xunit = GetUnit(xvar);
    string yunit = GetUnit(yvar);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * gc = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibad=0; i<nRuns; i++) {
      double xval, xerr=0;
      double yval, yerr=0;
      xval = fVarValues[xvar][i];
      yval = fVarValues[yvar][i];

      g->SetPoint(i, xval, yval);
      g->SetPointError(i, xerr, yerr);
      gc->SetPoint(i, xval, yval);
      gc->SetPointError(i, xerr, yerr);
      
      if (fCorBadRuns[cor].find(fRuns[i]) != fCorBadRuns[cor].cend()) {
        g_bad->SetPoint(ibad, xval, yval);
        g_bad->SetPointError(ibad, xerr, yerr);
        ibad++;
      }
    }
    g->SetTitle((cor.first + " vs " + cor.second + ";" + xunit + ";" + yunit).c_str());
    gc->SetMarkerStyle(20);
    g_bad->SetMarkerStyle(20);
    g_bad->SetMarkerSize(1.2);
    g_bad->SetMarkerColor(kRed);

    g->Fit("pol1");

    c->cd();
    gPad->SetRightMargin(0.05);
    g->Draw("AP");
    gc->Draw("P same");
    g_bad->Draw("P same");
    
    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s_vs_%s.png", out_name, xvar.c_str(), yvar.c_str()));
    c->Clear();
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t Done with drawing Correlations.\n";
}

const char * TRunWise::GetUnit (string var) {
  string branch = fVarNames[var].first;
  string leaf   = fVarNames[var].second;
  if (branch.find("asym") != string::npos) {
    if (leaf == "mean")
      return "ppb";
    else if (leaf == "rms")
      return "ppm";
  } else if (branch.find("diff") != string::npos) {
    if (leaf == "mean")
      return "nm";
    else if (leaf == "rms")
      return "um";
  } else {
    return "";
  }
	return "";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
