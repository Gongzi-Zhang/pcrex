#ifndef TMULPLOT_H
#define TMULPLOT_H

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

using namespace std;

class TMulPlot {

    // ClassDe (TMulPlot, 0) // mul plots

  private:
    Format format     = pdf; // default pdf output
    const char *out_name = "mulplot";
    // root file: $dir/$pattern
    const char *dir   = "/adaqfs/home/apar/PREX/prompt/results/";
    string pattern    = "prexPrompt_xxxx_???_regress_postpan.root";
    const char *tree  = "reg";
    TCut cut          = "ok_cut";
    bool logy         = false;
    vector<pair<long, long>> ecuts;

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

    set<string>         fSolos;
    map<string, VarCut> fSoloCuts;	// use the low and high cut as x range

    set<string>					fCustoms;
    map<string, VarCut>	fCustomCuts;
		map<string, Node *>	fCustomDefs;

    set<pair<string, string>>         fComps;
    map<pair<string, string>, VarCut>	fCompCuts;

    set<pair<string, string>>         fSlopes;
    map<pair<string, string>, VarCut> fSlopeCuts;

    set<pair<string, string>>         fCors;
    map<pair<string, string>, VarCut> fCorCuts;

    map<int, vector<string>> fRootFiles;
    map<int, int> fSigns;
    map<pair<string, string>, pair<int, int>>	  fSlopeIndexes;
    map<string, pair<string, string>> fVarNames;
    map<string, TLeaf *> fVarLeaves;
    map<string, double>  vars_buf;
    // double slopes_buf[ROWS][COLS];
    // double slopes_err_buf[ROWS][COLS];
    int      rows, cols;
    double * slopes_buf;
    double * slopes_err_buf;

    map<string, TH1F *>    fSoloHists;
    map<pair<string, string>, pair<TH1F *, TH1F *>>    fCompHists;
    // map<pair<string, string>, TH1F *>    fSlopeHists;
    // map<pair<string, string>, TH1F *>    fCorHists;

  public:
     TMulPlot(const char* confif_file, const char* run_list = NULL);
     ~TMulPlot();
     void Draw();
     void SetOutName(const char * name) {if (name) out_name = name;}
     void SetOutFormat(const char * f);
     void SetDir(const char * d);
     void SetLogy(bool log) {logy = log;}
     void SetSlugs(set<int> slugs);
     void SetRuns(set<int> runs);
     void CheckRuns();
     void CheckVars();
     bool CheckCustomVar(Node * node);
     void GetValues();
     bool CheckEntryCut(const long entry);
		 double get_custom_value(Node * node);
     void DrawHistogram();
};

// ClassImp(TMulPlot);

TMulPlot::TMulPlot(const char* config_file, const char* run_list) :
  fConf(config_file, run_list)
{
  fConf.ParseConfFile();
  fRuns = fConf.GetRuns();
  nRuns = fRuns.size();
  fVars	= fConf.GetVars();
  nVars = fVars.size();

  fSolos    = fConf.GetSolos();
  fSoloCuts = fConf.GetSoloCuts();
  nSolos    = fSolos.size();

  fCustoms    = fConf.GetCustoms();
  fCustomDefs = fConf.GetCustomDefs();
  fCustomCuts = fConf.GetCustomCuts();
  nCustoms    = fCustoms.size();

  fComps    = fConf.GetComps();
  fCompCuts = fConf.GetCompCuts();
  nComps    = fComps.size();

  fSlopes = fConf.GetSlopes();
  nSlopes = fSlopes.size();
  fSlopeCuts  = fConf.GetSlopeCuts();

  fCors	    = fConf.GetCors();
  fCorCuts  = fConf.GetCorCuts();

  if (fConf.GetDir())       SetDir(fConf.GetDir());
  if (fConf.GetPattern())   pattern = fConf.GetPattern();
  if (fConf.GetTreeName())  tree    = fConf.GetTreeName();
  if (fConf.GetTreeCut())   cut     = fConf.GetTreeCut();
  logy  = fConf.GetLogy();
  ecuts = fConf.GetEntryCuts();

  gROOT->SetBatch(1);
}

