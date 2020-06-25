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
enum IV {run, cycle}; // draw along eigher run number or cycle number

using namespace std;

class TRunWise {

    // ClassDe (TRunWise, 0) // check statistics

  private:
    TConfig fConf;
    Format format         = pdf;
    IV iv                 = run;  // default value: run
    const char *iv_name   = "run";
    const char *out_name  = "runwise";
    const char *dir	      = "/chafs2/work1/apar/BMODextractor/";
    string pattern        = "dit_alldet_slopes_1X_slugxxxx.root";
    const char *tree      = "dit";
    int	  nIvs;
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
    vector<int> fIvs; // x variable: run or cyclenum
    map<string, pair<string, string>> fVarNames;
    map<string, TLeaf *> fVarLeaves;

    map<string, set<int>>  fSoloBadIvs;
    map<pair<string, string>, set<int>>	  fCompBadIvs;
    map<pair<string, string>, set<int>>	  fCorBadIvs;
    map<string, vector<double>> fVarValues;

    TCanvas * c;
  public:
     TRunWise(const char*);
     ~TRunWise();
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutFormat(const char * f);
     // void SetFileType(const char * f);
     void SetIV(const char * var) { if (Contain(var, "cycle")) {iv = cycle; iv_name = "cycle";} }
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
  cerr << INFO << "Release TRunWise" << ENDL;
}

void TRunWise::SetDir(const char * d) {
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

void TRunWise::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << FATAL << "Unknow output format: " << f << ENDL;
    exit(40);
  }
}