TMulPlot::~TMulPlot() {
  cout << __PRETTY_FUNCTION__ << ":INFO\t End of TMulPlot\n";
}

void TMulPlot::Draw() {
  cout << __PRETTY_FUNCTION__ << ":INFO\t draw mul plots of" << endl
       << "\tin directory: " << dir << endl
       << "\tfrom files: " << pattern << endl
       << "\tuse tree: " << tree << endl;

  CheckRuns();
  CheckVars();
  GetValues();
  DrawHistogram();
}

void TMulPlot::SetDir(const char * d) {
  struct stat info;
  if (stat(d, &info) != 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t can't access specified dir: " << dir << endl;
    exit(30);
  } else if ( !(info.st_mode & S_IFDIR)) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t not a dir: " << dir << endl;
    exit(31);
  }
  dir = d;
}

void TMulPlot::SetOutFormat(const char * f) {
  if (strcmp(f, "pdf") == 0) {
    format = pdf;
  } else if (strcmp(f, "png") == 0) {
    format = png;
  } else {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unknow output format: " << f << endl;
    exit(40);
  }
}

void TMulPlot::SetSlugs(set<int> slugs) {
  for(int slug : slugs) {
    if (slug < START_SLUG || slug > END_SLUG) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid slug number (" << START_SLUG << "-" << END_SLUG << "): " << slug << endl;
      continue;
    }
    fSlugs.insert(slug);
  }
  nSlugs = fSlugs.size();
}

void TMulPlot::SetRuns(set<int> runs) {
  for(int run : runs) {
    if (run < START_RUN || run > END_RUN) {
      cerr << __PRETTY_FUNCTION__ << ":ERROR\t Invalid run number (" << START_RUN << "-" << END_RUN << "): " << run << endl;
      continue;
    }
    fRuns.insert(run);
  }
  nRuns = fRuns.size();
}

void TMulPlot::CheckRuns() {
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
      cout << __PRETTY_FUNCTION__ << ":WARNING\t no root file for run " << run << ". Ignore it.\n";
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
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t No valid runs specified!\n";
    EndConnection();
    exit(10);
  }

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nRuns << " valid runs specified:\n";
  for(int run : fRuns) {
    cout << "\t" << run << endl;
  }

  fSigns = GetSign(fRuns); // sign corrected
  EndConnection();
}

void TMulPlot::CheckVars() {
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

        /*  FIXME: no slope now
        if (nSlopes>0) {
          rows = l_dv->size();
          cols = l_iv->size();
          slopes_buf = new double[rows*cols];
          slopes_err_buf = new double[rows*cols];
          // if (rows != ROWS || cols != COLS) {
          //   cerr << __PRETTY_FUNCTION__ << ":FATAL\t Unmatched slope array size: " << rows << "x" << cols << " in run: " << run << endl;
          //   exit(20);
          // }
          bool error_dv_flag = false;
          bool error_iv_flag = false;
          for (set<pair<string, string>>::const_iterator it=fSlopes.cbegin(); it != fSlopes.cend(); ) {
            string dv = it->first;
            string iv = it->second;
            vector<TString>::const_iterator it_dv = find(l_dv->cbegin(), l_dv->cend(), dv);
            vector<TString>::const_iterator it_iv = find(l_iv->cbegin(), l_iv->cend(), iv);
            if (it_dv == l_dv->cend()) {
              cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid dv name for slope: " << dv << endl;
              it = fSlopes.erase(it);
              error_dv_flag = true;
              continue;
            }
            if (it_iv == l_iv->cend()) {
              cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid iv name for slope: " << iv << endl;
              it = fSlopes.erase(it);
              error_iv_flag = true;
              continue;
            }
            fSlopeIndexes[*it] = make_pair(it_dv-l_dv->cbegin(), it_iv-l_iv->cbegin());
            it++;
          }
          if (error_dv_flag) {
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid dv names:\n";
            for (vector<TString>::const_iterator it = l_dv->cbegin(); it != l_dv->cend(); it++) 
              cout << "\t" << (*it).Data() << endl;
          }
          if (error_iv_flag) {
            cout << __PRETTY_FUNCTION__ << ":DEBUG\t List of valid dv names:\n";
            for (vector<TString>::const_iterator it = l_iv->cbegin(); it != l_iv->cend(); it++) 
              cout << "\t" << (*it).Data() << endl;
          }
        }
        */
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

  nRuns = fRuns.size();
  if (nRuns == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no valid runs, aborting.\n";
    exit(10);
  }

  nVars = fVars.size();
  nSlopes = fSlopes.size();
  if (nVars == 0 && nSlopes == 0) {
    cerr << __PRETTY_FUNCTION__ << ":FATAL\t no valid variables specified, aborting.\n";
    exit(11);
  }

  for (set<string>::const_iterator it=fSolos.cbegin(); it!=fSolos.cend();) {
    if (fVars.find(*it) == fVars.cend()) {
			map<string, VarCut>::const_iterator it_c = fSoloCuts.find(*it);
			if (it_c != fSoloCuts.cend())
				fSoloCuts.erase(it_c);

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid solo variable: " << *it << endl;
      it = fSolos.erase(it);
		} else
      it++;
  }

  for (set<pair<string, string>>::const_iterator it=fComps.cbegin(); it!=fComps.cend(); ) {
    if (fVars.find(it->first) == fVars.cend() || fVars.find(it->second) == fVars.cend()) {
			map<pair<string, string>, VarCut>::const_iterator it_c = fCompCuts.find(*it);
			if (it_c != fCompCuts.cend())
				fCompCuts.erase(it_c);

			cerr << __PRETTY_FUNCTION__ << ":WARNING\t Invalid Comp variable: " << it->first << "\t" << it->second << endl;
      it = fComps.erase(it);
		} else 
			it++;
  }

  for (set<pair<string, string>>::const_iterator it=fCors.cbegin(); it!=fCors.cend(); ) {
    if (fVars.find(it->first) == fVars.cend() || fVars.find(it->second) == fVars.cend()) {
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

  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSolos << " valid solo variables specified:\n";
  for(string solo : fSolos) {
    cout << "\t" << solo << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nComps << " valid comparisons specified:\n";
  for(pair<string, string> comp : fComps) {
    cout << "\t" << comp.first << " , " << comp.second << endl;
  }
  cout << __PRETTY_FUNCTION__ << ":INFO\t " << nSlopes << " valid slopes specified:\n";
  for(pair<string, string> slope : fSlopes) {
    cout << "\t" << slope.first << " : " << slope.second << endl;
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

bool TMulPlot::CheckCustomVar(Node * node) {
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

void TMulPlot::GetValues() {
  // initialize histogram
  for (string solo : fSolos) {
    double min = -100;
    double max = 100;
    const char * unit = "";
    if (solo.find("asym") != string::npos) {
      unit = "ppm";
      min  = -3000;
      max  = 3000;
    }
    else if (solo.find("diff") != string::npos) {
      unit = "um";
      min  = -50;
      max  = 50;
    } 

    if (fSoloCuts[solo].low != 1024)
      min = fSoloCuts[solo].low;
    if (fSoloCuts[solo].high != 1024)
      max = fSoloCuts[solo].high;
    
    fSoloHists[solo] = new TH1F(solo.c_str(), Form("%s;%s", solo.c_str(), unit), 100, min, max);
  }

  for (string custom : fCustoms) {
    double min = -100;
    double max = 100;
    const char * unit = "";
    if (custom.find("asym") != string::npos) {
      unit = "ppm";
      min  = -3000;
      max  = 3000;
    }
    else if (custom.find("diff") != string::npos) {
      unit = "um";
      min  = -50;
      max  = 50;
    } 

    if (fCustomCuts[custom].low != 1024)
      min = fCustomCuts[custom].low;
    if (fCustomCuts[custom].high != 1024)
      max = fCustomCuts[custom].high;
    
    // !!! add it to solo histogram
    fSoloHists[custom] = new TH1F(custom.c_str(), Form("%s;%s", custom.c_str(), unit), 100, min, max);
  }

  for (pair<string, string> comp : fComps) {
    string var1 = comp.first;
    string var2 = comp.second;
    double min = -100;
    double max = 100;
    const char * unit = "";
    if (var1.find("asym") != string::npos) {
      unit = "ppm";
      min  = -3000;
      max  = 3000;
    }
    else if (var1.find("diff") != string::npos) {
      unit = "um";
      min  = -50;
      max  = 50;
    } 

    if (fCompCuts[comp].low != 1024)
      min = fCompCuts[comp].low;
    if (fCompCuts[comp].high != 1024)
      max = fCompCuts[comp].high;

    size_t h = hash<string>{}(var1+var2);
    fCompHists[comp].first  = new TH1F(Form("%s_%ld", var1.c_str(), h), Form("%s;%s", var1.c_str(), unit), 100, min, max);
    fCompHists[comp].second = new TH1F(Form("%s_%ld", var2.c_str(), h), Form("%s;%s", var2.c_str(), unit), 100, min, max);
  }

  long total = 0;
  long ok = 0;
  for (int run : fRuns) {
    size_t sessions = fRootFiles[run].size();
    for (size_t session=0; session < sessions; session++) {
      const char *file_name = fRootFiles[run][session].c_str();
      TFile f_rootfile(file_name, "read");
      if (!f_rootfile.IsOpen()) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t Can't open root file: " << file_name << endl;
        continue;
      }

      cout << __PRETTY_FUNCTION__ << Form(":INFO\t Read run: %d, session: %03d: ", run, session)
           << file_name << endl;
      TTree * tin = (TTree*) f_rootfile.Get(tree);
      if (! tin) {
        cerr << __PRETTY_FUNCTION__ << ":WARNING\t No " << tree << " tree in root file: " << file_name << endl;
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

      // if (nSlopes > 0) { // FIXME no slope now
      //   tin->SetBranchAddress("coeff", slopes_buf);
      //   tin->SetBranchAddress("err_coeff", slopes_err_buf);
      // }

      const int N = tin->Draw(">>elist", cut, "entrylist");
      TEntryList *elist = (TEntryList*) gDirectory->Get("elist");
      // tin->SetEntryList(elist);
      for(int n=0; n<N; n++) {
        if (n%10000 == 0)
          cout << __PRETTY_FUNCTION__ << ":INFO\t processing " << n << " event\n";

        const int en = elist->GetEntry(n);
        if (CheckEntryCut(total+en))
          continue;

        ok++;
        for (string var : fVars) {
          fVarLeaves[var]->GetBranch()->GetEntry(en);
          double val = fVarLeaves[var]->GetValue();

          double unit = 1;
          if (var.find("asym") != string::npos)
            unit = ppm;
          else if (var.find("diff") != string::npos)
            unit = um/mm; // japan output has a unit of mm

          val *= fSigns[run];
          val /= unit;
          vars_buf[var] = val;
        }
        for (string custom : fCustoms) {
          double val = get_custom_value(fCustomDefs[custom]);
          fSoloHists[custom]->Fill(val);
          vars_buf[custom] = val;
        }
        for (string solo : fSolos) {
          fSoloHists[solo]->Fill(vars_buf[solo]);
        }
        for (pair<string, string> comp : fComps) {
          fCompHists[comp].first->Fill(vars_buf[comp.first]);
          fCompHists[comp].second->Fill(vars_buf[comp.second]);
        }

        // for (pair<string, string> slope : fSlopes) {
        // }
      }
      total += tin->GetEntries();

      tin->Delete();
      f_rootfile.Close();
    }
  }

  cout << __PRETTY_FUNCTION__ << ":INFO\t read " << ok << "/" << total << " ok events." << endl;
}

double TMulPlot::get_custom_value(Node *node) {
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

bool TMulPlot::CheckEntryCut(const long entry) {
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

void TMulPlot::DrawHistogram() {
  TCanvas c("c", "c", 800, 600);
  c.SetGridy();
  if (logy)
    c.SetLogy();
  gStyle->SetOptFit(111);
  gStyle->SetOptStat(111110);
  gStyle->SetTitleX(0.5);
  gStyle->SetTitleAlign(23);
  if (format == pdf)
    c.Print(Form("%s.pdf[", out_name));


  for (string custom : fCustoms)
    fSolos.insert(custom);
  for (string solo : fSolos) {
    c.cd();
    fSoloHists[solo]->Fit("gaus");
    TH1F * hc = (TH1F*) fSoloHists[solo]->DrawClone();

    gPad->Update();
    TPaveStats * st = (TPaveStats*) gPad->GetPrimitive("stats");
    st->SetName("myStats");
    TList* l_line = st->GetListOfLines();
    l_line->Remove(st->GetLineWith("Constant"));
    hc->SetStats(0);
    gPad->Modified();
    fSoloHists[solo]->SetStats(0);
    fSoloHists[solo]->Draw("same");

    if (format == pdf) 
      c.Print(Form("%s.pdf", out_name));
    else if (format == png)
      c.Print(Form("%s_%s.png", out_name, solo.c_str()));

    c.Clear();
  }
  for (pair<string, string> comp : fComps) {
    c.cd();
    TH1F * h1 = fCompHists[comp].first;
    TH1F * h2 = fCompHists[comp].second;
    Color_t color1 = h1->GetLineColor();
    Color_t color2 = 28;
    h1->SetTitle(Form("#color[%d]{%s} & #color[%d]{%s}", color1, comp.first.c_str(), color2, comp.second.c_str()));
    h1->Fit("gaus");
    h2->Fit("gaus");
    TH1F * hc1 = (TH1F*) h1->DrawClone();
    h2->SetLineColor(color2);
    TH1F * hc2 = (TH1F*) h2->DrawClone("sames");
    double max1 = hc1->GetMaximum();
    double min1 = hc1->GetMinimum();
    double max2 = hc2->GetMaximum();
    double min2 = hc2->GetMinimum();
    double min  = min1 < min2 ? min1 : min2;
    if (logy && min <= 0) min = 0.1;
    double max  = max1 > max2 ? max1 : max2;

    c.Update();
    TPaveStats * st1 = (TPaveStats*) hc1->FindObject("stats");
    st1->SetName("stats1");
    double width = st1->GetX2NDC() - st1->GetX1NDC();
    st1->SetX1NDC(0.1);
    st1->SetX2NDC(0.1 + width);
    st1->SetTextColor(color1);
    st1->GetListOfLines()->Remove(st1->GetLineWith("Constant"));
    hc1->SetStats(0);

    TPaveStats * st2 = (TPaveStats*) hc2->FindObject("stats");
    st2->SetName("stats2");
    st2->SetTextColor(color2);
    st2->GetListOfLines()->Remove(st2->GetLineWith("Constant"));
    hc2->SetStats(0);

    hc1->GetYaxis()->SetRangeUser(min, max);
    h1->SetStats(0);
    h1->Draw("same");
    h2->SetStats(0);
    h2->Draw("same");
    c.Modified();

    if (format == pdf) 
      c.Print(Form("%s.pdf", out_name));
    else if (format == png)
      c.Print(Form("%s_%s-%s.png", out_name, comp.first.c_str(), comp.second.c_str()));
    c.Clear();
  }

  if (format == pdf)
    c.Print(Form("%s.pdf]", out_name));

  cout << __PRETTY_FUNCTION__ << ":INFO\t done with drawing plots\n";
}
#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