void TRunWise::SetSlugs(set<int> slugs) {
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

void TRunWise::CheckSlugs() {
  for (set<int>::const_iterator it = fSlugs.cbegin(); it != fSlugs.cend(); ) {
    int slug = *it;
    string p_buf(pattern);
    p_buf.replace(p_buf.find("xxxx"), 4, to_string(slug));
    const char * p = Form("%s/%s", dir, p_buf.c_str());
    glob_t globbuf;
    glob(p, 0, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
      cout << WARNING << "no root file for slug " << slug << ". Ignore it." << ENDL;
      it = fSlugs.erase(it);
      continue;
    }
    fRootFiles[slug] = globbuf.gl_pathv[0];

    globfree(&globbuf);
    it++;
  }

  nSlugs = fSlugs.size();
  if (nSlugs == 0) {
    cerr << FATAL << "No valid slugs specified!" << ENDL;
    exit(10);
  }

  cout << INFO << "" << nSlugs << " valid slugs specified:" << ENDL;
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
        cout << INFO << "use file to check vars: " << file_name << ENDL;

        TObjArray * l_var = tin->GetListOfBranches();
        for (string var : fVars) {
          size_t dot_pos = var.find_first_of('.');
          string branch = (dot_pos == string::npos) ? var : var.substr(0, dot_pos);
          string leaf = (dot_pos == string::npos) ? "" : var.substr(dot_pos+1);

          TBranch * bbuf = (TBranch *) l_var->FindObject(branch.c_str());
          if (!bbuf) {
            cerr << WARNING << "No such branch: " << branch << " in var: " << var << ENDL;
            cout << DEBUG << "List of valid branches:" << ENDL;
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
            cerr << WARNING << "No such leaf: " << leaf << " in var: " << var << ENDL;
            cout << DEBUG << "List of valid leaves:" << ENDL;
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
      
    cerr << WARNING << "root file of slug: " << slug << " is broken, ignore it." << ENDL;
    it_s = fSlugs.erase(it_s);

    if (it_s == fSlugs.cend())
      it_s = fSlugs.cbegin();
  }

  nSlugs = fSlugs.size();
  if (nSlugs == 0) {
    cerr << FATAL << "no valid slug, aborting." << ENDL;
    exit(10);
  }

  nVars = fVars.size();
  if (nVars == 0) {
    cerr << FATAL << "no valid variables specified, aborting." << ENDL;
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

			cerr << WARNING << "Invalid solo variable: " << *it << ENDL;
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
    //   cerr << WARNING << "different statistical types for comparison in: " << it->first << " , " << it->second << ENDL;
    // else 
		// 	cerr << WARNING << "Invalid Comp variable: " << it->first << "\t" << it->second << ENDL;

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

			cerr << WARNING << "Invalid Cor variable: " << it->first << "\t" << it->second << ENDL;
      it = fCors.erase(it);
		} else
      it++;
  }

  nSolos = fSolos.size();
  nComps = fComps.size();
  nCors  = fCors.size();

  // cout << DEBUG << "" << " internal variables:" << ENDL;
  // for (string var : fVars) {
  //   cout << "\t" << var << endl;
  // }
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
}

bool TRunWise::CheckVar(string var) {
  if (fVars.find(var) == fVars.cend()) {
    cerr << WARNING << "Unknown variable in: " << var << ENDL;
    return false;
  }

  return true;
}

void TRunWise::GetValues() {
  for (int slug : fSlugs) {
    const char * file_name = fRootFiles[slug].c_str();
    TFile f_rootfile(file_name, "read");
    if (!f_rootfile.IsOpen()) {
      cerr << WARNING << "Can't open root file: " << file_name << ENDL;
      continue;
    }

    cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read slug: %d", slug) << file_name << ENDL;
    TTree * tin = (TTree*) f_rootfile.Get(tree); // receive minitree
    if (! tin) {
      cerr << WARNING << "No such tree: " << tree << " in root file: " << file_name << ENDL;
      continue;
    }

    bool error = false;
    // run
    set<const char *> iv_names;
    if (iv == cycle) {
      iv_names.insert("cycle");
      iv_names.insert("cyclenum");
      iv_names.insert("cycleNum");
    } else {
      iv_names.insert("run");
      iv_names.insert("runnum");
    }
    TBranch * b_iv = NULL;
    for (const char * iname : iv_names) {
      if (!b_iv)
        b_iv = tin->GetBranch(iname);
      if (b_iv) 
        break;
    }
    if (!b_iv) {
      cerr << ERROR << "no " << iv_name << " branch in tree: " << tree 
        << " of file: " << file_name << ENDL;
      continue;
    }
    TLeaf *l_iv = (TLeaf *)b_iv->GetListOfLeaves()->At(0);

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

    const int nentries = tin->GetEntries();  
    for(int n=0; n<nentries; n++) { 
      tin->GetEntry(n);

      l_iv->GetBranch()->GetEntry(n);
      fIvs.push_back(l_iv->GetValue());
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
  nIvs = fIvs.size();
}

void TRunWise::CheckValues() {
  for (string solo : fSolos) {
    const double low_cut  = fSoloCuts[solo].low;
    const double high_cut = fSoloCuts[solo].high;
    const double burp_cut = fSoloCuts[solo].burplevel;
    double sum  = 0;
    double sum2 = 0;  // sum of square
    double mean;
    // double sigma = 0;
    for (int i=0; i<nIvs; i++) {
      double val = fVarValues[solo][i];

      if (i == 0) {
        mean = val;
        // sigma = 0;
      }

      if ( (low_cut  != 1024 && val < low_cut)
        || (high_cut != 1024 && val > high_cut)
        || (burp_cut != 1024 && abs(val-mean) > burp_cut)) {
        cout << ALERT << "bad datapoint in " << solo
             << " in run: " << fIvs[i] << ENDL;
        if (find(fSoloPlots.cbegin(), fSoloPlots.cend(), solo) == fSoloPlots.cend())
          fSoloPlots.push_back(solo);
        fSoloBadIvs[solo].insert(fIvs[i]);
      }

      sum  += val;
      sum2 += val*val;
      mean = sum/(i+1);
      // sigma = sqrt(sum2/(i+1) - pow(mean, 2));
    }
  }

  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
    const double low_cut  = fCompCuts[comp].low;
    const double high_cut = fCompCuts[comp].high;
    for (int i=0; i<nIvs; i++) {
      double val1 = fVarValues[var1][i];
      double val2 = fVarValues[var1][i];
			double diff = abs(val1 - val2);

      if ( (low_cut  != 1024 && diff < low_cut)
        || (high_cut != 1024 && diff > high_cut)) {
        cout << ALERT << "bad datapoint in Comp: " << var1 << " vs " << var2 
             << " in run: " << fIvs[i] << ENDL;
        if (find(fCompPlots.cbegin(), fCompPlots.cend(), comp) == fCompPlots.cend())
          fCompPlots.push_back(comp);
        fCompBadIvs[comp].insert(fIvs[i]);
      }
    }
  }

  for (pair<string, string> cor : fCors) {
    string yvar = cor.first;
    string xvar = cor.second;
    const double low_cut   = fCorCuts[cor].low;
    const double high_cut  = fCorCuts[cor].high;
    // const double 
    for (int i=0; i<nIvs; i++) {
      double xval = fVarValues[xvar][i];
      double yval = fVarValues[yvar][i];

			/*
      if () {
        cout << ALERT << "bad datapoint in Cor: " << yvar << " vs " << xvar 
             << " in run: " << fIvs[i] << ENDL;
        if (find(fCorPlots.cbegin(), fCorPlots.cend(), *it) == fCorPlots.cend())
          fCorPlots.push_back(*it);
        fCorBadIvs[*it].insert(fIvs[i]);
      }
			*/
    }
  }
  cout << INFO << "done with checking values" << ENDL;
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

  cout << INFO << "done with drawing plots" << ENDL;
}

void TRunWise::DrawSolos() {
  for (string solo : fSoloPlots) {
    string unit = GetUnit(solo);

    TGraphErrors * g = new TGraphErrors();
    TGraphErrors * gc = new TGraphErrors();
    TGraphErrors * g_bad  = new TGraphErrors();

    for(int i=0, ibad=0; i<nIvs; i++) {
      double val, err=0;
      val = fVarValues[solo][i];
      g->SetPoint(i, fIvs[i], val);
      // g->SetPointError(i, 0, err);
      gc->SetPoint(i, fIvs[i], val);
      // gc->SetPointError(i, 0, err);

      if (fSoloBadIvs[solo].find(fIvs[i]) != fSoloBadIvs[solo].cend()) {
        g_bad->SetPoint(ibad, fIvs[i], val);
        // g_bad->SetPointError(ibad, 0, err);
        ibad++;
      }
    }
    g->GetXaxis()->SetRangeUser(0, nIvs+1);
    g->SetTitle((solo + ";" + iv_name + ";" + unit).c_str());
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

    // TAxis * ax = g->GetXaxis();

    // ax->SetNdivisions(-(nIvs+1));
    // ax->ChangeLabel(1, -1, 0);  // erase first label
    // ax->ChangeLabel(-1, -1, 0); // erase last label
    // // ax->SetLabelOffset(0.02);
    // for (int i=0; i<=nIvs; i++) {
    //   ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d", fIvs[i]));
    // }

    c->Modified();
    if (format == pdf)
      c->Print(Form("%s.pdf", out_name));
    else if (format == png)
      c->Print(Form("%s_%s.png", out_name, solo.c_str()));

    c->Clear();
  }
  cout << INFO << "Done with drawing Solos." << ENDL;
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
    TH1F * h_diff = new TH1F("diff", "", nIvs, 0, nIvs);

    double min, max;
    for(int i=0, ibad=0; i<nIvs; i++) {
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

      g1->SetPoint(i, fIvs[i], val1);
      // g1->SetPointError(i, 0, err1);
      g2->SetPoint(i, fIvs[i], val2);
      // g2->SetPointError(i, 0, err2);
      gc1->SetPoint(i, fIvs[i], val1);
      // gc1->SetPointError(i, 0, err1);
      gc2->SetPoint(i, fIvs[i], val2);
      // gc2->SetPointError(i, 0, err2);
      h_diff->Fill(fIvs[i], val1-val2);
      
      if (fCompBadIvs[comp].find(fIvs[i]) != fCompBadIvs[comp].cend()) {
        g_bad1->SetPoint(ibad, i+1, val1);
        // g_bad1->SetPointError(ibad, 0, err1);
        g_bad2->SetPoint(ibad, i+1, val2);
        // g_bad2->SetPointError(ibad, 0, err2);
        ibad++;
      }
    }
    double margin = (max-min)/10;
    min -= margin;
    max += margin;

    g1->GetXaxis()->SetRangeUser(0, nIvs+1);
    h_diff->GetXaxis()->SetRangeUser(0, nIvs+1);

    g1->SetTitle(Form("#color[%d]{%s} & #color[%d]{%s};%s;%s", color1, var1.c_str(), color2, var2.c_str(), iv_name, unit.c_str()));
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
    g1->GetXaxis()->SetNdivisions(-(nIvs+1));
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
    // TAxis * ax = h_diff->GetXaxis();
    // ax->SetNdivisions(-(nIvs+1));
    // ax->ChangeLabel(1, -1, 0);  // erase first label
    // ax->ChangeLabel(-1, -1, 0); // erase last label
    // // ax->SetLabelOffset(0.02);
    // for (int i=0; i<=nIvs; i++) {
    //   ax->ChangeLabel(i+2, 90, -1, 32, -1, -1, Form("%d", fIvs[i]));
    // }
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
  cout << INFO << "Done with drawing Comparisons." << ENDL;
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

    for(int i=0, ibad=0; i<nIvs; i++) {
      double xval, xerr=0;
      double yval, yerr=0;
      xval = fVarValues[xvar][i];
      yval = fVarValues[yvar][i];

      g->SetPoint(i, xval, yval);
      // g->SetPointError(i, xerr, yerr);
      gc->SetPoint(i, xval, yval);
      // gc->SetPointError(i, xerr, yerr);
      
      if (fCorBadIvs[cor].find(fIvs[i]) != fCorBadIvs[cor].cend()) {
        g_bad->SetPoint(ibad, xval, yval);
        // g_bad->SetPointError(ibad, xerr, yerr);
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
  cout << INFO << "Done with drawing Correlations." << ENDL;
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
